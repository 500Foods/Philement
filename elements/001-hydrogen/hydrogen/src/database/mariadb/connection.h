/*
 * MariaDB Database Engine - Connection Management Header
 *
 * Header file for MariaDB connection management functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_CONNECTION_H
#define DATABASE_ENGINE_MARIADB_CONNECTION_H

#include <src/database/database.h>
#include "types.h"

// Utility functions
bool mariadb_check_timeout_expired(time_t start_time, int timeout_seconds);

// Connection management
bool mariadb_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mariadb_disconnect(DatabaseHandle* connection);
bool mariadb_health_check(DatabaseHandle* connection);
bool mariadb_h_reset_connection(DatabaseHandle* connection);

// Watchdog cancel hook - implements engine cancel_inflight
void mariadb_cancel_inflight(DatabaseHandle* connection);

// Library loading
bool load_libmariadb_functions(const char* designator);

// Utility functions for prepared statement cache
PreparedStatementCache* mariadb_create_prepared_statement_cache(void);
void mariadb_destroy_prepared_statement_cache(PreparedStatementCache* cache);

#endif // DATABASE_ENGINE_MARIADB_CONNECTION_H
