/*
 * Firebase DML: INSERT VALUES, UPDATE, DELETE with FB_* expression eval.
 */

#ifndef DATABASE_ENGINE_FIREBASE_SQL_DML_H
#define DATABASE_ENGINE_FIREBASE_SQL_DML_H

#include <src/database/database.h>
#include <jansson.h>
#include "types.h"
#include "http.h"
#include "sql_parse.h"
#include "sql_expr.h"

bool firebase_dml_execute(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                          QueryResult** result);
bool firebase_dml_insert(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result);
bool firebase_dml_update(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result);
bool firebase_dml_delete(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                         QueryResult** result);

bool firebase_dml_fail(QueryResult** result, const char* message);
json_t* firebase_dml_load_catalog(FirebaseConnection* fb, const char* collection,
                                  QueryResult** result);
char* firebase_dml_document_id(json_t* pk_names, json_t* values_by_column);
json_t* firebase_dml_value_to_field(const FirebaseValue* value, const char* sql_type);
char* firebase_dml_sql_datetime_to_rfc3339(const char* sql_dt);
bool firebase_dml_lookup_field(const char* name, FirebaseValue* out, void* userdata);
bool firebase_dml_firestore_field_to_value(json_t* field, FirebaseValue* out);
bool firebase_dml_where_match(const FirebaseSqlWhere* where, json_t* fields, char** error);
json_t* firebase_dml_list_documents(FirebaseConnection* fb, const char* collection,
                                    QueryResult** result);
bool firebase_sql_where_add(FirebaseSqlWhere* where, FirebaseSqlPredicate pred);
bool firebase_dml_values_match(const FirebaseValue* left, const FirebaseValue* right);
char* firebase_dml_id_fragment(const FirebaseValue* value);
json_t* firebase_dml_catalog_json_field(json_t* catalog, const char* field);
const char* firebase_dml_column_type(json_t* columns, const char* name);
json_t* firebase_dml_find_column(json_t* columns, const char* name);
bool firebase_dml_unique_conflict(json_t* unique_keys, json_t* new_fields, json_t* existing_docs);

#endif /* DATABASE_ENGINE_FIREBASE_SQL_DML_H */
