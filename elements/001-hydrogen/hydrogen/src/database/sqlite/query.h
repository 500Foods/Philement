/*
 * SQLite Database Engine - Query Execution Header
 *
 * Header file for SQLite query execution functions.
 */

#ifndef SQLITE_QUERY_H
#define SQLITE_QUERY_H

#include "../database.h"

// Query execution
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool sqlite_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

#endif // SQLITE_QUERY_H