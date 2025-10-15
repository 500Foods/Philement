/*
 * MySQL Database Engine - Query Helper Functions
 *
 * Helper functions for MySQL query processing, extracted for better testability
 * following the DB2 pattern of non-static helper functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_QUERY_HELPERS_H
#define DATABASE_ENGINE_MYSQL_QUERY_HELPERS_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations for database types
typedef struct DatabaseHandle DatabaseHandle;
typedef struct QueryRequest QueryRequest;
typedef struct QueryResult QueryResult;

// Forward declarations for helper functions (non-static for testing)
char** mysql_extract_column_names(void* mysql_result, size_t column_count);
bool mysql_build_json_from_result(void* mysql_result, size_t row_count, size_t column_count,
                                   char** column_names, char** json_buffer);
size_t mysql_calculate_json_buffer_size(size_t row_count, size_t column_count);
int mysql_json_escape_string(const char* input, char* output, size_t output_size);

// Additional helper functions for query execution (non-static for testing)
bool mysql_validate_query_parameters(const DatabaseHandle* connection, const QueryRequest* request, QueryResult** result);
bool mysql_execute_query_statement(void* mysql_connection, const char* sql_template, const char* designator);
void* mysql_store_query_result(void* mysql_connection, const char* designator);
bool mysql_process_query_result(void* mysql_result, QueryResult* db_result, const char* designator);
bool mysql_process_prepared_result(void* mysql_result, QueryResult* db_result, void* stmt_handle, const char* designator);

#endif // DATABASE_ENGINE_MYSQL_QUERY_HELPERS_H