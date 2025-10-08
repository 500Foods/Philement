/*
 * Database Migration Management Header
 *
 * Function declarations for migration validation and execution.
 * Modular architecture supporting PAYLOAD and path-based migrations.
 */

#ifndef DATABASE_MIGRATIONS_H
#define DATABASE_MIGRATIONS_H

#include <stdbool.h>
#include <lua.h>

// Forward declarations
struct DatabaseQueue;

// Include payload types
#include "../payload/payload.h"

// Core migration functions
bool database_migrations_validate(struct DatabaseQueue* db_queue);
bool database_migrations_execute_auto(struct DatabaseQueue* db_queue, DatabaseHandle* connection);
bool database_migrations_execute_test(struct DatabaseQueue* db_queue);

// File discovery functions
bool database_migrations_discover_files(const struct DatabaseConnection* conn_config, char*** migration_files,
                                      size_t* migration_count, const char* dqm_label);
void database_migrations_cleanup_files(char** migration_files, size_t migration_count);

// Transaction handling functions
bool database_migrations_execute_transaction(DatabaseHandle* connection, const char* sql_result,
                                          size_t sql_length, const char* migration_file,
                                          DatabaseEngine engine_type, const char* dqm_label);

// Lua integration functions
lua_State* database_migrations_lua_setup(const char* dqm_label);
bool database_migrations_lua_load_database_module(lua_State* L, const char* migration_name,
                                                PayloadFile* payload_files, size_t payload_count,
                                                const char* dqm_label);
PayloadFile* database_migrations_lua_find_migration_file(const char* migration_file_path,
                                                      PayloadFile* payload_files, size_t payload_count);
bool database_migrations_lua_load_migration_file(lua_State* L, PayloadFile* mig_file,
                                               const char* migration_file_path, const char* dqm_label);
bool database_migrations_lua_extract_queries_table(lua_State* L, int* query_count, const char* dqm_label);
bool database_migrations_lua_execute_run_migration(lua_State* L, const char* engine_name,
                                                 const char* migration_name, const char* schema_name,
                                                 size_t* sql_length, const char** sql_result,
                                                 const char* dqm_label);
void database_migrations_lua_log_execution_summary(const char* migration_file_path, size_t sql_length,
                                                 int line_count, int query_count, const char* dqm_label);
void database_migrations_lua_cleanup(lua_State* L);

#endif /* DATABASE_MIGRATIONS_H */