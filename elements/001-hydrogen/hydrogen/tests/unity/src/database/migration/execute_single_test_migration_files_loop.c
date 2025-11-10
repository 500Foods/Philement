/*
 * Unity Test File: execute_migration_files loop coverage
 * This file contains unit tests to cover the migration loop in execute_migration_files
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations for functions being tested
bool execute_migration_files(DatabaseHandle* connection, char** migration_files,
                            size_t migration_count, const char* engine_name,
                            const char* migration_name, const char* schema_name,
                            const char* dqm_label);

// Forward declarations for test functions
void test_execute_migration_files_with_single_migration(void);
void test_execute_migration_files_with_multiple_migrations(void);
void test_execute_migration_files_with_two_migrations(void);
void test_execute_migration_files_with_four_migrations(void);
void test_execute_migration_files_with_long_filenames(void);

// Test setup and teardown
void setUp(void) {
    // Reset any mocks if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== execute_migration_files LOOP TESTS =====

void test_execute_migration_files_with_single_migration(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Create array of migration file names
    char* migration_files[] = {
        (char*)"001_initial.lua"
    };
    
    bool result = execute_migration_files(
        &connection,
        migration_files,
        1,              // migration_count = 1
        "sqlite",
        "test_design",
        NULL,
        "test-label"
    );
    
    // Will fail because payload doesn't exist, but will enter the loop
    // This covers lines 116-120
    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_with_multiple_migrations(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    
    // Create array of multiple migration files
    char* migration_files[] = {
        (char*)"001_initial.lua",
        (char*)"002_add_users.lua",
        (char*)"003_add_posts.lua"
    };
    
    bool result = execute_migration_files(
        &connection,
        migration_files,
        3,              // migration_count = 3
        "postgresql",
        "test_design",
        "public",
        "test-label"
    );
    
    // Will fail on first migration and break
    // This covers the loop entry and break logic
    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_with_two_migrations(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    
    char* migration_files[] = {
        (char*)"001_schema.lua",
        (char*)"002_data.lua"
    };
    
    bool result = execute_migration_files(
        &connection,
        migration_files,
        2,
        "mysql",
        "test_design",
        NULL,
        "test-label"
    );
    
    // Covers loop execution with 2 iterations
    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_with_four_migrations(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    char* migration_files[] = {
        (char*)"001_base.lua",
        (char*)"002_alter.lua",
        (char*)"003_index.lua",
        (char*)"004_perms.lua"
    };
    
    bool result = execute_migration_files(
        &connection,
        migration_files,
        4,
        "db2",
        "test_design",
        NULL,
        "test-label"
    );
    
    // Covers loop with 4 elements
    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_with_long_filenames(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    char* migration_files[] = {
        (char*)"001_very_long_migration_filename_for_testing_purposes.lua",
        (char*)"002_another_long_filename_with_many_characters.lua"
    };
    
    bool result = execute_migration_files(
        &connection,
        migration_files,
        2,
        "sqlite",
        "very_long_design_name",
        NULL,
        "test-label-long"
    );
    
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Loop execution tests
    RUN_TEST(test_execute_migration_files_with_single_migration);
    RUN_TEST(test_execute_migration_files_with_multiple_migrations);
    RUN_TEST(test_execute_migration_files_with_two_migrations);
    RUN_TEST(test_execute_migration_files_with_four_migrations);
    RUN_TEST(test_execute_migration_files_with_long_filenames);
    
    return UNITY_END();
}