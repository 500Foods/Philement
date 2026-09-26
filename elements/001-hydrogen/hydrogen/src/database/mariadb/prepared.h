/*
 * MariaDB Database Engine - Prepared Statement Management Header
 *
 * Header file for MariaDB prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_PREPARED_H
#define DATABASE_ENGINE_MARIADB_PREPARED_H

#include <src/database/database.h>
#include "types.h"

// Prepared statement management
bool mariadb_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool mariadb_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool mariadb_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mariadb_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Helper functions for better testability
bool mariadb_validate_prepared_statement_functions(void);
void* mariadb_create_statement_handle(void* mariadb_connection);
bool mariadb_prepare_statement_handle(void* stmt_handle, const char* sql);
bool mariadb_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
size_t mariadb_find_lru_statement_index(DatabaseHandle* connection);
void mariadb_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool mariadb_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mariadb_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void mariadb_cleanup_prepared_statement(PreparedStatement* stmt);
void mariadb_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);

#endif // DATABASE_ENGINE_MARIADB_PREPARED_H
