/*
 * DB2 Database Engine - Query Execution Header
 *
 * Header file for DB2 query execution functions.
 */

#ifndef DATABASE_ENGINE_DB2_QUERY_H
#define DATABASE_ENGINE_DB2_QUERY_H

#include "../database.h"

// Query execution
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

#endif // DATABASE_ENGINE_DB2_QUERY_H