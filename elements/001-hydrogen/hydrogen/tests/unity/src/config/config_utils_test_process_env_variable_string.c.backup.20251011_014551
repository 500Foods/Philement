/*
 * Unity Test File: Configuration Utils Tests
 * This file contains unit tests for the config_utils functions
 * from src/config/config_utils.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_utils.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
char* process_env_variable_string(const char* input);

// Forward declarations for test functions
void test_process_env_variable_string_null_input(void);
void test_process_env_variable_string_no_env_var(void);
void test_process_env_variable_string_env_var_not_set(void);
void test_process_env_variable_string_env_var_set(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PROCESS_ENV_VARIABLE_STRING TESTS =====

void test_process_env_variable_string_null_input(void) {
    // Test with NULL input - should return NULL
    char* result = process_env_variable_string(NULL);

    TEST_ASSERT_NULL(result);
}

void test_process_env_variable_string_no_env_var(void) {
    // Test with string that doesn't contain environment variable
    const char* input = "regular string without env var";
    char* result = process_env_variable_string(input);

    TEST_ASSERT_NULL(result);
}

void test_process_env_variable_string_env_var_not_set(void) {
    // Test with environment variable that is not set
    const char* input = "${env.NONEXISTENT_VAR}";
    char* result = process_env_variable_string(input);

    TEST_ASSERT_NULL(result);
}

void test_process_env_variable_string_env_var_set(void) {
    // Test with environment variable that is set
    setenv("TEST_VAR", "test_value", 1);
    const char* input = "${env.TEST_VAR}";
    char* result = process_env_variable_string(input);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_value", result);

    // Clean up
    free(result);
    unsetenv("TEST_VAR");
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // process_env_variable_string tests
    RUN_TEST(test_process_env_variable_string_null_input);
    RUN_TEST(test_process_env_variable_string_no_env_var);
    RUN_TEST(test_process_env_variable_string_env_var_not_set);
    RUN_TEST(test_process_env_variable_string_env_var_set);

    return UNITY_END();
}