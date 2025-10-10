/*
 * Mock implementations for database migration functions
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/payload/payload.h>
#include <unity/mocks/mock_database_migrations.h>

// Static variables for mock state
static bool mock_single_migration_result = true;
static lua_State* mock_lua_setup_result = NULL;
static bool mock_get_payload_files_result = true;
static bool mock_load_database_module_result = true;
static PayloadFile* mock_find_migration_file_result = NULL;
static bool mock_load_migration_file_result = true;
static bool mock_extract_queries_table_result = true;
static bool mock_execute_run_migration_result = true;
static bool mock_execute_transaction_result = true;

// Mock implementations
bool mock_database_migrations_execute_single_migration(void* connection, const char* migration_file, const char* engine_name, const char* migration_name, const char* schema_name, const char* dqm_label) {
    (void)connection;
    (void)migration_file;
    (void)engine_name;
    (void)migration_name;
    (void)schema_name;
    (void)dqm_label;
    return mock_single_migration_result;
}

lua_State* mock_database_migrations_lua_setup(const char* dqm_label) {
    (void)dqm_label;
    return mock_lua_setup_result;
}

bool mock_get_payload_files_by_prefix(const char* prefix, void** files, size_t* count, size_t* capacity) {
    (void)prefix;
    (void)files;
    (void)count;
    (void)capacity;
    return mock_get_payload_files_result;
}

bool mock_database_migrations_lua_load_database_module(lua_State* L, const char* migration_name, void* payload_files, size_t payload_count, const char* dqm_label) {
    (void)L;
    (void)migration_name;
    (void)payload_files;
    (void)payload_count;
    (void)dqm_label;
    return mock_load_database_module_result;
}

void* mock_database_migrations_lua_find_migration_file(const char* migration_file_path, void* payload_files, size_t payload_count) {
    (void)migration_file_path;
    (void)payload_files;
    (void)payload_count;
    return mock_find_migration_file_result;
}

bool mock_database_migrations_lua_load_migration_file(lua_State* L, void* mig_file, const char* migration_file_path, const char* dqm_label) {
    (void)L;
    (void)mig_file;
    (void)migration_file_path;
    (void)dqm_label;
    return mock_load_migration_file_result;
}

bool mock_database_migrations_lua_extract_queries_table(lua_State* L, int* query_count, const char* dqm_label) {
    (void)L;
    (void)query_count;
    (void)dqm_label;
    return mock_extract_queries_table_result;
}

bool mock_database_migrations_lua_execute_run_migration(lua_State* L, const char* engine_name, const char* migration_name, const char* schema_name, size_t* sql_length, const char** sql_result, const char* dqm_label) {
    (void)L;
    (void)engine_name;
    (void)migration_name;
    (void)schema_name;
    (void)dqm_label;
    if (sql_length) *sql_length = 10;
    if (sql_result) *sql_result = "SELECT 1;";
    return mock_execute_run_migration_result;
}

void mock_database_migrations_lua_log_execution_summary(const char* migration_file_path, size_t sql_length, int line_count, int query_count, const char* dqm_label) {
    (void)migration_file_path;
    (void)sql_length;
    (void)line_count;
    (void)query_count;
    (void)dqm_label;
}

bool mock_database_migrations_execute_transaction(void* connection, const char* sql_result, size_t sql_length, const char* migration_file, int engine_type, const char* dqm_label) {
    (void)connection;
    (void)sql_result;
    (void)sql_length;
    (void)migration_file;
    (void)engine_type;
    (void)dqm_label;
    return mock_execute_transaction_result;
}

void mock_database_migrations_lua_cleanup(lua_State* L) {
    (void)L;
}

// Mock control functions
void mock_database_migrations_reset_all(void) {
    mock_single_migration_result = true;
    mock_lua_setup_result = NULL;
    mock_get_payload_files_result = true;
    mock_load_database_module_result = true;
    mock_find_migration_file_result = NULL;
    mock_load_migration_file_result = true;
    mock_extract_queries_table_result = true;
    mock_execute_run_migration_result = true;
    mock_execute_transaction_result = true;
}

void mock_database_migrations_set_single_migration_result(bool result) {
    mock_single_migration_result = result;
}

void mock_database_migrations_set_lua_setup_result(lua_State* result) {
    mock_lua_setup_result = result;
}

void mock_database_migrations_set_get_payload_files_result(bool result) {
    mock_get_payload_files_result = result;
}

void mock_database_migrations_set_load_database_module_result(bool result) {
    mock_load_database_module_result = result;
}

void mock_database_migrations_set_find_migration_file_result(void* result) {
    mock_find_migration_file_result = result;
}

void mock_database_migrations_set_load_migration_file_result(bool result) {
    mock_load_migration_file_result = result;
}

void mock_database_migrations_set_extract_queries_table_result(bool result) {
    mock_extract_queries_table_result = result;
}

void mock_database_migrations_set_execute_run_migration_result(bool result) {
    mock_execute_run_migration_result = result;
}

void mock_database_migrations_set_execute_transaction_result(bool result) {
    mock_execute_transaction_result = result;
}