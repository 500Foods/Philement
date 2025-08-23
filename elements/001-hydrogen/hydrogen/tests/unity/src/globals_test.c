/*
 * Unity Test File: globals_test.c
 *
 * Tests for globals.c functions and global state variables
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_get_executable_size_valid_executable(void);
void test_get_executable_size_null_argv(void);
void test_get_executable_size_null_executable(void);
void test_get_executable_size_invalid_executable(void);
void test_global_state_initialization(void);
void test_registry_state_variables(void);
void test_executable_size_state_variable(void);

#ifndef GLOBALS_TEST_RUNNER
void setUp(void) {
    // Reset global state before each test
    app_config = NULL;
    registry_registered = 0;
    registry_running = 0;
    registry_attempted = 0;
    registry_failed = 0;
    server_executable_size = 0;
}

void tearDown(void) {
    // Clean up after each test - nothing specific needed for globals tests
}
#endif

// Tests for get_executable_size function
void test_get_executable_size_valid_executable(void) {
    // Save original size
    long long original_size = server_executable_size;

    // Create argv with a valid executable path
    char *argv[] = {(char*)"/bin/ls", NULL};

    // Call function
    get_executable_size(argv);

    // Verify size was set (should be > 0 for a real executable)
    TEST_ASSERT_GREATER_THAN(0LL, server_executable_size);

    // Restore original size for other tests
    server_executable_size = original_size;
}

void test_get_executable_size_null_argv(void) {
    // Save original size
    long long original_size = server_executable_size;

    // Call function with NULL argv
    get_executable_size(NULL);

    // Verify size was set to 0
    TEST_ASSERT_EQUAL(0LL, server_executable_size);

    // Restore original size
    server_executable_size = original_size;
}

void test_get_executable_size_null_executable(void) {
    // Save original size
    long long original_size = server_executable_size;

    // Create argv with NULL executable
    char *argv[] = {NULL, NULL};

    // Call function
    get_executable_size(argv);

    // Verify size was set to 0
    TEST_ASSERT_EQUAL(0LL, server_executable_size);

    // Restore original size
    server_executable_size = original_size;
}

void test_get_executable_size_invalid_executable(void) {
    // Save original size
    long long original_size = server_executable_size;

    // Create argv with non-existent executable
    char *argv[] = {(char*)"/nonexistent/path/to/executable", NULL};

    // Call function
    get_executable_size(argv);

    // Verify size was set to 0 (stat should fail)
    TEST_ASSERT_EQUAL(0LL, server_executable_size);

    // Restore original size
    server_executable_size = original_size;
}

// Tests for global state variables
void test_global_state_initialization(void) {
    // Test that global variables are properly initialized
    // (These should be initialized to 0/NULL as set up in setUp)

    TEST_ASSERT_NULL(app_config);
    TEST_ASSERT_EQUAL(0, registry_registered);
    TEST_ASSERT_EQUAL(0, registry_running);
    TEST_ASSERT_EQUAL(0, registry_attempted);
    TEST_ASSERT_EQUAL(0, registry_failed);
    TEST_ASSERT_EQUAL(0LL, server_executable_size);
}

void test_registry_state_variables(void) {
    // Test that registry state variables can be modified

    // Modify registry variables
    registry_registered = 5;
    registry_running = 3;
    registry_attempted = 8;
    registry_failed = 2;

    // Verify changes
    TEST_ASSERT_EQUAL(5, registry_registered);
    TEST_ASSERT_EQUAL(3, registry_running);
    TEST_ASSERT_EQUAL(8, registry_attempted);
    TEST_ASSERT_EQUAL(2, registry_failed);

    // Test arithmetic operations
    registry_registered++;
    registry_running += 2;
    registry_attempted *= 2;

    TEST_ASSERT_EQUAL(6, registry_registered);
    TEST_ASSERT_EQUAL(5, registry_running);
    TEST_ASSERT_EQUAL(16, registry_attempted);
}

void test_executable_size_state_variable(void) {
    // Test that server_executable_size can be modified

    // Set to a specific value
    server_executable_size = 12345LL;

    // Verify change
    TEST_ASSERT_EQUAL(12345LL, server_executable_size);

    // Test arithmetic operations
    server_executable_size *= 2;  // 12345 * 2 = 24690
    server_executable_size += 1000;  // 24690 + 1000 = 25690

    TEST_ASSERT_EQUAL(25690LL, server_executable_size);

    // Test setting to 0
    server_executable_size = 0;
    TEST_ASSERT_EQUAL(0LL, server_executable_size);
}

// Main function - only compiled when this file is run standalone
#ifndef GLOBALS_TEST_RUNNER
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_executable_size_valid_executable);
    RUN_TEST(test_get_executable_size_null_argv);
    RUN_TEST(test_get_executable_size_null_executable);
    RUN_TEST(test_get_executable_size_invalid_executable);
    RUN_TEST(test_global_state_initialization);
    RUN_TEST(test_registry_state_variables);
    RUN_TEST(test_executable_size_state_variable);

    return UNITY_END();
}
#endif
