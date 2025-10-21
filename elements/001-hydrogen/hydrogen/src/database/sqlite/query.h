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

#endif // SQLITE_QUERY_H