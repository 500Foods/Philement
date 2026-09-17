/*
 * Firebase DDL executor: catalog collection _schema plus data collections.
 */

#ifndef DATABASE_ENGINE_FIREBASE_SQL_DDL_H
#define DATABASE_ENGINE_FIREBASE_SQL_DDL_H

#include <src/database/database.h>
#include <jansson.h>
#include "types.h"
#include "http.h"
#include "sql_parse.h"

char* firebase_resolve_collection_name(const FirebaseConnection* fb, const char* sql_name);
char* firebase_catalog_document_json(const FirebaseSqlTable* table, const char* collection,
                                     const char* indexes_json);
char* firebase_catalog_indexes_json_from_body(const char* body);
bool firebase_catalog_has_documents(const char* list_body);
json_t* firebase_firestore_string_field(const char* value);
json_t* firebase_catalog_load(const char* body);
char* firebase_catalog_get_field(json_t* doc, const char* field);
bool firebase_catalog_set_field(json_t* doc, const char* field, const char* json_text);
char* firebase_catalog_dump(json_t* doc);
FirebaseConnection* firebase_ddl_connection(DatabaseHandle* connection);
FirebaseHttpResponse* firebase_ddl_http_get(FirebaseConnection* fb, const char* url);
FirebaseHttpResponse* firebase_ddl_http_patch(FirebaseConnection* fb, const char* url,
                                              const char* body);
FirebaseHttpResponse* firebase_ddl_http_delete(FirebaseConnection* fb, const char* url);
bool firebase_ddl_delete_collection_docs(FirebaseConnection* fb, const char* collection,
                                         QueryResult** result);
bool firebase_ddl_copy_collection_docs(FirebaseConnection* fb, const char* from_collection,
                                       const char* to_collection, QueryResult** result);
char* firebase_sql_key_to_json(const FirebaseSqlKey* key);
bool firebase_ddl_fail(QueryResult** result, const char* message);
bool firebase_ddl_ok(QueryResult** result, int affected);
const char* firebase_ddl_doc_id_from_name(const char* resource_name);
bool firebase_ddl_alter_add_column(FirebaseConnection* fb, json_t* doc,
                                   const FirebaseSqlColumn* col, QueryResult** result);
bool firebase_ddl_alter_drop_column(json_t* doc, const char* column, QueryResult** result);

bool firebase_ddl_execute(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                          QueryResult** result);
bool firebase_ddl_create_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                               QueryResult** result);
bool firebase_ddl_drop_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                             QueryResult** result);
bool firebase_ddl_create_index(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                               QueryResult** result);
bool firebase_ddl_alter_table(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                              QueryResult** result);
bool firebase_ddl_create_function(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                  QueryResult** result);
bool firebase_ddl_drop_function(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                QueryResult** result);
bool firebase_ddl_refuse_drop(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                              QueryResult** result);

#endif /* DATABASE_ENGINE_FIREBASE_SQL_DDL_H */
