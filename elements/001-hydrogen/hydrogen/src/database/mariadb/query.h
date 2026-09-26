/*
 * MariaDB Database Engine - Query Execution Header
 *
 * Header file for MariaDB query execution functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_QUERY_H
#define DATABASE_ENGINE_MARIADB_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

// Helper functions
void mariadb_cleanup_column_names(char** column_names, size_t column_count);

// Query execution
bool mariadb_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mariadb_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 * -------------------------------------------------------------------------- */
void mariadb_cleanup_bound_values(void** bound_values, size_t count);
void mariadb_bind_attach_indicators(void* bind, unsigned int param_index, char is_null_flag);
bool mariadb_bind_single_parameter(void* bind, unsigned int param_index, TypedParameter* param,
                                 void** bound_values, size_t total_param_count, const char* designator);

#endif // DATABASE_ENGINE_MARIADB_QUERY_H
