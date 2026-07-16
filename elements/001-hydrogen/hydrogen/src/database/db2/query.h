/*
 * DB2 Database Engine - Query Execution Header
 *
 * Header file for DB2 query execution functions.
 */

#ifndef DATABASE_ENGINE_DB2_QUERY_H
#define DATABASE_ENGINE_DB2_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

// Query execution
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Helper functions for processing query results (non-static for testing)
bool db2_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result);
char** db2_get_column_names(void* stmt_handle, int column_count);
bool db2_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                        char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row);
void db2_cleanup_column_names(char** column_names, int column_count);
bool db2_bind_single_parameter(void* stmt_handle, unsigned short param_index, TypedParameter* param,
                               void** bound_values, long* str_len_indicators, const char* designator);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 * -------------------------------------------------------------------------- */
char* db2_trim_trailing_whitespace(char* str);
char* db2_format_datetime_string(char* str);
char* db2_format_timestamp_string(char* str);
void db2_cleanup_bound_values(void** bound_values, size_t count);

#endif // DATABASE_ENGINE_DB2_QUERY_H