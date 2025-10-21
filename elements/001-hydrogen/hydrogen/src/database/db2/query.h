/*
 * DB2 Database Engine - Query Execution Header
 *
 * Header file for DB2 query execution functions.
 */

#ifndef DATABASE_ENGINE_DB2_QUERY_H
#define DATABASE_ENGINE_DB2_QUERY_H

#include <src/database/database.h>

// Query execution
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Helper functions for processing query results (non-static for testing)
bool db2_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result);
char** db2_get_column_names(void* stmt_handle, int column_count);
bool db2_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                        char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row);
void db2_cleanup_column_names(char** column_names, int column_count);

#endif // DATABASE_ENGINE_DB2_QUERY_H