/*
 * MSSQL Database Engine - Query Execution Header
 *
 * Header file for MSSQL query execution functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_QUERY_H
#define DATABASE_ENGINE_MSSQL_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

// Query execution
bool mssql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mssql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Result rows are assembled in query_result.c (non-static for testing)
bool mssql_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result);
char** mssql_get_column_names(void* stmt_handle, int column_count);
bool mssql_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                          char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row);
void mssql_cleanup_column_names(char** column_names, int column_count);
bool mssql_bind_single_parameter(void* stmt_handle, unsigned short param_index, TypedParameter* param,
                                 void** bound_values, long* str_len_indicators, const char* designator);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 * -------------------------------------------------------------------------- */
char* mssql_trim_trailing_whitespace(char* str);
char* mssql_format_datetime_string(char* str);
char* mssql_format_timestamp_string(char* str);
bool mssql_normalize_iso8601_timestamp(const char* input, char* output, size_t output_size);
void mssql_complete_standalone_statement(DatabaseHandle* connection, bool success);
void mssql_cleanup_bound_values(void** bound_values, size_t count);

#endif // DATABASE_ENGINE_MSSQL_QUERY_H
