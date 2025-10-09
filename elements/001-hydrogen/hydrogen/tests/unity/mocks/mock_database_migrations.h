#ifndef MOCK_DATABASE_MIGRATIONS_H
#define MOCK_DATABASE_MIGRATIONS_H

#include <stdbool.h>
#include <lua.h>
#include <stddef.h>

// Enable mock override
#ifdef USE_MOCK_DATABASE_MIGRATIONS
#define database_migrations_execute_single_migration mock_database_migrations_execute_single_migration
#define database_migrations_lua_setup mock_database_migrations_lua_setup
#define get_payload_files_by_prefix mock_get_payload_files_by_prefix
#define database_migrations_lua_load_database_module mock_database_migrations_lua_load_database_module
#define database_migrations_lua_find_migration_file mock_database_migrations_lua_find_migration_file
#define database_migrations_lua_load_migration_file mock_database_migrations_lua_load_migration_file
#define database_migrations_lua_extract_queries_table mock_database_migrations_lua_extract_queries_table
#define database_migrations_lua_execute_run_migration mock_database_migrations_lua_execute_run_migration
#define database_migrations_lua_log_execution_summary mock_database_migrations_lua_log_execution_summary
#define database_migrations_execute_transaction mock_database_migrations_execute_transaction
#define database_migrations_lua_cleanup mock_database_migrations_lua_cleanup
#endif

// Mock function prototypes
bool mock_database_migrations_execute_single_migration(void* connection, const char* migration_file, const char* engine_name, const char* migration_name, const char* schema_name, const char* dqm_label);
lua_State* mock_database_migrations_lua_setup(const char* dqm_label);
bool mock_get_payload_files_by_prefix(const char* prefix, void** files, size_t* count, size_t* capacity);
bool mock_database_migrations_lua_load_database_module(lua_State* L, const char* migration_name, void* payload_files, size_t payload_count, const char* dqm_label);
void* mock_database_migrations_lua_find_migration_file(const char* migration_file_path, void* payload_files, size_t payload_count);
bool mock_database_migrations_lua_load_migration_file(lua_State* L, void* mig_file, const char* migration_file_path, const char* dqm_label);
bool mock_database_migrations_lua_extract_queries_table(lua_State* L, int* query_count, const char* dqm_label);
bool mock_database_migrations_lua_execute_run_migration(lua_State* L, const char* engine_name, const char* migration_name, const char* schema_name, size_t* sql_length, const char** sql_result, const char* dqm_label);
void mock_database_migrations_lua_log_execution_summary(const char* migration_file_path, size_t sql_length, int line_count, int query_count, const char* dqm_label);
bool mock_database_migrations_execute_transaction(void* connection, const char* sql_result, size_t sql_length, const char* migration_file, int engine_type, const char* dqm_label);
void mock_database_migrations_lua_cleanup(lua_State* L);

// Mock control functions
void mock_database_migrations_reset_all(void);
void mock_database_migrations_set_single_migration_result(bool result);
void mock_database_migrations_set_lua_setup_result(lua_State* result);
void mock_database_migrations_set_get_payload_files_result(bool result);
void mock_database_migrations_set_load_database_module_result(bool result);
void mock_database_migrations_set_find_migration_file_result(void* result);
void mock_database_migrations_set_load_migration_file_result(bool result);
void mock_database_migrations_set_extract_queries_table_result(bool result);
void mock_database_migrations_set_execute_run_migration_result(bool result);
void mock_database_migrations_set_execute_transaction_result(bool result);

#endif // MOCK_DATABASE_MIGRATIONS_H