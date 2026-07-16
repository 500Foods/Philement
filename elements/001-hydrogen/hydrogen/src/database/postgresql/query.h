/*
 * PostgreSQL Database Engine - Query Execution Header
 *
 * Header file for PostgreSQL query execution functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_QUERY_H
#define DATABASE_ENGINE_POSTGRESQL_QUERY_H

#include <stdbool.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include "connection.h"

// Function prototypes for query execution
bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool postgresql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Helper function for parameter conversion (exposed for testing)
char* postgresql_convert_param_value(const TypedParameter* param, const char* designator);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 * -------------------------------------------------------------------------- */
bool postgresql_is_numeric_type(int oid);
bool postgresql_is_datetime_type(int oid);
char* postgresql_format_timestamp_string(char* str);
char* postgresql_trim_trailing_whitespace(char* str);

#endif // DATABASE_ENGINE_POSTGRESQL_QUERY_H