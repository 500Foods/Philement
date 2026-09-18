/*
 * Firebase INSERT…SELECT / CTE / MAX+1 / RETURNING (Phase 7).
 */

#ifndef DATABASE_ENGINE_FIREBASE_SQL_SELECT_H
#define DATABASE_ENGINE_FIREBASE_SQL_SELECT_H

#include <src/database/database.h>
#include <jansson.h>
#include "types.h"
#include "sql_parse.h"
#include "sql_expr.h"

bool firebase_sql_parse_insert_source(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_returning(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_select_list(const char** cursor, FirebaseSqlSelect* sel,
                                    FirebaseSqlStatement* stmt);

bool firebase_dml_insert_select(DatabaseHandle* connection, const FirebaseSqlStatement* stmt,
                                QueryResult** result);
bool firebase_select_eval_expr(const FirebaseExpr* expr, json_t* row_fields, json_t* documents,
                               FirebaseValue* out, char** error);
bool firebase_select_eval_max(json_t* documents, const char* column, FirebaseValue* out);
bool firebase_select_set_returning(QueryResult** result, int affected, json_t* fields,
                                   char** columns, size_t column_count);
json_t* firebase_select_value_to_json(const FirebaseValue* value);
json_t* firebase_select_field_to_json(json_t* field);
bool firebase_select_commit_row(FirebaseConnection* fb, const char* collection, json_t* pk,
                                json_t* uniques, json_t* existing, json_t* written,
                                json_t* fields, json_t* id_parts, QueryResult** result);
bool firebase_select_fill_defaults(json_t* columns, json_t* fields, json_t* id_parts,
                                   QueryResult** result);
bool firebase_dml_insert_select_star(FirebaseConnection* fb, const FirebaseSqlStatement* stmt,
                                     QueryResult** result);

#endif /* DATABASE_ENGINE_FIREBASE_SQL_SELECT_H */
