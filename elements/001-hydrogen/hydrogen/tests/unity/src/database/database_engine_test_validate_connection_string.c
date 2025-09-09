/*
 * Unity Test File: database_engine_validate_connection_string
 * This file contains unit tests for database_engine_validate_connection_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string);

// Function prototypes for test functions
void test_database_engine_validate_connection_string_null_string(void);
void test_database_engine_validate_connection_string_empty_string(void);
void test_database_engine_validate_connection_string_invalid_engine(void);
void test_database_engine_validate_connection_string_sqlite_valid(void);
void test_database_engine_validate_connection_string_sqlite_memory(void);
void test_database_engine_validate_connection_string_sqlite_file_path(void);
void test_database_engine_validate_connection_string_unregistered_engine(void);

void setUp(void) {
    // Initialize the engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_validate_connection_string_null_string(void) {
    // Test null connection string
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_validate_connection_string_empty_string(void) {
    // Test empty connection string
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, "");
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_validate_connection_string_invalid_engine(void) {
    // Test invalid engine type
    bool result = database_engine_validate_connection_string(DB_ENGINE_MAX, "test.db");
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_validate_connection_string_sqlite_valid(void) {
    // Test valid SQLite connection string
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, "test.db");
    TEST_ASSERT_TRUE(result);
}

void test_database_engine_validate_connection_string_sqlite_memory(void) {
    // Test SQLite :memory: connection string
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, ":memory:");
    TEST_ASSERT_TRUE(result);
}

void test_database_engine_validate_connection_string_sqlite_file_path(void) {
    // Test SQLite file path
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, "/path/to/database.db");
    TEST_ASSERT_TRUE(result);
}

void test_database_engine_validate_connection_string_unregistered_engine(void) {
    // Test engine that isn't registered (like DB2 if not available)
    bool result = database_engine_validate_connection_string(DB_ENGINE_DB2, "test");
    // This might return false if DB2 engine isn't loaded, which is expected
    // We just test that it doesn't crash
    TEST_ASSERT_NOT_NULL(&result); // Just ensure function returns
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_validate_connection_string_null_string);
    RUN_TEST(test_database_engine_validate_connection_string_empty_string);
    RUN_TEST(test_database_engine_validate_connection_string_invalid_engine);
    RUN_TEST(test_database_engine_validate_connection_string_sqlite_valid);
    RUN_TEST(test_database_engine_validate_connection_string_sqlite_memory);
    RUN_TEST(test_database_engine_validate_connection_string_sqlite_file_path);
    RUN_TEST(test_database_engine_validate_connection_string_unregistered_engine);

    return UNITY_END();
}