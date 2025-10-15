/*
 * Database Migration Management Header
 *
 * Function declarations for migration validation and execution.
 * Modular architecture supporting PAYLOAD and path-based migrations.
 */

#ifndef MIGRATION_H
#define MIGRATION_H

#include <stdbool.h>
#include <lua.h>

// Include database types for DatabaseEngine
#include "../database_types.h"

// Forward declarations
struct DatabaseQueue;
struct DatabaseHandle;

// Include payload types
#include "../../payload/payload.h"

// Core migration functions
bool validate(struct DatabaseQueue* db_queue);
bool execute_auto(struct DatabaseQueue* db_queue, DatabaseHandle* connection);
bool execute_test(struct DatabaseQueue* db_queue);

// Utility functions for migration execution
const char* normalize_engine_name(const char* engine_name);
const char* extract_migration_name(const char* migrations_config, char** path_copy_out);
bool execute_single_migration(DatabaseHandle* connection, const char* migration_file,
                             const char* engine_name, const char* migration_name,
                             const char* schema_name, const char* dqm_label);
bool execute_migration_files(DatabaseHandle* connection, char** migration_files,
                            size_t migration_count, const char* engine_name,
                            const char* migration_name, const char* schema_name,
                            const char* dqm_label);

void free_payload_files(PayloadFile* payload_files, size_t payload_count);

// File discovery functions
bool discover_files(const struct DatabaseConnection* conn_config, char*** migration_files,
                   size_t* migration_count, const char* dqm_label);
void cleanup_files(char** migration_files, size_t migration_count);

// Internal file discovery functions (exposed for unit testing)
void sort_migration_files(char** migration_files, size_t migration_count);
bool discover_payload_migration_files(const char* migration_name, char*** migration_files,
                                     size_t* migration_count, size_t* files_capacity,
                                     const char* dqm_label);
bool discover_path_migration_files(const struct DatabaseConnection* conn_config, char*** migration_files,
                                  size_t* migration_count, size_t* files_capacity,
                                  const char* dqm_label);

// Internal validation functions (exposed for unit testing)
bool validate_payload_migrations(const struct DatabaseConnection* conn_config, const char* dqm_label);
bool validate_path_migrations(const struct DatabaseConnection* conn_config, const char* dqm_label);

// Transaction handling functions
bool execute_transaction(DatabaseHandle* connection, const char* sql_result,
                        size_t sql_length, const char* migration_file,
                        DatabaseEngine engine_type, const char* dqm_label);

// Internal transaction functions (exposed for unit testing)
bool parse_sql_statements(const char* sql_result, size_t sql_length, char*** statements,
                         size_t* statement_count, size_t* statements_capacity,
                         const char* dqm_label);
bool execute_db2_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                          const char* migration_file, const char* dqm_label);
bool execute_postgresql_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                                 const char* migration_file, const char* dqm_label);
bool execute_mysql_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                            const char* migration_file, const char* dqm_label);
bool execute_sqlite_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                             const char* migration_file, const char* dqm_label);

// Lua integration functions
lua_State* lua_setup(const char* dqm_label);
bool lua_load_engine_module(lua_State* L, const char* migration_name,
                           const char* engine_name, PayloadFile* payload_files,
                           size_t payload_count, const char* dqm_label);
bool lua_load_database_module(lua_State* L, const char* migration_name,
                             PayloadFile* payload_files, size_t payload_count,
                             const char* dqm_label);
PayloadFile* lua_find_migration_file(const char* migration_file_path,
                                   PayloadFile* payload_files, size_t payload_count);
bool lua_load_migration_file(lua_State* L, PayloadFile* mig_file,
                            const char* migration_file_path, const char* dqm_label);
bool lua_extract_queries_table(lua_State* L, int* query_count, const char* dqm_label);
bool lua_execute_run_migration(lua_State* L, const char* engine_name,
                              const char* migration_name, const char* schema_name,
                              size_t* sql_length, const char** sql_result,
                              const char* dqm_label);
void lua_log_execution_summary(const char* migration_file_path, size_t sql_length,
                              int line_count, int query_count, const char* dqm_label);
void lua_cleanup(lua_State* L);

#endif /* MIGRATION_H */