/*
 * MySQL Database Engine - Connection Management Header
 *
 * Header file for MySQL connection management functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_CONNECTION_H
#define DATABASE_ENGINE_MYSQL_CONNECTION_H

#include "../database.h"
#include "types.h"

// Connection management
bool mysql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mysql_disconnect(DatabaseHandle* connection);
bool mysql_health_check(DatabaseHandle* connection);
bool mysql_reset_connection(DatabaseHandle* connection);

// Library loading
bool load_libmysql_functions(void);

// Utility functions for prepared statement cache
PreparedStatementCache* mysql_create_prepared_statement_cache(void);
void mysql_destroy_prepared_statement_cache(PreparedStatementCache* cache);

#endif // DATABASE_ENGINE_MYSQL_CONNECTION_H