/*
 * SQLite Database Engine - Prepared Statement Management Header
 *
 * Header file for SQLite prepared statement management functions.
 */

#ifndef SQLITE_PREPARED_H
#define SQLITE_PREPARED_H

#include <src/database/database.h>

// Prepared statement management
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool sqlite_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool sqlite_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Helper functions for better testability
bool sqlite_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
bool sqlite_evict_lru_prepared_statement(DatabaseHandle* connection, const SQLiteConnection* sqlite_conn, const char* new_stmt_name);
bool sqlite_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
void sqlite_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);

#endif // SQLITE_PREPARED_H