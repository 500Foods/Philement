/*
 * Unity Test File: database_get_supported_engines
 * This file contains unit tests for database_get_supported_engines functionality
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../../src/database/database.h"

// Forward declarations for functions being tested
void database_get_supported_engines(char* buffer, size_t buffer_size);

// Function prototypes for test functions
void test_database_get_supported_engines_basic_functionality(void);
void test_database_get_supported_engines_null_buffer(void);
void test_database_get_supported_engines_zero_buffer_size(void);
void test_database_get_supported_engines_small_buffer(void);
void test_database_get_supported_engines_large_buffer(void);

void setUp(void) {
    // Initialize database subsystem for testing
    database_subsystem_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
    database_subsystem_shutdown();
}

// Test functions
void test_database_get_supported_engines_basic_functionality(void) {
    // Test basic functionality
    char buffer[256];
    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    // Should contain expected engine names
    TEST_ASSERT_TRUE(strstr(buffer, "PostgreSQL") != NULL ||
                     strstr(buffer, "SQLite") != NULL ||
                     strstr(buffer, "MySQL") != NULL);
}

void test_database_get_supported_engines_null_buffer(void) {
    // Test null buffer handling - should not crash
    database_get_supported_engines(NULL, 100);
    // Function should handle gracefully
}

void test_database_get_supported_engines_zero_buffer_size(void) {
    // Test zero buffer size
    char buffer[256] = {0}; // Initialize to zeros
    memset(buffer, 'X', sizeof(buffer)); // Fill with known pattern
    database_get_supported_engines(buffer, 0);
    // Should not write anything - buffer should still contain our pattern
    TEST_ASSERT_EQUAL('X', buffer[0]);
}

void test_database_get_supported_engines_small_buffer(void) {
    // Test small buffer size
    char buffer[10];
    database_get_supported_engines(buffer, sizeof(buffer));
    // Should truncate safely
    TEST_ASSERT_TRUE(strlen(buffer) < sizeof(buffer));
}

void test_database_get_supported_engines_large_buffer(void) {
    // Test large buffer size
    char buffer[1024];
    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_get_supported_engines_basic_functionality);
    RUN_TEST(test_database_get_supported_engines_null_buffer);
    RUN_TEST(test_database_get_supported_engines_zero_buffer_size);
    RUN_TEST(test_database_get_supported_engines_small_buffer);
    RUN_TEST(test_database_get_supported_engines_large_buffer);

    return UNITY_END();
}