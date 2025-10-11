/*
 * Unity Test File: Configuration Files Tests
 * This file contains unit tests for the config_files functions
 * from src/config/config_files.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_files.h"

// Forward declarations for functions being tested
bool is_file_readable(const char* path);
char* get_executable_path(void);
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);

// Forward declarations for test functions
void test_is_file_readable_null_path(void);
void test_is_file_readable_nonexistent_file(void);
void test_is_file_readable_existing_file(void);
void test_get_executable_path(void);
void test_get_file_size_null_filename(void);
void test_get_file_size_nonexistent_file(void);
void test_get_file_modification_time_null_filename(void);
void test_get_file_modification_time_nonexistent_file(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== IS_FILE_READABLE TESTS =====

void test_is_file_readable_null_path(void) {
    // Test with NULL path - should return false
    bool result = is_file_readable(NULL);

    TEST_ASSERT_FALSE(result);
}

void test_is_file_readable_nonexistent_file(void) {
    // Test with nonexistent file - should return false
    bool result = is_file_readable("/nonexistent/file/path");

    TEST_ASSERT_FALSE(result);
}

void test_is_file_readable_existing_file(void) {
    // Test with existing file (this test file itself)
    bool result = is_file_readable(__FILE__);

    TEST_ASSERT_TRUE(result);
}

// ===== GET_EXECUTABLE_PATH TESTS =====

void test_get_executable_path(void) {
    // Test getting executable path - should return a valid path
    char* path = get_executable_path();

    // Should return a non-NULL path
    TEST_ASSERT_NOT_NULL(path);

    // Should be an absolute path (start with /)
    TEST_ASSERT_TRUE(path[0] == '/');

    // Should contain "hydrogen" in the path
    TEST_ASSERT_NOT_NULL(strstr(path, "hydrogen"));

    // Clean up
    free(path);
}

// ===== GET_FILE_SIZE TESTS =====

void test_get_file_size_null_filename(void) {
    // Test with NULL filename - should return -1
    long result = get_file_size(NULL);

    TEST_ASSERT_EQUAL(-1, result);
}

void test_get_file_size_nonexistent_file(void) {
    // Test with nonexistent file - should return -1
    long result = get_file_size("/nonexistent/file/path");

    TEST_ASSERT_EQUAL(-1, result);
}

// ===== GET_FILE_MODIFICATION_TIME TESTS =====

void test_get_file_modification_time_null_filename(void) {
    // Test with NULL filename - should return NULL
    char* result = get_file_modification_time(NULL);

    TEST_ASSERT_NULL(result);
}

void test_get_file_modification_time_nonexistent_file(void) {
    // Test with nonexistent file - should return NULL
    char* result = get_file_modification_time("/nonexistent/file/path");

    TEST_ASSERT_NULL(result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // is_file_readable tests
    RUN_TEST(test_is_file_readable_null_path);
    RUN_TEST(test_is_file_readable_nonexistent_file);
    RUN_TEST(test_is_file_readable_existing_file);

    // get_executable_path tests
    RUN_TEST(test_get_executable_path);

    // get_file_size tests
    RUN_TEST(test_get_file_size_null_filename);
    RUN_TEST(test_get_file_size_nonexistent_file);

    // get_file_modification_time tests
    RUN_TEST(test_get_file_modification_time_null_filename);
    RUN_TEST(test_get_file_modification_time_nonexistent_file);

    return UNITY_END();
}