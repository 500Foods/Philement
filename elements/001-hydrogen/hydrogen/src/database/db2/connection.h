/*
 * DB2 Database Engine - Connection Management Header
 *
 * Header file for DB2 connection management functions.
 */

#ifndef DATABASE_ENGINE_DB2_CONNECTION_H
#define DATABASE_ENGINE_DB2_CONNECTION_H

#include <src/database/database.h>
#include "types.h"

// Utility functions
bool db2_check_timeout_expired(time_t start_time, int timeout_seconds);

// Connection management
bool db2_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool db2_disconnect(DatabaseHandle* connection);
bool db2_health_check(DatabaseHandle* connection);
bool db2_reset_connection(DatabaseHandle* connection);

// Watchdog cancel hook - implements engine cancel_inflight
void db2_cancel_inflight(DatabaseHandle* connection);

// Active-statement tracking for watchdog cancel support. Query
// execution paths call db2_active_stmt_set immediately after
// allocating a statement handle and db2_active_stmt_clear
// immediately before freeing it. The cancel hook reads the value
// under active_stmt_lock.
void db2_active_stmt_set(DatabaseHandle* connection, void* stmt_handle);
void db2_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle);

// Library loading
bool load_libdb2_functions(const char* designator);

// Utility functions for prepared statement cache
PreparedStatementCache* db2_create_prepared_statement_cache(void);
void db2_destroy_prepared_statement_cache(PreparedStatementCache* cache);

// Transaction control function
extern SQLSetConnectAttr_t SQLSetConnectAttr_ptr;

#endif // DATABASE_ENGINE_DB2_CONNECTION_H