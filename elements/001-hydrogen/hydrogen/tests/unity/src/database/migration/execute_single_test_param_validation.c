/*
 * Unity Test File: execute_single_migration parameter validation
 * This file tests the new parameter validation logic added to execute_single_migration
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations
bool execute_single_migration(DatabaseHandle* connection, const char* migration_file,
                             const char* engine_name, const char* migration_name,
                             const char* schema_name, const char* dqm_label);

// Forward declarations for test functions
void test_parameter_validation_all_nulls(void);
void test_parameter_validation_null_connection(void);
void test_parameter_validation_null_migration_file(void);
void test_parameter_validation_null_engine_name(void);
void test_parameter_validation_null_migration_name(void);
void test_parameter_validation_null_dqm_label(void);
void test_parameter_validation_empty_migration_file(void);
void test_parameter_validation_empty_engine_name(void);
void test_parameter_validation_empty_migration_name(void);
void test_parameter_validation_empty_schema_name(void);
void test_parameter_validation_valid_minimal(void);

void setUp(void) {
    // Reset any state
}

void tearDown(void) {
    // Clean up
}

// Test all NULL parameters
void test_parameter_validation_all_nulls(void) {
    bool result = execute_single_migration(NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test each NULL parameter individually
void test_parameter_validation_null_connection(void) {
    bool result = execute_single_migration(NULL, "test.lua", "sqlite", "test", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_null_migration_file(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, NULL, "sqlite", "test", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_null_engine_name(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", NULL, "test", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_null_migration_name(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", "sqlite", NULL, NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_null_dqm_label(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", "sqlite", "test", NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test empty string parameters
void test_parameter_validation_empty_migration_file(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "", "sqlite", "test", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_empty_engine_name(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", "", "test", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_empty_migration_name(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", "sqlite", "", NULL, "label");
    TEST_ASSERT_FALSE(result);
}

void test_parameter_validation_empty_schema_name(void) {
    DatabaseHandle conn = {0};
    bool result = execute_single_migration(&conn, "test.lua", "postgresql", "test", "", "label");
    TEST_ASSERT_FALSE(result);
}

// Test with minimal valid parameters (will still fail but passes validation)
void test_parameter_validation_valid_minimal(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_SQLITE;
    bool result = execute_single_migration(&conn, "t.lua", "sqlite", "test", NULL, "label");
    // Will fail at payload stage but validation passes
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parameter_validation_all_nulls);
    RUN_TEST(test_parameter_validation_null_connection);
    RUN_TEST(test_parameter_validation_null_migration_file);
    RUN_TEST(test_parameter_validation_null_engine_name);
    RUN_TEST(test_parameter_validation_null_migration_name);
    RUN_TEST(test_parameter_validation_null_dqm_label);
    RUN_TEST(test_parameter_validation_empty_migration_file);
    RUN_TEST(test_parameter_validation_empty_engine_name);
    RUN_TEST(test_parameter_validation_empty_migration_name);
    RUN_TEST(test_parameter_validation_empty_schema_name);
    RUN_TEST(test_parameter_validation_valid_minimal);
    
    return UNITY_END();
}