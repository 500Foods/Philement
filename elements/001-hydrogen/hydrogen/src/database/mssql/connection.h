/*
 * MSSQL Database Engine - Connection Management Header
 *
 * Header file for MSSQL connection management functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_CONNECTION_H
#define DATABASE_ENGINE_MSSQL_CONNECTION_H

#include <src/database/database.h>
#include "types.h"

// Utility functions
bool mssql_check_timeout_expired(time_t start_time, int timeout_seconds);

// Connection management
bool mssql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mssql_disconnect(DatabaseHandle* connection);
bool mssql_health_check(DatabaseHandle* connection);
bool mssql_reset_connection(DatabaseHandle* connection);

// Watchdog cancel hook - implements engine cancel_inflight
void mssql_cancel_inflight(DatabaseHandle* connection);

// Active-statement tracking for watchdog cancel support. Query
// execution paths call mssql_active_stmt_set immediately after
// allocating a statement handle and mssql_active_stmt_clear
// immediately before freeing it. The cancel hook reads the value
// under active_stmt_lock.
void mssql_active_stmt_set(DatabaseHandle* connection, void* stmt_handle);
void mssql_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle);

// Library loading
bool load_msobdc_functions(const char* designator);

// Utility functions for prepared statement cache
PreparedStatementCache* mssql_create_prepared_statement_cache(void);
void mssql_destroy_prepared_statement_cache(PreparedStatementCache* cache);

// Transaction control function
extern SQLSetConnectAttr_t mssql_SQLSetConnectAttr_ptr;

#endif // DATABASE_ENGINE_MSSQL_CONNECTION_H
