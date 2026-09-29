/*
 * MSSQL Database Engine - Prepared Statement Management Header
 *
 * Header file for MSSQL prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_PREPARED_H
#define DATABASE_ENGINE_MSSQL_PREPARED_H

#include <src/database/database.h>
#include "types.h"

// Prepared statement management
bool mssql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool mssql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool mssql_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mssql_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Helper functions for better testability
bool mssql_validate_prepared_statement_functions(void);
void* mssql_create_statement_handle(void* mssql_connection);
bool mssql_prepare_statement_handle(void* stmt_handle, const char* sql);
bool mssql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
size_t mssql_find_lru_statement_index(DatabaseHandle* connection);
void mssql_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool mssql_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mssql_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void mssql_cleanup_prepared_statement(PreparedStatement* stmt);
void mssql_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);

#endif // DATABASE_ENGINE_MSSQL_PREPARED_H
