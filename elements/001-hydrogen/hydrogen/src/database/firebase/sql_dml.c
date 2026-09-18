/*
 * Firebase DML parse and execute: INSERT VALUES, UPDATE, DELETE.
 * Transactions stay fail-closed; multi-row writes are sequential.
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
#include "fns_tz.h"

#include <jansson.h>

bool firebase_dml_fail(QueryResult** result, const char* message) {
    return firebase_ddl_fail(result, message);
}

char* firebase_dml_sql_datetime_to_rfc3339(const char* sql_dt) {
    if (!sql_dt || !*sql_dt) {
        return NULL;
    }
    if (strlen(sql_dt) >= 19 && sql_dt[10] == 'T') {
        return strdup(sql_dt);
    }
    struct tm parsed;
    if (!firebase_tz_parse_datetime(sql_dt, &parsed)) {
        return NULL;
    }
    char* out = malloc(32);
    if (!out) {
        return NULL;
    }
    if (strftime(out, 32, "%Y-%m-%dT%H:%M:%SZ", &parsed) == 0) {
        free(out);
        return NULL;
    }
    return out;
}

json_t* firebase_dml_value_to_field(const FirebaseValue* value, const char* sql_type) {
    json_t* field = json_object();
    if (!field) {
        return NULL;
    }
    if (firebase_value_is_null(value)) {
        if (json_object_set_new(field, "nullValue", json_null()) != 0) {
            json_decref(field);
            return NULL;
        }
        return field;
    }
    if (sql_type && strcasecmp(sql_type, "timestamp") == 0) {
        char* text = firebase_value_as_text(value);
        char* rfc = firebase_dml_sql_datetime_to_rfc3339(text);
        free(text);
        if (!rfc) {
            json_decref(field);
            return NULL;
        }
        int rc = json_object_set_new(field, "timestampValue", json_string(rfc));
        free(rfc);
        if (rc != 0) {
            json_decref(field);
            return NULL;
        }
        return field;
    }
    if (sql_type && (strcasecmp(sql_type, "integer") == 0 || strcasecmp(sql_type, "bigint") == 0)) {
        long long number = 0;
        if (!firebase_value_as_int(value, &number)) {
            json_decref(field);
            return NULL;
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", number);
        if (json_object_set_new(field, "integerValue", json_string(buf)) != 0) {
            json_decref(field);
            return NULL;
        }
        return field;
    }
    if (sql_type && strcasecmp(sql_type, "real") == 0) {
        double number = (value->kind == FIREBASE_VAL_DOUBLE) ? value->d : (double)value->i;
        if (json_object_set_new(field, "doubleValue", json_real(number)) != 0) {
            json_decref(field);
            return NULL;
        }
        return field;
    }
    char* text = firebase_value_as_text(value);
    int rc = json_object_set_new(field, "stringValue", json_string(text ? text : ""));
    free(text);
    if (rc != 0) {
        json_decref(field);
        return NULL;
    }
    return field;
}

bool firebase_dml_firestore_field_to_value(json_t* field, FirebaseValue* out) {
    firebase_value_init(out);
    if (!field) {
        return firebase_value_set_null(out);
    }
    if (json_object_get(field, "nullValue")) {
        return firebase_value_set_null(out);
    }
    json_t* integer_v = json_object_get(field, "integerValue");
    if (json_is_string(integer_v)) {
        return firebase_value_set_int(out, strtoll(json_string_value(integer_v), NULL, 10));
    }
    json_t* double_v = json_object_get(field, "doubleValue");
    if (json_is_number(double_v)) {
        return firebase_value_set_double(out, json_number_value(double_v));
    }
    json_t* string_v = json_object_get(field, "stringValue");
    if (json_is_string(string_v)) {
        return firebase_value_copy_text(out, json_string_value(string_v));
    }
    json_t* ts = json_object_get(field, "timestampValue");
    if (json_is_string(ts)) {
        return firebase_value_copy_text(out, json_string_value(ts));
    }
    return firebase_value_set_null(out);
}

bool firebase_dml_lookup_field(const char* name, FirebaseValue* out, void* userdata) {
    json_t* fields = (json_t*)userdata;
    if (!name || !out) {
        return false;
    }
    json_t* field = json_object_get(fields, name);
    return firebase_dml_firestore_field_to_value(field, out);
}

bool firebase_dml_values_match(const FirebaseValue* left, const FirebaseValue* right) {
    if (firebase_value_is_null(left) && firebase_value_is_null(right)) {
        return true;
    }
    if (firebase_value_is_null(left) || firebase_value_is_null(right)) {
        return false;
    }
    long long left_i = 0;
    long long right_i = 0;
    if (firebase_value_as_int(left, &left_i) && firebase_value_as_int(right, &right_i)) {
        return left_i == right_i;
    }
    char* left_s = firebase_value_as_text(left);
    char* right_s = firebase_value_as_text(right);
    bool same = left_s && right_s && strcmp(left_s, right_s) == 0;
    free(left_s);
    free(right_s);
    return same;
}

bool firebase_dml_where_match(const FirebaseSqlWhere* where, json_t* fields, char** error) {
    if (!where || where->count == 0) {
        return false;
    }
    for (size_t i = 0; i < where->count; i++) {
        const FirebaseSqlPredicate* pred = &where->predicates[i];
        FirebaseValue colval;
        firebase_dml_lookup_field(pred->column, &colval, fields);
        bool match = false;
        if (pred->op == FIREBASE_PRED_IS_NULL) {
            match = firebase_value_is_null(&colval);
        } else if (pred->op == FIREBASE_PRED_EQ) {
            FirebaseValue rhs;
            if (!firebase_expr_eval(pred->value, firebase_dml_lookup_field, fields, &rhs, error)) {
                firebase_value_free(&colval);
                return false;
            }
            match = firebase_dml_values_match(&colval, &rhs);
            firebase_value_free(&rhs);
        } else if (pred->op == FIREBASE_PRED_IN) {
            for (size_t j = 0; j < pred->in_count; j++) {
                FirebaseValue item;
                if (!firebase_expr_eval(pred->in_values[j], firebase_dml_lookup_field, fields, &item, error)) {
                    firebase_value_free(&colval);
                    return false;
                }
                match = firebase_dml_values_match(&colval, &item);
                firebase_value_free(&item);
                if (match) {
                    break;
                }
            }
        }
        firebase_value_free(&colval);
        if (!match) {
            return false;
        }
    }
    return true;
}

json_t* firebase_dml_load_catalog(FirebaseConnection* fb, const char* collection, QueryResult** result) {
    char* url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION, collection);
    if (!url) {
        firebase_dml_fail(result, "DML: out of memory");
        return NULL;
    }
    FirebaseHttpResponse* resp = firebase_ddl_http_get(fb, url);
    free(url);
    if (!resp) {
        firebase_dml_fail(result, "DML: catalog GET failed");
        return NULL;
    }
    if (!firebase_http_status_is_healthy(resp->http_status)) {
        firebase_http_response_free(resp);
        firebase_dml_fail(result, "DML: table does not exist");
        return NULL;
    }
    json_t* doc = firebase_catalog_load(resp->body);
    firebase_http_response_free(resp);
    if (!doc) {
        firebase_dml_fail(result, "DML: invalid catalog");
        return NULL;
    }
    return doc;
}

json_t* firebase_dml_list_documents(FirebaseConnection* fb, const char* collection, QueryResult** result) {
    char* url = firebase_http_build_collection_url(fb->base_url, collection);
    if (!url) {
        firebase_dml_fail(result, "DML: out of memory");
        return NULL;
    }
    FirebaseHttpResponse* resp = firebase_ddl_http_get(fb, url);
    free(url);
    if (!resp) {
        firebase_dml_fail(result, "DML: list failed");
        return NULL;
    }
    if (resp->http_status == 404) {
        firebase_http_response_free(resp);
        return json_array();
    }
    if (!firebase_http_status_is_healthy(resp->http_status)) {
        firebase_http_response_free(resp);
        firebase_dml_fail(result, "DML: failed to list collection");
        return NULL;
    }
    json_t* root = firebase_catalog_load(resp->body);
    firebase_http_response_free(resp);
    json_t* docs = root ? json_object_get(root, "documents") : NULL;
    json_t* copy = json_is_array(docs) ? json_deep_copy(docs) : json_array();
    json_decref(root);
    if (!copy) {
        firebase_dml_fail(result, "DML: out of memory");
        return NULL;
    }
    return copy;
}

char* firebase_dml_document_id(json_t* pk_names, json_t* values_by_column) {
    if (!json_is_array(pk_names) || json_array_size(pk_names) == 0 || !json_is_object(values_by_column)) {
        return NULL;
    }
    size_t cap = 64;
    char* id = calloc(1, cap);
    if (!id) {
        return NULL;
    }
    size_t n = 0;
    size_t pk_n = json_array_size(pk_names);
    for (size_t i = 0; i < pk_n; i++) {
        const char* col = json_string_value(json_array_get(pk_names, i));
        const char* text = json_string_value(json_object_get(values_by_column, col));
        if (!col || !text || !*text) {
            free(id);
            return NULL;
        }
        size_t add = strlen(text) + (i > 0 ? 1 : 0);
        if (n + add + 1 > cap) {
            cap = (n + add + 1) * 2;
            char* grown = realloc(id, cap);
            if (!grown) {
                free(id);
                return NULL;
            }
            id = grown;
        }
        if (i > 0) {
            id[n++] = '_';
        }
        memcpy(id + n, text, strlen(text));
        n += strlen(text);
        id[n] = '\0';
    }
    return id;
}

char* firebase_dml_id_fragment(const FirebaseValue* value) {
    if (firebase_value_is_null(value)) {
        return NULL;
    }
    if (value->kind == FIREBASE_VAL_INT) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", value->i);
        return strdup(buf);
    }
    return firebase_value_as_text(value);
}

json_t* firebase_dml_catalog_json_field(json_t* catalog, const char* field) {
    char* raw = firebase_catalog_get_field(catalog, field);
    if (!raw) {
        return NULL;
    }
    json_t* parsed = json_loads(raw, 0, NULL);
    free(raw);
    return parsed;
}

const char* firebase_dml_column_type(json_t* columns, const char* name) {
    if (!json_is_array(columns) || !name) {
        return NULL;
    }
    size_t n = json_array_size(columns);
    for (size_t i = 0; i < n; i++) {
        json_t* col = json_array_get(columns, i);
        const char* col_name = json_string_value(json_object_get(col, "name"));
        if (col_name && strcmp(col_name, name) == 0) {
            return json_string_value(json_object_get(col, "type"));
        }
    }
    return NULL;
}

json_t* firebase_dml_find_column(json_t* columns, const char* name) {
    if (!json_is_array(columns) || !name) {
        return NULL;
    }
    size_t n = json_array_size(columns);
    for (size_t i = 0; i < n; i++) {
        json_t* col = json_array_get(columns, i);
        const char* col_name = json_string_value(json_object_get(col, "name"));
        if (col_name && strcmp(col_name, name) == 0) {
            return col;
        }
    }
    return NULL;
}

bool firebase_dml_unique_conflict(json_t* unique_keys, json_t* new_fields, json_t* existing_docs) {
    if (!json_is_array(unique_keys) || json_array_size(unique_keys) == 0) {
        return false;
    }
    size_t key_n = json_array_size(unique_keys);
    size_t doc_n = json_is_array(existing_docs) ? json_array_size(existing_docs) : 0;
    for (size_t k = 0; k < key_n; k++) {
        json_t* key = json_array_get(unique_keys, k);
        if (!json_is_array(key) || json_array_size(key) == 0) {
            continue;
        }
        for (size_t d = 0; d < doc_n; d++) {
            json_t* doc = json_array_get(existing_docs, d);
            json_t* fields = json_object_get(doc, "fields");
            if (!fields && json_is_object(doc)) {
                fields = doc;
            }
            bool all = true;
            size_t cols = json_array_size(key);
            for (size_t c = 0; c < cols; c++) {
                const char* col = json_string_value(json_array_get(key, c));
                FirebaseValue left;
                FirebaseValue right;
                firebase_dml_lookup_field(col, &left, new_fields);
                firebase_dml_lookup_field(col, &right, fields);
                bool same = firebase_dml_values_match(&left, &right);
                firebase_value_free(&left);
                firebase_value_free(&right);
                if (!same) {
                    all = false;
                    break;
                }
            }
            if (all) {
                return true;
            }
        }
    }
    return false;
}

bool firebase_dml_insert(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result) {
    if (stmt && (stmt->insert.select.star || stmt->insert.select.item_count > 0)) {
        return firebase_dml_insert_select(connection, stmt, result);
    }
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->insert.table) {
        return firebase_dml_fail(result, "INSERT: invalid connection");
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
    json_t* written = json_array();
    if (!columns || !pk || !uniques || !written) {
        json_decref(catalog);
        json_decref(columns);
        json_decref(pk);
        json_decref(uniques);
        json_decref(written);
        free(collection);
        return firebase_dml_fail(result, "INSERT: invalid catalog");
    }

    json_t* existing = NULL;
    if (json_array_size(uniques) > 0) {
        existing = firebase_dml_list_documents(fb, collection, result);
        if (!existing) {
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(written);
            free(collection);
            return false;
        }
    }

    int affected = 0;
    for (size_t r = 0; r < stmt->insert.row_count; r++) {
        const FirebaseSqlInsertRow* row = &stmt->insert.rows[r];
        json_t* fields = json_object();
        json_t* id_parts = json_object();
        if (!fields || !id_parts) {
            json_decref(fields);
            json_decref(id_parts);
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(written);
            json_decref(existing);
            free(collection);
            return firebase_dml_fail(result, "INSERT: out of memory");
        }

        bool row_ok = true;
        const char* fail_msg = NULL;
        for (size_t c = 0; c < stmt->insert.column_count && row_ok; c++) {
            const char* col_name = stmt->insert.columns[c];
            json_t* col_meta = firebase_dml_find_column(columns, col_name);
            if (!col_meta) {
                fail_msg = "INSERT: unknown column";
                row_ok = false;
                break;
            }
            char* err = NULL;
            FirebaseValue val;
            if (!firebase_expr_eval(row->values[c], NULL, NULL, &val, &err)) {
                fail_msg = err ? err : "INSERT: expression failed";
                row_ok = false;
                /* err is freed after fail() copies the message */
                if (!err) {
                    fail_msg = "INSERT: expression failed";
                }
                firebase_value_free(&val);
                if (err) {
                    firebase_dml_fail(result, err);
                    free(err);
                    fail_msg = NULL;
                }
                break;
            }
            if (firebase_value_exceeds_max(&val)) {
                firebase_value_free(&val);
                fail_msg = "INSERT: field exceeds 900 KiB";
                row_ok = false;
                break;
            }
            if (json_object_get(col_meta, "not_null") == json_true() && firebase_value_is_null(&val)) {
                firebase_value_free(&val);
                fail_msg = "INSERT: NOT NULL";
                row_ok = false;
                break;
            }
            const char* type = json_string_value(json_object_get(col_meta, "type"));
            json_t* field = firebase_dml_value_to_field(&val, type);
            char* fragment = firebase_dml_id_fragment(&val);
            firebase_value_free(&val);
            if (!field) {
                free(fragment);
                fail_msg = "INSERT: failed to encode field";
                row_ok = false;
                break;
            }
            json_object_set_new(fields, col_name, field);
            if (fragment) {
                json_object_set_new(id_parts, col_name, json_string(fragment));
                free(fragment);
            }
        }

        size_t catalog_n = json_array_size(columns);
        for (size_t i = 0; i < catalog_n && row_ok; i++) {
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
                if (!eval_ok) {
                    firebase_value_free(&val);
                    free(err);
                    fail_msg = "INSERT: DEFAULT failed";
                    row_ok = false;
                    break;
                }
                free(err);
            } else {
                firebase_value_set_null(&val);
            }
            if (json_object_get(col_meta, "not_null") == json_true() && firebase_value_is_null(&val)) {
                firebase_value_free(&val);
                fail_msg = "INSERT: NOT NULL";
                row_ok = false;
                break;
            }
            if (firebase_value_exceeds_max(&val)) {
                firebase_value_free(&val);
                fail_msg = "INSERT: field exceeds 900 KiB";
                row_ok = false;
                break;
            }
            const char* type = json_string_value(json_object_get(col_meta, "type"));
            json_t* field = firebase_dml_value_to_field(&val, type);
            char* fragment = firebase_dml_id_fragment(&val);
            firebase_value_free(&val);
            if (!field) {
                free(fragment);
                fail_msg = "INSERT: failed to encode field";
                row_ok = false;
                break;
            }
            json_object_set_new(fields, col_name, field);
            if (fragment) {
                json_object_set_new(id_parts, col_name, json_string(fragment));
                free(fragment);
            }
        }

        char* doc_id = row_ok ? firebase_dml_document_id(pk, id_parts) : NULL;
        json_decref(id_parts);
        if (row_ok && !doc_id) {
            fail_msg = "INSERT: missing primary key";
            row_ok = false;
        }

        if (row_ok) {
            size_t written_n = json_array_size(written);
            for (size_t w = 0; w < written_n; w++) {
                const char* prev = json_string_value(json_array_get(written, w));
                if (prev && strcmp(prev, doc_id) == 0) {
                    fail_msg = "INSERT: duplicate primary key";
                    row_ok = false;
                    break;
                }
            }
        }

        if (row_ok) {
            char* doc_url = firebase_http_build_document_url(fb->base_url, collection, doc_id);
            FirebaseHttpResponse* got = doc_url ? firebase_ddl_http_get(fb, doc_url) : NULL;
            if (!doc_url || !got) {
                free(doc_url);
                firebase_http_response_free(got);
                free(doc_id);
                json_decref(fields);
                json_decref(catalog);
                json_decref(columns);
                json_decref(pk);
                json_decref(uniques);
                json_decref(written);
                json_decref(existing);
                free(collection);
                return firebase_dml_fail(result, "INSERT: existence GET failed");
            }
            if (firebase_http_status_is_healthy(got->http_status)) {
                firebase_http_response_free(got);
                free(doc_url);
                fail_msg = "INSERT: duplicate primary key";
                row_ok = false;
            } else {
                firebase_http_response_free(got);
                if (existing && firebase_dml_unique_conflict(uniques, fields, existing)) {
                    free(doc_url);
                    fail_msg = "INSERT: UNIQUE";
                    row_ok = false;
                } else {
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
                        json_decref(fields);
                        json_decref(catalog);
                        json_decref(columns);
                        json_decref(pk);
                        json_decref(uniques);
                        json_decref(written);
                        json_decref(existing);
                        free(collection);
                        return firebase_dml_fail(result, "INSERT: PATCH failed");
                    }
                    firebase_http_response_free(patch);
                    json_array_append_new(written, json_string(doc_id));
                    if (existing) {
                        json_t* clone = json_object();
                        json_object_set(clone, "fields", fields);
                        json_array_append_new(existing, clone);
                    }
                    affected++;
                }
            }
        }

        json_decref(fields);
        free(doc_id);
        if (!row_ok) {
            json_decref(catalog);
            json_decref(columns);
            json_decref(pk);
            json_decref(uniques);
            json_decref(written);
            json_decref(existing);
            free(collection);
            if (fail_msg) {
                return firebase_dml_fail(result, fail_msg);
            }
            return false;
        }
    }

    json_decref(catalog);
    json_decref(columns);
    json_decref(pk);
    json_decref(uniques);
    json_decref(written);
    json_decref(existing);
    free(collection);
    if (result) {
        *result = firebase_query_result_ok(affected);
    }
    return true;
}

bool firebase_dml_update(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->update.table) {
        return firebase_dml_fail(result, "UPDATE: invalid connection");
    }
    if (stmt->update.where.count == 0) {
        return firebase_dml_fail(result, "UPDATE: WHERE is required");
    }
    char* collection = firebase_resolve_collection_name(fb, stmt->update.table);
    if (!collection) {
        return firebase_dml_fail(result, "UPDATE: missing table");
    }
    json_t* catalog = firebase_dml_load_catalog(fb, collection, result);
    if (!catalog) {
        free(collection);
        return false;
    }
    json_t* columns = firebase_dml_catalog_json_field(catalog, "columns");
    json_t* docs = firebase_dml_list_documents(fb, collection, result);
    if (!columns || !docs) {
        json_decref(catalog);
        json_decref(columns);
        json_decref(docs);
        free(collection);
        if (!docs) {
            return false;
        }
        return firebase_dml_fail(result, "UPDATE: invalid catalog");
    }

    int affected = 0;
    size_t n = json_array_size(docs);
    for (size_t i = 0; i < n; i++) {
        json_t* doc = json_array_get(docs, i);
        json_t* fields = json_object_get(doc, "fields");
        char* err = NULL;
        if (!firebase_dml_where_match(&stmt->update.where, fields, &err)) {
            if (err) {
                json_decref(catalog);
                json_decref(columns);
                json_decref(docs);
                free(collection);
                firebase_dml_fail(result, err);
                free(err);
                return false;
            }
            continue;
        }
        json_t* merged = json_deep_copy(fields);
        if (!merged) {
            merged = json_object();
        }
        bool ok = true;
        const char* fail_msg = NULL;
        for (size_t s = 0; s < stmt->update.set_count && ok; s++) {
            FirebaseValue val;
            if (!firebase_expr_eval(stmt->update.sets[s].value, firebase_dml_lookup_field, merged, &val, &err)) {
                firebase_value_free(&val);
                fail_msg = err ? err : "UPDATE: expression failed";
                ok = false;
                break;
            }
            if (firebase_value_exceeds_max(&val)) {
                firebase_value_free(&val);
                fail_msg = "UPDATE: field exceeds 900 KiB";
                ok = false;
                break;
            }
            const char* type = firebase_dml_column_type(columns, stmt->update.sets[s].column);
            json_t* field = firebase_dml_value_to_field(&val, type ? type : "text");
            firebase_value_free(&val);
            if (!field) {
                fail_msg = "UPDATE: failed to encode field";
                ok = false;
                break;
            }
            json_object_set_new(merged, stmt->update.sets[s].column, field);
        }
        if (!ok) {
            json_decref(merged);
            json_decref(catalog);
            json_decref(columns);
            json_decref(docs);
            free(collection);
            firebase_dml_fail(result, fail_msg ? fail_msg : "UPDATE failed");
            free(err);
            return false;
        }
        const char* name = json_string_value(json_object_get(doc, "name"));
        const char* id = firebase_ddl_doc_id_from_name(name);
        char* url = id ? firebase_http_build_document_url(fb->base_url, collection, id) : NULL;
        json_t* payload = json_object();
        json_object_set_new(payload, "fields", merged);
        char* body = json_dumps(payload, JSON_COMPACT);
        json_decref(payload);
        FirebaseHttpResponse* patch = (url && body) ? firebase_ddl_http_patch(fb, url, body) : NULL;
        free(url);
        free(body);
        if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
            firebase_http_response_free(patch);
            json_decref(catalog);
            json_decref(columns);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "UPDATE: PATCH failed");
        }
        firebase_http_response_free(patch);
        affected++;
        free(err);
    }

    json_decref(catalog);
    json_decref(columns);
    json_decref(docs);
    free(collection);
    if (result) {
        *result = firebase_query_result_ok(affected);
    }
    return true;
}

bool firebase_dml_delete(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->del.table) {
        return firebase_dml_fail(result, "DELETE: invalid connection");
    }
    if (stmt->del.where.count == 0) {
        return firebase_dml_fail(result, "DELETE: WHERE is required");
    }
    char* collection = firebase_resolve_collection_name(fb, stmt->del.table);
    if (!collection) {
        return firebase_dml_fail(result, "DELETE: missing table");
    }
    json_t* docs = firebase_dml_list_documents(fb, collection, result);
    if (!docs) {
        free(collection);
        return false;
    }
    int affected = 0;
    size_t n = json_array_size(docs);
    for (size_t i = 0; i < n; i++) {
        json_t* doc = json_array_get(docs, i);
        json_t* fields = json_object_get(doc, "fields");
        char* err = NULL;
        if (!firebase_dml_where_match(&stmt->del.where, fields, &err)) {
            if (err) {
                json_decref(docs);
                free(collection);
                firebase_dml_fail(result, err);
                free(err);
                return false;
            }
            continue;
        }
        const char* name = json_string_value(json_object_get(doc, "name"));
        const char* id = firebase_ddl_doc_id_from_name(name);
        char* url = id ? firebase_http_build_document_url(fb->base_url, collection, id) : NULL;
        FirebaseHttpResponse* del = url ? firebase_ddl_http_delete(fb, url) : NULL;
        free(url);
        if (!del || (!firebase_http_status_is_healthy(del->http_status) && del->http_status != 404)) {
            firebase_http_response_free(del);
            json_decref(docs);
            free(collection);
            return firebase_dml_fail(result, "DELETE: failed");
        }
        firebase_http_response_free(del);
        affected++;
        free(err);
    }
    json_decref(docs);
    free(collection);
    if (result) {
        *result = firebase_query_result_ok(affected);
    }
    return true;
}

bool firebase_dml_execute(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                          QueryResult** result) {
    if (!stmt) {
        return firebase_dml_fail(result, "DML: NULL statement");
    }
    switch (stmt->kind) {
        case FIREBASE_SQL_KIND_INSERT:
            return firebase_dml_insert(connection, stmt, result);
        case FIREBASE_SQL_KIND_UPDATE:
            return firebase_dml_update(connection, stmt, result);
        case FIREBASE_SQL_KIND_DELETE:
            return firebase_dml_delete(connection, stmt, result);
        case FIREBASE_SQL_KIND_NONE:
        case FIREBASE_SQL_KIND_CREATE_TABLE:
        case FIREBASE_SQL_KIND_DROP_TABLE:
        case FIREBASE_SQL_KIND_CREATE_INDEX:
        case FIREBASE_SQL_KIND_ALTER_TABLE:
        case FIREBASE_SQL_KIND_CREATE_FUNCTION:
        case FIREBASE_SQL_KIND_DROP_FUNCTION:
        case FIREBASE_SQL_KIND_REFUSE_DROP:
        case FIREBASE_SQL_KIND_UNSUPPORTED:
            return firebase_dml_fail(result, "DML: not a DML statement");
    }
    return firebase_dml_fail(result, "DML: not a DML statement");
}
