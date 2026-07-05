/*
 * SQLite Database Engine - Query Execution Header
 *
 * Header file for SQLite query execution functions.
 */

#ifndef SQLITE_QUERY_H
#define SQLITE_QUERY_H

#include <src/database/database.h>

// Query execution
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Callback function for sqlite3_exec
int sqlite_exec_callback(void* data, int argc, char** argv, char** col_names);

// Build a populated failure QueryResult (error_class + error_message) from a
// SQLite step result code so failures are classified correctly by the engine
// retry layer instead of being treated as a retryable NULL/transport error.
QueryResult* sqlite_build_error_result(int step_result, const char* error_msg);

#endif // SQLITE_QUERY_H