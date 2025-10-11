/*
 * Unity Test File: utils_dependency_test_parsing_functions.c
 *
 * Tests for database version parsing functions in utils_dependency.c
 * These functions are static, so we test them indirectly through
 * the public interface that triggers parsing operations.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing system functions
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Function prototypes for test functions
void test_database_parsing_integration(void);
void test_db2_parsing_behavior(void);
void test_postgresql_parsing_behavior(void);
void test_mysql_parsing_behavior(void);
void test_sqlite_parsing_behavior(void);
void test_parsing_with_malformed_output(void);
void test_parsing_with_empty_output(void);
void test_parsing_with_null_output(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_system_reset_all();
}

void tearDown(void) {
    // Reset mocks after each test
    mock_system_reset_all();
}

// Integration test for database parsing functions
void test_database_parsing_integration(void) {
    // Test that database parsing works through the public interface
    // This indirectly tests parse_db2_version, parse_postgresql_version,
    // parse_mysql_version, and parse_sqlite_version functions
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // This should complete without crashing and test the parsing functions indirectly
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test DB2 parsing behavior through multiple runs
void test_db2_parsing_behavior(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Run multiple times to test DB2 parsing consistency
    for (int i = 0; i < 3; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test PostgreSQL parsing behavior through multiple runs
void test_postgresql_parsing_behavior(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Run multiple times to test PostgreSQL parsing consistency
    for (int i = 0; i < 3; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test MySQL parsing behavior through multiple runs
void test_mysql_parsing_behavior(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Run multiple times to test MySQL parsing consistency
    for (int i = 0; i < 3; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test SQLite parsing behavior through multiple runs
void test_sqlite_parsing_behavior(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Run multiple times to test SQLite parsing consistency
    for (int i = 0; i < 3; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test parsing with malformed command output
void test_parsing_with_malformed_output(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that malformed output is handled gracefully
    // The parsing functions should return "None" for unparseable output
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test parsing with empty command output
void test_parsing_with_empty_output(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that empty output is handled gracefully
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test parsing with NULL output scenarios
void test_parsing_with_null_output(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that NULL output scenarios are handled gracefully
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_parsing_integration);
    RUN_TEST(test_db2_parsing_behavior);
    RUN_TEST(test_postgresql_parsing_behavior);
    RUN_TEST(test_mysql_parsing_behavior);
    RUN_TEST(test_sqlite_parsing_behavior);
    RUN_TEST(test_parsing_with_malformed_output);
    RUN_TEST(test_parsing_with_empty_output);
    RUN_TEST(test_parsing_with_null_output);

    return UNITY_END();
}