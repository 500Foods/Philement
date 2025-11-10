/*
 * Unity Test File: execute_single_migration with mocked failures
 * This file uses mocks to test error paths in execute_single_migration
 * that require getting past initial payload validation
 */

#include <src/hydrogen.h>
#include <tests/unity/mocks/mock_database_migrations.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/payload/payload.h>

// Forward declarations
void test_execute_single_migration_lua_load_database_module_failure(void);
void test_execute_single_migration_lua_find_migration_file_failure(void);
void test_execute_single_migration_lua_load_migration_file_failure(void);

void setUp(void) {
    mock_database_migrations_reset_all();
}

void tearDown(void) {
    mock_database_migrations_reset_all();
}

// Test lua_load_database_module failure (lines 50-54)
void test_execute_single_migration_lua_load_database_module_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Configure mocks to pass get_payload_files but fail lua_load_database_module
    mock_database_migrations_set_get_payload_files_result(true);
    mock_database_migrations_set_load_database_module_result(false);
    
    bool result = execute_single_migration(
        &connection,
        "test.lua",
        "sqlite",
        "test",
        NULL,
        "test-label"
    );
    
    // Should fail at lua_load_database_module (line 50)
    TEST_ASSERT_FALSE(result);
}

// Test lua_find_migration_file failure (lines 58-65)
void test_execute_single_migration_lua_find_migration_file_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    
    // Configure mocks to pass get_payload_files and lua_load_database_module
    // but fail lua_find_migration_file
    mock_database_migrations_set_get_payload_files_result(true);
    mock_database_migrations_set_load_database_module_result(true);
    mock_database_migrations_set_find_migration_file_result(NULL);  // File not found
    
    bool result = execute_single_migration(
        &connection,
        "nonexistent.lua",
        "postgresql",
        "test",
        "public",
        "test-label"
    );
    
    // Should fail at lua_find_migration_file check (line 60)
    TEST_ASSERT_FALSE(result);
}

// Test lua_load_migration_file failure (lines 69-73)
void test_execute_single_migration_lua_load_migration_file_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    
    // Create a dummy payload file pointer (non-NULL)
    PayloadFile dummy_file = {0};
    
    // Configure mocks to pass preliminary checks but fail lua_load_migration_file
    mock_database_migrations_set_get_payload_files_result(true);
    mock_database_migrations_set_load_database_module_result(true);
    mock_database_migrations_set_find_migration_file_result(&dummy_file);
    mock_database_migrations_set_load_migration_file_result(false);
    
    bool result = execute_single_migration(
        &connection,
        "test.lua",
        "mysql",
        "test",
        NULL,
        "test-label"
    );
    
    // Should fail at lua_load_migration_file (line 69)
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_execute_single_migration_lua_load_database_module_failure);
    RUN_TEST(test_execute_single_migration_lua_find_migration_file_failure);
    RUN_TEST(test_execute_single_migration_lua_load_migration_file_failure);
    
    return UNITY_END();
}