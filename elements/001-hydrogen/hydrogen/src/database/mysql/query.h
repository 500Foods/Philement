/*
 * MySQL Database Engine - Query Execution Header
 *
 * Header file for MySQL query execution functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_QUERY_H
#define DATABASE_ENGINE_MYSQL_QUERY_H

#include "../database.h"

// Query execution
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

#endif // DATABASE_ENGINE_MYSQL_QUERY_H