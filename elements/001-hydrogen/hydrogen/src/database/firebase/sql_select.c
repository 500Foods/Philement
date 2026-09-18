/*
 * INSERT…SELECT, WITH CTE MAX+1, SELECT *, and RETURNING for Firebase.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "http.h"
#include "query.h"
#include "sql_parse.h"
#include "sql_expr.h"
#include "sql_ddl.h"
#include "sql_dml.h"
#include "sql_select.h"

#include <jansson.h>

bool firebase_sql_parse_select_list(const char** cursor, FirebaseSqlSelect* sel,
                                    FirebaseSqlStatement* stmt) {
    if (!cursor || !sel || !stmt) {
        return false;
    }
    do {
        FirebaseSqlSelectItem item = {0};
        char* err = NULL;
        item.expr = firebase_expr_parse(cursor, &err);
        if (!item.expr) {
            firebase_sql_set_error(stmt, err ? err : "SELECT: invalid expression");
            free(err);
            return false;
        }
        free(err);
        if (firebase_sql_match_keyword(cursor, "AS")) {
            if (!firebase_sql_parse_ident(cursor, &item.alias)) {
                firebase_sql_select_item_free(&item);
                return firebase_sql_set_error(stmt, "SELECT: expected alias after AS");
            }
        } else if (item.expr->kind == FIREBASE_EXPR_IDENT && item.expr->text) {
            item.alias = strdup(item.expr->text);
            if (!item.alias) {
                firebase_sql_select_item_free(&item);
                return firebase_sql_set_error(stmt, "SELECT: out of memory");
            }
        }
        if (!firebase_sql_select_add_item(sel, item)) {
            return firebase_sql_set_error(stmt, "SELECT: out of memory");
        }
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        break;
    } while (1);
    return sel->item_count > 0;
}

bool firebase_sql_parse_returning(const char** cursor, FirebaseSqlStatement* stmt) {
    if (!stmt) {
        return false;
    }
    if (!firebase_sql_match_keyword(cursor, "RETURNING")) {
        return true;
    }
    do {
        char* ident = NULL;
        if (!firebase_sql_parse_ident(cursor, &ident)) {
            return firebase_sql_set_error(stmt, "RETURNING: expected column");
        }
        char** grown = realloc(stmt->insert.returning,
                               (stmt->insert.returning_count + 1) * sizeof(char*));
        if (!grown) {
            free(ident);
            return firebase_sql_set_error(stmt, "RETURNING: out of memory");
        }
        stmt->insert.returning = grown;
        stmt->insert.returning[stmt->insert.returning_count] = ident;
        stmt->insert.returning_count++;
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        break;
    } while (1);
    return stmt->insert.returning_count > 0;
}

bool firebase_sql_parse_insert_source(const char** cursor, FirebaseSqlStatement* stmt) {
    if (!cursor || !stmt) {
        return false;
    }
    if (firebase_sql_match_keyword(cursor, "WITH")) {
        if (!firebase_sql_parse_ident(cursor, &stmt->insert.cte.name)) {
            return firebase_sql_set_error(stmt, "WITH: expected CTE name");
        }
        if (!firebase_sql_match_keyword(cursor, "AS")) {
            return firebase_sql_set_error(stmt, "WITH: expected AS");
        }
        firebase_sql_skip(cursor);
        if (**cursor != '(') {
            return firebase_sql_set_error(stmt, "WITH: expected (");
        }
        (*cursor)++;
        if (!firebase_sql_match_keyword(cursor, "SELECT")) {
            return firebase_sql_set_error(stmt, "WITH: expected SELECT");
        }
        if (!firebase_sql_parse_select_list(cursor, &stmt->insert.cte.query, stmt)) {
            return false;
        }
        if (!firebase_sql_match_keyword(cursor, "FROM")) {
            return firebase_sql_set_error(stmt, "WITH: expected FROM");
        }
        if (!firebase_sql_parse_ident(cursor, &stmt->insert.cte.query.from_name)) {
            return firebase_sql_set_error(stmt, "WITH: expected table name");
        }
        firebase_sql_skip(cursor);
        if (**cursor != ')') {
            return firebase_sql_set_error(stmt, "WITH: expected )");
        }
        (*cursor)++;
        if (!firebase_sql_match_keyword(cursor, "SELECT")) {
            return firebase_sql_set_error(stmt, "INSERT: expected SELECT after WITH");
        }
        if (!firebase_sql_parse_select_list(cursor, &stmt->insert.select, stmt)) {
            return false;
        }
        if (!firebase_sql_match_keyword(cursor, "FROM")) {
            return firebase_sql_set_error(stmt, "SELECT: expected FROM");
        }
        if (!firebase_sql_parse_ident(cursor, &stmt->insert.select.from_name)) {
            return firebase_sql_set_error(stmt, "SELECT: expected CTE name");
        }
        if (!stmt->insert.cte.name || !stmt->insert.select.from_name ||
            strcasecmp(stmt->insert.cte.name, stmt->insert.select.from_name) != 0) {
            return firebase_sql_set_error(stmt, "SELECT: FROM must match CTE name");
        }
        if (stmt->insert.column_count == 0) {
            return firebase_sql_set_error(stmt, "INSERT SELECT: column list required");
        }
        if (stmt->insert.select.item_count != stmt->insert.column_count) {
            return firebase_sql_set_error(stmt, "INSERT SELECT: column/select count mismatch");
        }
        return firebase_sql_parse_returning(cursor, stmt);
    }

    if (!firebase_sql_match_keyword(cursor, "SELECT")) {
        return firebase_sql_set_error(stmt, "INSERT: expected VALUES, WITH, or SELECT");
    }
    firebase_sql_skip(cursor);
    if (**cursor == '*') {
        (*cursor)++;
        stmt->insert.select.star = true;
        if (!firebase_sql_match_keyword(cursor, "FROM")) {
            return firebase_sql_set_error(stmt, "SELECT *: expected FROM");
        }
        if (!firebase_sql_parse_ident(cursor, &stmt->insert.select.from_name)) {
            return firebase_sql_set_error(stmt, "SELECT *: expected table name");
        }
        return firebase_sql_parse_returning(cursor, stmt);
    }
    return firebase_sql_set_error(stmt, "INSERT SELECT requires WITH or SELECT *");
}

json_t* firebase_select_value_to_json(const FirebaseValue* value) {
    if (firebase_value_is_null(value)) {
        return json_null();
    }
    if (value->kind == FIREBASE_VAL_INT) {
        return json_integer(value->i);
    }
    if (value->kind == FIREBASE_VAL_DOUBLE) {
        return json_real(value->d);
    }
    char* text = firebase_value_as_text(value);
    json_t* out = json_string(text ? text : "");
    free(text);
    return out;
}

json_t* firebase_select_field_to_json(json_t* field) {
    FirebaseValue value;
    firebase_dml_firestore_field_to_value(field, &value);
    json_t* out = firebase_select_value_to_json(&value);
    firebase_value_free(&value);
    return out;
}

bool firebase_select_set_returning(QueryResult** result, int affected, json_t* fields,
                                   char** columns, size_t column_count) {
    json_t* row = json_object();
    json_t* rows = json_array();
    if (!row || !rows) {
        json_decref(row);
        json_decref(rows);
        return firebase_dml_fail(result, "RETURNING: out of memory");
    }
    char** names = NULL;
    if (column_count > 0) {
        names = calloc(column_count, sizeof(char*));
        if (!names) {
            json_decref(row);
            json_decref(rows);
            return firebase_dml_fail(result, "RETURNING: out of memory");
        }
    }
    for (size_t i = 0; i < column_count; i++) {
        const char* col = columns[i];
        json_t* field = fields ? json_object_get(fields, col) : NULL;
        json_t* value = firebase_select_field_to_json(field);
        if (!value) {
            value = json_null();
        }
        json_object_set_new(row, col, value);
        names[i] = strdup(col ? col : "");
        if (!names[i]) {
            for (size_t j = 0; j < i; j++) {
                free(names[j]);
            }
            free(names);
            json_decref(row);
            json_decref(rows);
            return firebase_dml_fail(result, "RETURNING: out of memory");
        }
    }
    json_array_append_new(rows, row);
    char* dumped = json_dumps(rows, JSON_COMPACT);
    json_decref(rows);
    QueryResult* ok = firebase_query_result_ok(affected);
    if (!ok || !dumped) {
        free(dumped);
        if (names) {
            for (size_t i = 0; i < column_count; i++) {
                free(names[i]);
            }
            free(names);
        }
        if (ok) {
            free(ok->data_json);
            free(ok);
        }
        return firebase_dml_fail(result, "RETURNING: out of memory");
    }
    free(ok->data_json);
    ok->data_json = dumped;
    ok->row_count = 1;
    ok->column_count = column_count;
    ok->column_names = names;
    if (result) {
        *result = ok;
    } else {
        database_engine_cleanup_result(ok);
    }
    return true;
}

bool firebase_select_eval_max(json_t* documents, const char* column, FirebaseValue* out) {
    if (!out) {
        return false;
    }
    firebase_value_init(out);
    if (!json_is_array(documents) || !column) {
        return firebase_value_set_null(out);
    }
    bool any = false;
    long long max_value = 0;
    size_t n = json_array_size(documents);
    for (size_t i = 0; i < n; i++) {
        json_t* doc = json_array_get(documents, i);
        json_t* fields = json_object_get(doc, "fields");
        FirebaseValue val;
        firebase_dml_lookup_field(column, &val, fields);
        long long number = 0;
        if (firebase_value_as_int(&val, &number)) {
            if (!any || number > max_value) {
                max_value = number;
            }
            any = true;
        }
        firebase_value_free(&val);
    }
    if (!any) {
        return firebase_value_set_null(out);
    }
    return firebase_value_set_int(out, max_value);
}

bool firebase_select_eval_expr(const FirebaseExpr* expr, json_t* row_fields, json_t* documents,
                               FirebaseValue* out, char** error) {
    if (!out) {
        return false;
    }
    firebase_value_init(out);
    if (!expr) {
        return firebase_expr_set_eval_error(error, "expression is NULL");
    }
    if (expr->kind == FIREBASE_EXPR_ADD) {
        if (expr->arg_count != 2) {
            return firebase_expr_set_eval_error(error, "invalid addition");
        }
        FirebaseValue left;
        FirebaseValue right;
        if (!firebase_select_eval_expr(expr->args[0], row_fields, documents, &left, error) ||
            !firebase_select_eval_expr(expr->args[1], row_fields, documents, &right, error)) {
            firebase_value_free(&left);
            firebase_value_free(&right);
            return false;
        }
        if (firebase_value_is_null(&left) || firebase_value_is_null(&right)) {
            firebase_value_free(&left);
            firebase_value_free(&right);
            return firebase_value_set_null(out);
        }
        long long left_i = 0;
        long long right_i = 0;
        bool ok = firebase_value_as_int(&left, &left_i) && firebase_value_as_int(&right, &right_i) &&
                  firebase_value_set_int(out, left_i + right_i);
        firebase_value_free(&left);
        firebase_value_free(&right);
        if (!ok) {
            return firebase_expr_set_eval_error(error, "addition requires integers");
        }
        return true;
    }
    if (expr->kind == FIREBASE_EXPR_CALL && expr->text && strcasecmp(expr->text, "MAX") == 0) {
        if (expr->arg_count != 1 || !expr->args[0] || expr->args[0]->kind != FIREBASE_EXPR_IDENT) {
            return firebase_expr_set_eval_error(error, "MAX expects one column");
        }
        return firebase_select_eval_max(documents, expr->args[0]->text, out);
    }
    if (expr->kind == FIREBASE_EXPR_CALL && expr->text && strcasecmp(expr->text, "COALESCE") == 0) {
        if (expr->arg_count < 1) {
            return firebase_expr_set_eval_error(error, "COALESCE expects at least one argument");
        }
        firebase_value_set_null(out);
        for (size_t i = 0; i < expr->arg_count; i++) {
            FirebaseValue item;
            if (!firebase_select_eval_expr(expr->args[i], row_fields, documents, &item, error)) {
                firebase_value_free(&item);
                return false;
            }
            if (!firebase_value_is_null(&item)) {
                bool ok = firebase_value_copy(out, &item);
                firebase_value_free(&item);
                return ok || firebase_expr_set_eval_error(error, "out of memory");
            }
            firebase_value_free(&item);
        }
        return true;
    }
    return firebase_expr_eval(expr, firebase_dml_lookup_field, row_fields, out, error);
}

bool firebase_select_commit_row(FirebaseConnection* fb, const char* collection, json_t* pk,
                                json_t* uniques, json_t* existing, json_t* written,
                                json_t* fields, json_t* id_parts, QueryResult** result) {
    char* doc_id = firebase_dml_document_id(pk, id_parts);
    if (!doc_id) {
        return firebase_dml_fail(result, "INSERT: missing primary key");
    }
    size_t written_n = json_is_array(written) ? json_array_size(written) : 0;
    for (size_t w = 0; w < written_n; w++) {
        const char* prev = json_string_value(json_array_get(written, w));
        if (prev && strcmp(prev, doc_id) == 0) {
            free(doc_id);
            return firebase_dml_fail(result, "INSERT: duplicate primary key");
        }
    }
    char* doc_url = firebase_http_build_document_url(fb->base_url, collection, doc_id);
    FirebaseHttpResponse* got = doc_url ? firebase_ddl_http_get(fb, doc_url) : NULL;
    if (!doc_url || !got) {
        free(doc_url);
        firebase_http_response_free(got);
        free(doc_id);
        return firebase_dml_fail(result, "INSERT: existence GET failed");
    }
    if (firebase_http_status_is_healthy(got->http_status)) {
        firebase_http_response_free(got);
        free(doc_url);
        free(doc_id);
        return firebase_dml_fail(result, "INSERT: duplicate primary key");
    }
    firebase_http_response_free(got);
    if (existing && firebase_dml_unique_conflict(uniques, fields, existing)) {
        free(doc_url);
        free(doc_id);
        return firebase_dml_fail(result, "INSERT: UNIQUE");
    }
    json_t* payload = json_object();
    json_object_set(payload, "fields", fields);
    char* body = json_dumps(payload, JSON_COMPACT);
    json_decref(payload);
    FirebaseHttpResponse* patch = body ? firebase_ddl_http_patch(fb, doc_url, body) : NULL;
    free(body);
    free(doc_url);
    if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
        firebase_http_response_free(patch);
        free(doc_id);
        return firebase_dml_fail(result, "INSERT: PATCH failed");
    }
    firebase_http_response_free(patch);
    if (written) {
        json_array_append_new(written, json_string(doc_id));
    }
    if (existing) {
        json_t* clone = json_object();
        json_object_set(clone, "fields", fields);
        json_array_append_new(existing, clone);
    }
    free(doc_id);
    return true;
}

bool firebase_select_fill_defaults(json_t* columns, json_t* fields, json_t* id_parts,
                                   QueryResult** result) {
    size_t catalog_n = json_is_array(columns) ? json_array_size(columns) : 0;
    for (size_t i = 0; i < catalog_n; i++) {
        json_t* col_meta = json_array_get(columns, i);
        const char* col_name = json_string_value(json_object_get(col_meta, "name"));
        if (!col_name || json_object_get(fields, col_name)) {
            continue;
        }
        const char* default_sql = json_string_value(json_object_get(col_meta, "default_sql"));
        FirebaseValue val;
        firebase_value_init(&val);
        if (default_sql && *default_sql) {
            const char* dcur = default_sql;
            char* err = NULL;
            FirebaseExpr* dexpr = firebase_expr_parse(&dcur, &err);
            bool eval_ok = dexpr && firebase_expr_eval(dexpr, NULL, NULL, &val, &err);
            firebase_expr_free(dexpr);
            free(err);
            if (!eval_ok) {
                firebase_value_free(&val);
                return firebase_dml_fail(result, "INSERT: DEFAULT failed");
            }
        } else {
            firebase_value_set_null(&val);
        }
        if (json_object_get(col_meta, "not_null") == json_true() && firebase_value_is_null(&val)) {
            firebase_value_free(&val);
            return firebase_dml_fail(result, "INSERT: NOT NULL");
        }
        if (firebase_value_exceeds_max(&val)) {
            firebase_value_free(&val);
            return firebase_dml_fail(result, "INSERT: field exceeds 900 KiB");
        }
        const char* type = json_string_value(json_object_get(col_meta, "type"));
        json_t* field = firebase_dml_value_to_field(&val, type);
        char* fragment = firebase_dml_id_fragment(&val);
        firebase_value_free(&val);
        if (!field) {
            free(fragment);
            return firebase_dml_fail(result, "INSERT: failed to encode field");
        }
        json_object_set_new(fields, col_name, field);
        if (fragment && id_parts) {
            json_object_set_new(id_parts, col_name, json_string(fragment));
        }
        free(fragment);
    }
    return true;
}

bool firebase_dml_insert_select_star(FirebaseConnection* fb, const FirebaseSqlStatement* stmt,
                                     QueryResult** result) {
    char* dest = firebase_resolve_collection_name(fb, stmt->insert.table);
    char* src = firebase_resolve_collection_name(fb, stmt->insert.select.from_name);
    if (!dest || !src) {
        free(dest);
        free(src);
        return firebase_dml_fail(result, "INSERT SELECT *: missing table");
    }
    json_t* docs = firebase_dml_list_documents(fb, src, result);
    if (!docs) {
        free(dest);
        free(src);
        return false;
    }
    int affected = 0;
    size_t n = json_array_size(docs);
    json_t* last_fields = NULL;
    for (size_t i = 0; i < n; i++) {
        json_t* doc = json_array_get(docs, i);
        json_t* fields = json_object_get(doc, "fields");
        const char* name = json_string_value(json_object_get(doc, "name"));
        const char* id = firebase_ddl_doc_id_from_name(name);
        if (!fields || !id) {
            continue;
        }
        json_t* payload = json_object();
        json_object_set(payload, "fields", fields);
        char* body = json_dumps(payload, JSON_COMPACT);
        json_decref(payload);
        char* url = firebase_http_build_document_url(fb->base_url, dest, id);
        FirebaseHttpResponse* patch = (url && body) ? firebase_ddl_http_patch(fb, url, body) : NULL;
        free(url);
        free(body);
        if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
            firebase_http_response_free(patch);
            json_decref(docs);
            free(dest);
            free(src);
            return firebase_dml_fail(result, "INSERT SELECT *: PATCH failed");
        }
        firebase_http_response_free(patch);
        last_fields = fields;
        affected++;
    }
    bool ok;
    if (stmt->insert.returning_count > 0) {
        ok = firebase_select_set_returning(result, affected, last_fields,
                                           stmt->insert.returning, stmt->insert.returning_count);
    } else {
        ok = true;
        if (result) {
            *result = firebase_query_result_ok(affected);
        }
    }
    json_decref(docs);
    free(dest);
    free(src);
    return ok;
}

bool firebase_dml_insert_select(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->insert.table) {
        return firebase_dml_fail(result, "INSERT SELECT: invalid connection");
    }
    if (stmt->insert.select.star) {
        return firebase_dml_insert_select_star(fb, stmt, result);
    }
    if (!stmt->insert.cte.name || stmt->insert.select.item_count == 0) {
        return firebase_dml_fail(result, "INSERT SELECT: WITH CTE required");
    }

    char* collection = firebase_resolve_collection_name(fb, stmt->insert.table);
    if (!collection) {
        return firebase_dml_fail(result, "INSERT: missing table");
    }
    json_t* catalog = firebase_dml_load_catalog(fb, collection, result);
    if (!catalog) {
        free(collection);
        return false;
    }
    json_t* columns = firebase_dml_catalog_json_field(catalog, "columns");
    json_t* pk = firebase_dml_catalog_json_field(catalog, "primary_key");
    json_t* uniques = firebase_dml_catalog_json_field(catalog, "unique_keys");
    if (!columns || !pk || !uniques) {
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        free(collection);
        return firebase_dml_fail(result, "INSERT: invalid catalog");
    }

    char* from_coll = firebase_resolve_collection_name(fb, stmt->insert.cte.query.from_name);
    json_t* docs = from_coll ? firebase_dml_list_documents(fb, from_coll, result) : NULL;
    free(from_coll);
    if (!docs) {
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        free(collection);
        return false;
    }

    json_t* cte_row = json_object();
    if (!cte_row) {
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        json_decref(docs);
        free(collection);
        return firebase_dml_fail(result, "INSERT SELECT: out of memory");
    }
    for (size_t i = 0; i < stmt->insert.cte.query.item_count; i++) {
        const FirebaseSqlSelectItem* item = &stmt->insert.cte.query.items[i];
        char* err = NULL;
        FirebaseValue val;
        if (!firebase_select_eval_expr(item->expr, NULL, docs, &val, &err)) {
            firebase_value_free(&val);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            firebase_dml_fail(result, err ? err : "INSERT SELECT: CTE eval failed");
            free(err);
            return false;
        }
        const char* alias = item->alias ? item->alias : "col";
        json_t* field = firebase_dml_value_to_field(&val, "integer");
        firebase_value_free(&val);
        if (!field) {
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "INSERT SELECT: CTE encode failed");
        }
        json_object_set_new(cte_row, alias, field);
        free(err);
    }

    json_t* fields = json_object();
    json_t* id_parts = json_object();
    if (!fields || !id_parts) {
        json_decref(fields);
        json_decref(id_parts);
        json_decref(cte_row);
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        json_decref(docs);
        free(collection);
        return firebase_dml_fail(result, "INSERT SELECT: out of memory");
    }
    for (size_t c = 0; c < stmt->insert.column_count; c++) {
        const char* col_name = stmt->insert.columns[c];
        json_t* col_meta = firebase_dml_find_column(columns, col_name);
        if (!col_meta) {
            json_decref(fields);
            json_decref(id_parts);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "INSERT: unknown column");
        }
        const FirebaseSqlSelectItem* item = &stmt->insert.select.items[c];
        char* err = NULL;
        FirebaseValue val;
        if (!firebase_select_eval_expr(item->expr, cte_row, docs, &val, &err)) {
            firebase_value_free(&val);
            json_decref(fields);
            json_decref(id_parts);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            firebase_dml_fail(result, err ? err : "INSERT SELECT: expression failed");
            free(err);
            return false;
        }
        if (firebase_value_exceeds_max(&val)) {
            firebase_value_free(&val);
            json_decref(fields);
            json_decref(id_parts);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "INSERT: field exceeds 900 KiB");
        }
        if (json_object_get(col_meta, "not_null") == json_true() && firebase_value_is_null(&val)) {
            firebase_value_free(&val);
            json_decref(fields);
            json_decref(id_parts);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "INSERT: NOT NULL");
        }
        const char* type = json_string_value(json_object_get(col_meta, "type"));
        json_t* field = firebase_dml_value_to_field(&val, type);
        char* fragment = firebase_dml_id_fragment(&val);
        firebase_value_free(&val);
        if (!field) {
            free(fragment);
            json_decref(fields);
            json_decref(id_parts);
            json_decref(cte_row);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "INSERT: failed to encode field");
        }
        json_object_set_new(fields, col_name, field);
        if (fragment) {
            json_object_set_new(id_parts, col_name, json_string(fragment));
            free(fragment);
        }
        free(err);
    }

    if (!firebase_select_fill_defaults(columns, fields, id_parts, result)) {
        json_decref(fields);
        json_decref(id_parts);
        json_decref(cte_row);
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        json_decref(docs);
        free(collection);
        return false;
    }

    json_t* written = json_array();
    if (!firebase_select_commit_row(fb, collection, pk, uniques, docs, written, fields, id_parts,
                                    result)) {
        json_decref(fields);
        json_decref(id_parts);
        json_decref(cte_row);
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        json_decref(docs);
        json_decref(written);
        free(collection);
        return false;
    }

    bool ok;
    if (stmt->insert.returning_count > 0) {
        ok = firebase_select_set_returning(result, 1, fields, stmt->insert.returning,
                                           stmt->insert.returning_count);
    } else {
        ok = true;
        if (result) {
            *result = firebase_query_result_ok(1);
        }
    }
    json_decref(fields);
    json_decref(id_parts);
    json_decref(cte_row);
    json_decref(catalog);
    json_decref(columns);
    json_decref(pk);
    json_decref(uniques);
    json_decref(docs);
    json_decref(written);
    free(collection);
    return ok;
}
