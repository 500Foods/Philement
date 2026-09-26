/*
 * MariaDB Database Engine - Query Helper Functions
 *
 * Helper functions for MariaDB query processing, extracted for better testability
 * following the DB2 pattern of non-static helper functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_QUERY_HELPERS_H
#define DATABASE_ENGINE_MARIADB_QUERY_HELPERS_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations for database types
typedef struct DatabaseHandle DatabaseHandle;
typedef struct QueryRequest QueryRequest;
typedef struct QueryResult QueryResult;

// Forward declarations for helper functions (non-static for testing)
bool mariadb_is_numeric_type(unsigned int type);
char** mariadb_extract_column_names(void* mariadb_result, size_t column_count);
bool mariadb_build_json_from_result(void* mariadb_result, size_t row_count, size_t column_count,
                                   char** column_names, char** json_buffer);
size_t mariadb_calculate_json_buffer_size(size_t row_count, size_t column_count);
int mariadb_json_escape_string(const char* input, char* output, size_t output_size);

// Additional helper functions for query execution (non-static for testing)
bool mariadb_validate_query_parameters(const DatabaseHandle* connection, const QueryRequest* request, QueryResult** result);
bool mariadb_execute_query_statement(void* mariadb_connection, const char* sql_template, const char* designator);
void* mariadb_store_query_result(void* mariadb_connection, const char* designator);
bool mariadb_process_query_result(void* mariadb_result, QueryResult* db_result, const char* designator);
bool mariadb_process_prepared_result(void* mariadb_result, QueryResult* db_result, void* stmt_handle, const char* designator);

// New helper functions for prepared statement result processing
bool mariadb_process_prepared_stmt_result(void* stmt, QueryResult* result, const char* designator);
bool mariadb_process_direct_result(void* mariadb_conn, void* mariadb_result, QueryResult* result, const char* designator);

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * -------------------------------------------------------------------------- */
char* mariadb_trim_trailing_whitespace(char* str);

#endif // DATABASE_ENGINE_MARIADB_QUERY_HELPERS_H
