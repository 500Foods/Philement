/*
 * DB2 Database Engine - Prepared Statement Management Header
 *
 * Header file for DB2 prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_DB2_PREPARED_H
#define DATABASE_ENGINE_DB2_PREPARED_H

#include <src/database/database.h>
#include "types.h"

// Prepared statement management
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Helper functions for better testability
bool db2_validate_prepared_statement_functions(void);
void* db2_create_statement_handle(void* db2_connection);
bool db2_prepare_statement_handle(void* stmt_handle, const char* sql);
bool db2_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
size_t db2_find_lru_statement_index(DatabaseHandle* connection);
void db2_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool db2_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool db2_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void db2_cleanup_prepared_statement(PreparedStatement* stmt);
void db2_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);

#endif // DATABASE_ENGINE_DB2_PREPARED_H