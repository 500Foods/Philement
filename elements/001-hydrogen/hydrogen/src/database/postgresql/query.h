/*
 * PostgreSQL Database Engine - Query Execution Header
 *
 * Header file for PostgreSQL query execution functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_QUERY_H
#define DATABASE_ENGINE_POSTGRESQL_QUERY_H

#include <stdbool.h>

#include "../database.h"
#include "connection.h"

// Function prototypes for query execution
bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool postgresql_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

#endif // DATABASE_ENGINE_POSTGRESQL_QUERY_H