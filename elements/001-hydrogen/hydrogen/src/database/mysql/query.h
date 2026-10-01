/*
 * MySQL Database Engine - Query Execution Header
 *
 * Header file for MySQL query execution functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_QUERY_H
#define DATABASE_ENGINE_MYSQL_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

// Helper functions
void mysql_cleanup_column_names(char** column_names, size_t column_count);

// Query execution
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 * -------------------------------------------------------------------------- */
void mysql_cleanup_bound_values(void** bound_values, size_t count);
/*
 * Oracle MySQL rejects INSERT ... RETURNING. When the statement ends with
 * RETURNING <column> and selects new_<column>, wrap that selected token in
 * LAST_INSERT_ID() and drop the RETURNING clause. Returns NULL when the
 * statement is left unchanged. column_name receives the RETURNING column.
 */
char* mysql_rewrite_insert_returning(const char* sql, char* column_name, size_t column_name_len);
void mysql_bind_attach_indicators(void* bind, unsigned int param_index, char is_null_flag);
bool mysql_bind_single_parameter(void* bind, unsigned int param_index, TypedParameter* param,
                                 void** bound_values, size_t total_param_count, const char* designator);

#endif // DATABASE_ENGINE_MYSQL_QUERY_H