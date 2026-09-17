/*
 * Firebase DDL: persist table metadata in collection _schema and
 * manage data collections over the HTTP seam.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "http.h"
#include "query.h"
#include "sql_parse.h"
#include "sql_ddl.h"

#include <jansson.h>

FirebaseConnection* firebase_ddl_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBASE ||
        !connection->connection_handle) {
        return NULL;
    }
    return (FirebaseConnection*)connection->connection_handle;
}

FirebaseHttpResponse* firebase_ddl_http_get(FirebaseConnection* fb, const char* url) {
    if (!fb) {
        return NULL;
    }
    return firebase_http_get(url, NULL, false, &fb->inflight, &fb->abort_requested);
}

FirebaseHttpResponse* firebase_ddl_http_patch(FirebaseConnection* fb, const char* url,
                                              const char* body) {
    if (!fb) {
        return NULL;
    }
    return firebase_http_patch(url, body, NULL, false, &fb->inflight, &fb->abort_requested);
}

FirebaseHttpResponse* firebase_ddl_http_delete(FirebaseConnection* fb, const char* url) {
    if (!fb) {
        return NULL;
    }
    return firebase_http_delete(url, NULL, false, &fb->inflight, &fb->abort_requested);
}

char* firebase_resolve_collection_name(const FirebaseConnection* fb, const char* sql_name) {
    if (!sql_name || !*sql_name) {
        return NULL;
    }
    if (!fb || !fb->schema || !*fb->schema) {
        return strdup(sql_name);
    }
    size_t prefix_len = strlen(fb->schema);
    if (strncmp(sql_name, fb->schema, prefix_len) == 0 && sql_name[prefix_len] == '_') {
        return strdup(sql_name);
    }
    size_t n = prefix_len + 1 + strlen(sql_name) + 1;
    char* out = malloc(n);
    if (!out) {
        return NULL;
    }
    snprintf(out, n, "%s_%s", fb->schema, sql_name);
    return out;
}

json_t* firebase_firestore_string_field(const char* value) {
    json_t* field = json_object();
    if (!field) {
        return NULL;
    }
    if (json_object_set_new(field, "stringValue", json_string(value ? value : "")) != 0) {
        json_decref(field);
        return NULL;
    }
    return field;
}

json_t* firebase_catalog_load(const char* body) {
    if (!body || !*body) {
        return NULL;
    }
    json_error_t err;
    return json_loads(body, 0, &err);
}

char* firebase_catalog_get_field(json_t* doc, const char* field) {
    if (!doc || !field) {
        return NULL;
    }
    json_t* fields = json_object_get(doc, "fields");
    json_t* node = json_object_get(fields, field);
    json_t* sv = json_object_get(node, "stringValue");
    const char* text = json_string_value(sv);
    return text ? strdup(text) : NULL;
}

bool firebase_catalog_set_field(json_t* doc, const char* field, const char* json_text) {
    if (!doc || !field) {
        return false;
    }
    json_t* fields = json_object_get(doc, "fields");
    if (!json_is_object(fields)) {
        fields = json_object();
        if (!fields || json_object_set_new(doc, "fields", fields) != 0) {
            json_decref(fields);
            return false;
        }
    }
    json_t* node = firebase_firestore_string_field(json_text);
    if (!node) {
        return false;
    }
    if (json_object_set_new(fields, field, node) != 0) {
        json_decref(node);
        return false;
    }
    return true;
}

char* firebase_catalog_dump(json_t* doc) {
    if (!doc) {
        return NULL;
    }
    return json_dumps(doc, JSON_COMPACT);
}

char* firebase_catalog_indexes_json_from_body(const char* body) {
    json_t* doc = firebase_catalog_load(body);
    if (!doc) {
        return strdup("[]");
    }
    char* indexes = firebase_catalog_get_field(doc, "indexes");
    json_decref(doc);
    if (!indexes) {
        return strdup("[]");
    }
    return indexes;
}

bool firebase_catalog_has_documents(const char* list_body) {
    if (!list_body || !*list_body) {
        return false;
    }
    json_t* root = firebase_catalog_load(list_body);
    if (!root) {
        return false;
    }
    json_t* docs = json_object_get(root, "documents");
    bool has = json_is_array(docs) && json_array_size(docs) > 0;
    json_decref(root);
    return has;
}

char* firebase_sql_key_to_json(const FirebaseSqlKey* key) {
    json_t* arr = json_array();
    if (!arr) {
        return NULL;
    }
    if (key) {
        for (size_t i = 0; i < key->count; i++) {
            json_array_append_new(arr, json_string(key->columns[i] ? key->columns[i] : ""));
        }
    }
    char* dumped = json_dumps(arr, JSON_COMPACT);
    json_decref(arr);
    return dumped;
}

char* firebase_catalog_document_json(const FirebaseSqlTable* table, const char* collection,
                                     const char* indexes_json) {
    if (!table || !collection) {
        return NULL;
    }

    json_t* columns = json_array();
    if (!columns) {
        return NULL;
    }
    for (size_t i = 0; i < table->column_count; i++) {
        const FirebaseSqlColumn* col = &table->columns[i];
        json_t* obj = json_object();
        json_object_set_new(obj, "name", json_string(col->name ? col->name : ""));
        json_object_set_new(obj, "type", json_string(col->type ? col->type : "text"));
        json_object_set_new(obj, "not_null", col->not_null ? json_true() : json_false());
        if (col->default_sql) {
            json_object_set_new(obj, "default_sql", json_string(col->default_sql));
        }
        json_array_append_new(columns, obj);
    }
    char* columns_s = json_dumps(columns, JSON_COMPACT);
    json_decref(columns);

    char* pk_s = firebase_sql_key_to_json(&table->primary_key);

    json_t* uniques = json_array();
    for (size_t i = 0; i < table->unique_count; i++) {
        json_t* one = json_array();
        for (size_t j = 0; j < table->unique_keys[i].count; j++) {
            json_array_append_new(one, json_string(table->unique_keys[i].columns[j]));
        }
        json_array_append_new(uniques, one);
    }
    char* uniques_s = json_dumps(uniques, JSON_COMPACT);
    json_decref(uniques);

    json_t* fields = json_object();
    json_object_set_new(fields, "name", firebase_firestore_string_field(collection));
    json_object_set_new(fields, "columns", firebase_firestore_string_field(columns_s));
    json_object_set_new(fields, "primary_key", firebase_firestore_string_field(pk_s));
    json_object_set_new(fields, "unique_keys", firebase_firestore_string_field(uniques_s));
    json_object_set_new(fields, "indexes",
                        firebase_firestore_string_field(indexes_json ? indexes_json : "[]"));

    json_t* doc = json_object();
    json_object_set_new(doc, "fields", fields);
    char* out = json_dumps(doc, JSON_COMPACT);
    json_decref(doc);
    free(columns_s);
    free(pk_s);
    free(uniques_s);
    return out;
}

bool firebase_ddl_fail(QueryResult** result, const char* message) {
    if (result) {
        *result = firebase_query_result_error(message);
    }
    return false;
}

bool firebase_ddl_ok(QueryResult** result, int affected) {
    if (result) {
        *result = firebase_query_result_ok(affected);
    }
    return true;
}

const char* firebase_ddl_doc_id_from_name(const char* resource_name) {
    if (!resource_name) {
        return NULL;
    }
    const char* slash = strrchr(resource_name, '/');
    if (!slash || !slash[1]) {
        return NULL;
    }
    return slash + 1;
}

bool firebase_ddl_delete_collection_docs(FirebaseConnection* fb, const char* collection,
                                         QueryResult** result) {
    char* list_url = firebase_http_build_collection_url(fb->base_url, collection);
    if (!list_url) {
        return firebase_ddl_fail(result, "DROP TABLE: out of memory");
    }
    FirebaseHttpResponse* list = firebase_ddl_http_get(fb, list_url);
    free(list_url);
    if (!list) {
        return firebase_ddl_fail(result, "DROP TABLE: list failed");
    }
    if (list->http_status == 404) {
        firebase_http_response_free(list);
        return true;
    }
    if (!firebase_http_status_is_healthy(list->http_status)) {
        firebase_http_response_free(list);
        return firebase_ddl_fail(result, "DROP TABLE: failed to list collection");
    }
    json_t* root = firebase_catalog_load(list->body);
    firebase_http_response_free(list);
    json_t* docs = root ? json_object_get(root, "documents") : NULL;
    if (json_is_array(docs)) {
        size_t n = json_array_size(docs);
        for (size_t i = 0; i < n; i++) {
            json_t* doc = json_array_get(docs, i);
            const char* name = json_string_value(json_object_get(doc, "name"));
            const char* id = firebase_ddl_doc_id_from_name(name);
            if (!id) {
                continue;
            }
            char* doc_url = firebase_http_build_document_url(fb->base_url, collection, id);
            if (!doc_url) {
                json_decref(root);
                return firebase_ddl_fail(result, "DROP TABLE: out of memory");
            }
            FirebaseHttpResponse* del = firebase_ddl_http_delete(fb, doc_url);
            free(doc_url);
            if (!del || (!firebase_http_status_is_healthy(del->http_status) &&
                         del->http_status != 404)) {
                firebase_http_response_free(del);
                json_decref(root);
                return firebase_ddl_fail(result, "DROP TABLE: failed to delete row");
            }
            firebase_http_response_free(del);
        }
    }
    json_decref(root);
    return true;
}

bool firebase_ddl_copy_collection_docs(FirebaseConnection* fb, const char* from_collection,
                                       const char* to_collection, QueryResult** result) {
    char* list_url = firebase_http_build_collection_url(fb->base_url, from_collection);
    if (!list_url) {
        return firebase_ddl_fail(result, "ALTER TABLE RENAME: out of memory");
    }
    FirebaseHttpResponse* list = firebase_ddl_http_get(fb, list_url);
    free(list_url);
    if (!list) {
        return firebase_ddl_fail(result, "ALTER TABLE RENAME: list failed");
    }
    if (list->http_status == 404) {
        firebase_http_response_free(list);
        return true;
    }
    if (!firebase_http_status_is_healthy(list->http_status)) {
        firebase_http_response_free(list);
        return firebase_ddl_fail(result, "ALTER TABLE RENAME: failed to list collection");
    }
    json_t* root = firebase_catalog_load(list->body);
    firebase_http_response_free(list);
    json_t* docs = root ? json_object_get(root, "documents") : NULL;
    if (!json_is_array(docs)) {
        json_decref(root);
        return true;
    }
    size_t n = json_array_size(docs);
    for (size_t i = 0; i < n; i++) {
        json_t* doc = json_array_get(docs, i);
        const char* name = json_string_value(json_object_get(doc, "name"));
        const char* id = firebase_ddl_doc_id_from_name(name);
        json_t* fields = json_object_get(doc, "fields");
        if (!id || !fields) {
            continue;
        }
        json_t* payload = json_object();
        json_object_set(payload, "fields", fields);
        char* body = json_dumps(payload, JSON_COMPACT);
        json_decref(payload);
        char* dest = firebase_http_build_document_url(fb->base_url, to_collection, id);
        if (!body || !dest) {
            free(body);
            free(dest);
            json_decref(root);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: out of memory");
        }
        FirebaseHttpResponse* patch = firebase_ddl_http_patch(fb, dest, body);
        free(body);
        free(dest);
        if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
            firebase_http_response_free(patch);
            json_decref(root);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: copy failed");
        }
        firebase_http_response_free(patch);
    }
    json_decref(root);
    return firebase_ddl_delete_collection_docs(fb, from_collection, result);
}

bool firebase_ddl_create_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                               QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->table.name) {
        return firebase_ddl_fail(result, "CREATE TABLE: invalid connection");
    }
    char* collection = firebase_resolve_collection_name(fb, stmt->table.name);
    char* schema_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                        collection);
    if (!collection || !schema_url) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE TABLE: out of memory");
    }
    FirebaseHttpResponse* existing = firebase_ddl_http_get(fb, schema_url);
    if (!existing) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE TABLE: catalog GET failed");
    }
    if (firebase_http_status_is_healthy(existing->http_status)) {
        firebase_http_response_free(existing);
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE TABLE: table already exists");
    }
    firebase_http_response_free(existing);

    char* body = firebase_catalog_document_json(&stmt->table, collection, "[]");
    if (!body) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE TABLE: failed to build catalog");
    }
    FirebaseHttpResponse* patch = firebase_ddl_http_patch(fb, schema_url, body);
    free(body);
    free(schema_url);
    free(collection);
    if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
        firebase_http_response_free(patch);
        return firebase_ddl_fail(result, "CREATE TABLE: catalog PATCH failed");
    }
    firebase_http_response_free(patch);
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_drop_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                             QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->ident) {
        return firebase_ddl_fail(result, "DROP TABLE: invalid connection");
    }
    char* collection = firebase_resolve_collection_name(fb, stmt->ident);
    char* schema_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                        collection);
    if (!collection || !schema_url) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "DROP TABLE: out of memory");
    }
    FirebaseHttpResponse* existing = firebase_ddl_http_get(fb, schema_url);
    if (!existing) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "DROP TABLE: catalog GET failed");
    }
    if (existing->http_status == 404) {
        firebase_http_response_free(existing);
        free(collection);
        free(schema_url);
        if (stmt->if_exists) {
            return firebase_ddl_ok(result, 0);
        }
        return firebase_ddl_fail(result, "DROP TABLE: table does not exist");
    }
    if (!firebase_http_status_is_healthy(existing->http_status)) {
        firebase_http_response_free(existing);
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "DROP TABLE: catalog GET failed");
    }
    firebase_http_response_free(existing);

    if (!firebase_ddl_delete_collection_docs(fb, collection, result)) {
        free(collection);
        free(schema_url);
        return false;
    }
    FirebaseHttpResponse* del = firebase_ddl_http_delete(fb, schema_url);
    free(schema_url);
    free(collection);
    if (!del || (!firebase_http_status_is_healthy(del->http_status) && del->http_status != 404)) {
        firebase_http_response_free(del);
        return firebase_ddl_fail(result, "DROP TABLE: catalog DELETE failed");
    }
    firebase_http_response_free(del);
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_create_index(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                               QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->index.table || !stmt->index.name) {
        return firebase_ddl_fail(result, "CREATE INDEX: invalid connection");
    }
    char* collection = firebase_resolve_collection_name(fb, stmt->index.table);
    char* schema_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                        collection);
    if (!collection || !schema_url) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE INDEX: out of memory");
    }
    FirebaseHttpResponse* existing = firebase_ddl_http_get(fb, schema_url);
    if (!existing || !firebase_http_status_is_healthy(existing->http_status)) {
        firebase_http_response_free(existing);
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "CREATE INDEX: table does not exist");
    }
    json_t* doc = firebase_catalog_load(existing->body);
    char* indexes_s = firebase_catalog_indexes_json_from_body(existing->body);
    firebase_http_response_free(existing);
    json_t* indexes = indexes_s ? json_loads(indexes_s, 0, NULL) : json_array();
    free(indexes_s);
    if (!json_is_array(indexes)) {
        json_decref(indexes);
        indexes = json_array();
    }
    json_t* idx = json_object();
    json_object_set_new(idx, "name", json_string(stmt->index.name));
    json_object_set_new(idx, "unique", stmt->index.unique ? json_true() : json_false());
    json_t* cols = json_array();
    for (size_t i = 0; i < stmt->index.columns.count; i++) {
        json_array_append_new(cols, json_string(stmt->index.columns.columns[i]));
    }
    json_object_set_new(idx, "columns", cols);
    json_array_append_new(indexes, idx);
    char* new_indexes = json_dumps(indexes, JSON_COMPACT);
    json_decref(indexes);
    firebase_catalog_set_field(doc, "indexes", new_indexes);
    free(new_indexes);
    char* body = firebase_catalog_dump(doc);
    json_decref(doc);
    FirebaseHttpResponse* patch = firebase_ddl_http_patch(fb, schema_url, body);
    free(body);
    free(schema_url);
    free(collection);
    if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
        firebase_http_response_free(patch);
        return firebase_ddl_fail(result, "CREATE INDEX: catalog PATCH failed");
    }
    firebase_http_response_free(patch);
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_alter_add_column(FirebaseConnection* fb, json_t* doc,
                                   const FirebaseSqlColumn* col, QueryResult** result) {
    char* columns_s = firebase_catalog_get_field(doc, "columns");
    json_t* columns = columns_s ? json_loads(columns_s, 0, NULL) : json_array();
    free(columns_s);
    if (!json_is_array(columns)) {
        json_decref(columns);
        return firebase_ddl_fail(result, "ALTER TABLE: invalid columns catalog");
    }
    size_t n = json_array_size(columns);
    for (size_t i = 0; i < n; i++) {
        json_t* existing = json_array_get(columns, i);
        const char* name = json_string_value(json_object_get(existing, "name"));
        if (name && col->name && strcmp(name, col->name) == 0) {
            json_decref(columns);
            return firebase_ddl_fail(result, "ALTER TABLE: column already exists");
        }
    }
    json_t* obj = json_object();
    json_object_set_new(obj, "name", json_string(col->name));
    json_object_set_new(obj, "type", json_string(col->type ? col->type : "text"));
    json_object_set_new(obj, "not_null", col->not_null ? json_true() : json_false());
    if (col->default_sql) {
        json_object_set_new(obj, "default_sql", json_string(col->default_sql));
    }
    json_array_append_new(columns, obj);
    char* dumped = json_dumps(columns, JSON_COMPACT);
    json_decref(columns);
    firebase_catalog_set_field(doc, "columns", dumped);
    free(dumped);
    (void)fb;
    return true;
}

bool firebase_ddl_alter_drop_column(json_t* doc, const char* column, QueryResult** result) {
    char* columns_s = firebase_catalog_get_field(doc, "columns");
    json_t* columns = columns_s ? json_loads(columns_s, 0, NULL) : NULL;
    free(columns_s);
    if (!json_is_array(columns)) {
        json_decref(columns);
        return firebase_ddl_fail(result, "ALTER TABLE: invalid columns catalog");
    }
    json_t* kept = json_array();
    bool found = false;
    size_t n = json_array_size(columns);
    for (size_t i = 0; i < n; i++) {
        json_t* existing = json_array_get(columns, i);
        const char* name = json_string_value(json_object_get(existing, "name"));
        if (name && column && strcmp(name, column) == 0) {
            found = true;
            continue;
        }
        json_array_append(kept, existing);
    }
    json_decref(columns);
    if (!found) {
        json_decref(kept);
        return firebase_ddl_fail(result, "ALTER TABLE: column does not exist");
    }
    char* dumped = json_dumps(kept, JSON_COMPACT);
    json_decref(kept);
    firebase_catalog_set_field(doc, "columns", dumped);
    free(dumped);
    return true;
}

bool firebase_ddl_alter_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                              QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt || !stmt->alter.table) {
        return firebase_ddl_fail(result, "ALTER TABLE: invalid connection");
    }

    if (stmt->alter.op == FIREBASE_ALTER_RENAME_TABLE) {
        char* old_name = firebase_resolve_collection_name(fb, stmt->alter.table);
        char* new_name = firebase_resolve_collection_name(fb, stmt->alter.rename_to);
        char* old_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                         old_name);
        char* new_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                         new_name);
        if (!old_name || !new_name || !old_url || !new_url) {
            free(old_name);
            free(new_name);
            free(old_url);
            free(new_url);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: out of memory");
        }
        FirebaseHttpResponse* old_doc = firebase_ddl_http_get(fb, old_url);
        if (!old_doc || !firebase_http_status_is_healthy(old_doc->http_status)) {
            firebase_http_response_free(old_doc);
            free(old_name);
            free(new_name);
            free(old_url);
            free(new_url);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: table does not exist");
        }
        FirebaseHttpResponse* new_doc = firebase_ddl_http_get(fb, new_url);
        if (new_doc && firebase_http_status_is_healthy(new_doc->http_status)) {
            firebase_http_response_free(old_doc);
            firebase_http_response_free(new_doc);
            free(old_name);
            free(new_name);
            free(old_url);
            free(new_url);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: target already exists");
        }
        firebase_http_response_free(new_doc);
        json_t* catalog = firebase_catalog_load(old_doc->body);
        firebase_http_response_free(old_doc);
        firebase_catalog_set_field(catalog, "name", new_name);
        char* body = firebase_catalog_dump(catalog);
        json_decref(catalog);
        FirebaseHttpResponse* patch = firebase_ddl_http_patch(fb, new_url, body);
        free(body);
        if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
            firebase_http_response_free(patch);
            free(old_name);
            free(new_name);
            free(old_url);
            free(new_url);
            return firebase_ddl_fail(result, "ALTER TABLE RENAME: catalog PATCH failed");
        }
        firebase_http_response_free(patch);
        if (!firebase_ddl_copy_collection_docs(fb, old_name, new_name, result)) {
            free(old_name);
            free(new_name);
            free(old_url);
            free(new_url);
            return false;
        }
        FirebaseHttpResponse* del = firebase_ddl_http_delete(fb, old_url);
        firebase_http_response_free(del);
        free(old_name);
        free(new_name);
        free(old_url);
        free(new_url);
        return firebase_ddl_ok(result, 0);
    }

    char* collection = firebase_resolve_collection_name(fb, stmt->alter.table);
    char* schema_url = firebase_http_build_document_url(fb->base_url, FIREBASE_SCHEMA_COLLECTION,
                                                        collection);
    if (!collection || !schema_url) {
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "ALTER TABLE: out of memory");
    }
    FirebaseHttpResponse* existing = firebase_ddl_http_get(fb, schema_url);
    if (!existing || !firebase_http_status_is_healthy(existing->http_status)) {
        firebase_http_response_free(existing);
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "ALTER TABLE: table does not exist");
    }
    json_t* doc = firebase_catalog_load(existing->body);
    firebase_http_response_free(existing);
    bool changed = false;
    if (stmt->alter.op == FIREBASE_ALTER_ADD_COLUMN) {
        changed = firebase_ddl_alter_add_column(fb, doc, &stmt->alter.add_column, result);
    } else if (stmt->alter.op == FIREBASE_ALTER_DROP_COLUMN) {
        changed = firebase_ddl_alter_drop_column(doc, stmt->alter.drop_column, result);
    } else {
        json_decref(doc);
        free(collection);
        free(schema_url);
        return firebase_ddl_fail(result, "ALTER TABLE: unsupported action");
    }
    if (!changed) {
        json_decref(doc);
        free(collection);
        free(schema_url);
        return false;
    }
    char* body = firebase_catalog_dump(doc);
    json_decref(doc);
    FirebaseHttpResponse* patch = firebase_ddl_http_patch(fb, schema_url, body);
    free(body);
    free(schema_url);
    free(collection);
    if (!patch || !firebase_http_status_is_healthy(patch->http_status)) {
        firebase_http_response_free(patch);
        return firebase_ddl_fail(result, "ALTER TABLE: catalog PATCH failed");
    }
    firebase_http_response_free(patch);
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_create_function(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                  QueryResult** result) {
    (void)connection;
    if (!stmt || !stmt->ident) {
        return firebase_ddl_fail(result, "CREATE FUNCTION: missing name");
    }
    if (!firebase_sql_is_known_function(stmt->ident)) {
        return firebase_ddl_fail(result, "CREATE FUNCTION: unknown function");
    }
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_drop_function(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                QueryResult** result) {
    (void)connection;
    if (!stmt || !stmt->ident) {
        return firebase_ddl_fail(result, "DROP FUNCTION: missing name");
    }
    if (firebase_sql_is_known_function(stmt->ident) || stmt->if_exists) {
        return firebase_ddl_ok(result, 0);
    }
    return firebase_ddl_fail(result, "DROP FUNCTION: unknown function");
}

bool firebase_ddl_refuse_drop(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                              QueryResult** result) {
    FirebaseConnection* fb = firebase_ddl_connection(connection);
    if (!fb || !stmt) {
        return firebase_ddl_fail(result, "DROP_CHECK: invalid connection");
    }
    const char* raw = stmt->exists_table ? stmt->exists_table : stmt->refuse_table;
    char* collection = firebase_resolve_collection_name(fb, raw);
    if (!collection) {
        return firebase_ddl_fail(result, "DROP_CHECK: missing table");
    }
    char* list_url = firebase_http_build_collection_url(fb->base_url, collection);
    if (!list_url) {
        free(collection);
        return firebase_ddl_fail(result, "DROP_CHECK: out of memory");
    }
    FirebaseHttpResponse* list = firebase_ddl_http_get(fb, list_url);
    free(list_url);
    bool has = list && firebase_http_status_is_healthy(list->http_status) &&
               firebase_catalog_has_documents(list->body);
    firebase_http_response_free(list);
    if (has) {
        char message[256];
        const char* shown = stmt->refuse_table ? stmt->refuse_table : collection;
        snprintf(message, sizeof(message),
                 "Refusing to drop table %s – it contains data", shown);
        free(collection);
        return firebase_ddl_fail(result, message);
    }
    free(collection);
    return firebase_ddl_ok(result, 0);
}

bool firebase_ddl_execute(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                          QueryResult** result) {
    if (!stmt) {
        return firebase_ddl_fail(result, "DDL: NULL statement");
    }
    switch (stmt->kind) {
        case FIREBASE_SQL_KIND_CREATE_TABLE:
            return firebase_ddl_create_table(connection, stmt, result);
        case FIREBASE_SQL_KIND_DROP_TABLE:
            return firebase_ddl_drop_table(connection, stmt, result);
        case FIREBASE_SQL_KIND_CREATE_INDEX:
            return firebase_ddl_create_index(connection, stmt, result);
        case FIREBASE_SQL_KIND_ALTER_TABLE:
            return firebase_ddl_alter_table(connection, stmt, result);
        case FIREBASE_SQL_KIND_CREATE_FUNCTION:
            return firebase_ddl_create_function(connection, stmt, result);
        case FIREBASE_SQL_KIND_DROP_FUNCTION:
            return firebase_ddl_drop_function(connection, stmt, result);
        case FIREBASE_SQL_KIND_REFUSE_DROP:
            return firebase_ddl_refuse_drop(connection, stmt, result);
        case FIREBASE_SQL_KIND_INSERT:
        case FIREBASE_SQL_KIND_UPDATE:
        case FIREBASE_SQL_KIND_DELETE:
        case FIREBASE_SQL_KIND_NONE:
        case FIREBASE_SQL_KIND_UNSUPPORTED:
            return firebase_ddl_fail(result, "Firebase SQL interpreter is not implemented yet");
    }
    return firebase_ddl_fail(result, "Firebase SQL interpreter is not implemented yet");
}
