/*
 * MySQL Database Engine - Query Execution Header
 *
 * Header file for MySQL query execution functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_QUERY_H
#define DATABASE_ENGINE_MYSQL_QUERY_H

#include <src/database/database.h>

// Helper functions
void mysql_cleanup_column_names(char** column_names, size_t column_count);

// Query execution
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * -------------------------------------------------------------------------- */
void mysql_cleanup_bound_values(void** bound_values, size_t count);

#endif // DATABASE_ENGINE_MYSQL_QUERY_H