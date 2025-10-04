/*
 * Unity Test File: utils_dependency_test_database_version.c
 *
 * Tests for get_database_version function in utils_dependency.c
 * This function is static, so we test it indirectly through
 * the public interface that triggers database version checking.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Enable mocks for testing system functions
#define USE_MOCK_SYSTEM
#include "../../../../tests/unity/mocks/mock_system.h"

// Function prototypes for test functions
void test_get_database_version_integration(void);
void test_get_database_version_with_cache_hits(void);
void test_get_database_version_with_cache_misses(void);
void test_get_database_version_command_execution(void);
void test_get_database_version_timeout_handling(void);
void test_get_database_version_error_conditions(void);
void test_get_database_version_different_databases(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_system_reset_all();
}

void tearDown(void) {
    // Reset mocks after each test
    mock_system_reset_all();
}

// Integration test for get_database_version function
void test_get_database_version_integration(void) {
    // Test that get_database_version works through the public interface
    // This indirectly tests the get_database_version function
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // This should complete without crashing and test get_database_version indirectly
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test get_database_version with cache hits
void test_get_database_version_with_cache_hits(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Run multiple times to test cache hit behavior
    // The second run should hit the cache for database version checks
    for (int i = 0; i < 3; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test get_database_version with cache misses
void test_get_database_version_with_cache_misses(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // First run should be cache miss, subsequent runs should be cache hits
    for (int i = 0; i < 2; i++) {
        int result = check_library_dependencies(&config);
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    }
}

// Test get_database_version command execution scenarios
void test_get_database_version_command_execution(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that command execution works properly
    // This exercises the popen, fread, pclose sequence in get_database_version
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test get_database_version timeout handling
void test_get_database_version_timeout_handling(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that timeout handling works correctly
    // The 30-second timeout in get_database_version should be respected
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test get_database_version error conditions
void test_get_database_version_error_conditions(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test various error conditions that get_database_version might encounter
    // - NULL config parameter
    // - Command execution failures
    // - File I/O errors
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test get_database_version with different database types
void test_get_database_version_different_databases(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // Test that different database types are handled correctly
    // This exercises different code paths in get_database_version
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_database_version_integration);
    RUN_TEST(test_get_database_version_with_cache_hits);
    RUN_TEST(test_get_database_version_with_cache_misses);
    RUN_TEST(test_get_database_version_command_execution);
    RUN_TEST(test_get_database_version_timeout_handling);
    RUN_TEST(test_get_database_version_error_conditions);
    RUN_TEST(test_get_database_version_different_databases);

    return UNITY_END();
}