/*
 * SQLite Database Engine - Connection Management Header
 *
 * Header file for SQLite connection management functions.
 */

#ifndef SQLITE_CONNECTION_H
#define SQLITE_CONNECTION_H

#include "../database.h"

// Utility functions
bool sqlite_check_timeout_expired(time_t start_time, int timeout_seconds);

// Connection management
bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool sqlite_disconnect(DatabaseHandle* connection);
bool sqlite_health_check(DatabaseHandle* connection);
bool sqlite_reset_connection(DatabaseHandle* connection);

// Library loading
bool load_libsqlite_functions(const char* designator);

// Utility functions for prepared statement cache
PreparedStatementCache* sqlite_create_prepared_statement_cache(void);
void sqlite_destroy_prepared_statement_cache(PreparedStatementCache* cache);

#endif // SQLITE_CONNECTION_H