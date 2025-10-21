/*
 * MySQL Database Engine - Prepared Statement Management Header
 *
 * Header file for MySQL prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_PREPARED_H
#define DATABASE_ENGINE_MYSQL_PREPARED_H

#include <src/database/database.h>
#include "types.h"

// Prepared statement management
bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool mysql_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mysql_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Helper functions for better testability
bool mysql_validate_prepared_statement_functions(void);
void* mysql_create_statement_handle(void* mysql_connection);
bool mysql_prepare_statement_handle(void* stmt_handle, const char* sql);
bool mysql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
size_t mysql_find_lru_statement_index(DatabaseHandle* connection);
void mysql_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool mysql_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mysql_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void mysql_cleanup_prepared_statement(PreparedStatement* stmt);

#endif // DATABASE_ENGINE_MYSQL_PREPARED_H