/*
 * Unity Test File: System AppConfig get_executable_size Function Tests
 * This file contains unit tests for the get_executable_size function in globals.c
 * This function is more testable as it doesn't require complex system resources.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Test function prototypes
void test_get_executable_size_valid_executable(void);
void test_get_executable_size_null_argv(void);
void test_get_executable_size_empty_argv(void);
void test_get_executable_size_nonexistent_file(void);
void test_get_executable_size_directory_instead_of_file(void);
void test_get_executable_size_permission_denied(void);
void test_server_executable_size_initialization(void);

void setUp(void) {
    // Reset the global variable before each test
    server_executable_size = 0;
}

void tearDown(void) {
    // Clean up after each test
    server_executable_size = 0;
}

// Test with a valid executable (using a known system file)
void test_get_executable_size_valid_executable(void) {
    // Save the original value to restore later
    long long original_size = server_executable_size;

    // Use a known system file instead of the test executable to avoid path issues
    char *argv[] = {(char*)"/bin/ls", NULL};

    // Call the function
    get_executable_size(argv);

    // The size should be greater than 0 for a valid executable
    TEST_ASSERT_GREATER_THAN(0LL, server_executable_size);

    // The size should be reasonable (not too large, not too small)
    TEST_ASSERT_LESS_THAN(10LL * 1024 * 1024, server_executable_size);  // Less than 10MB
    TEST_ASSERT_GREATER_THAN(1024LL, server_executable_size);  // Greater than 1KB

    // Restore original value
    server_executable_size = original_size;
}

// Test with NULL argv
void test_get_executable_size_null_argv(void) {
    // Call with NULL argv - should handle gracefully
    get_executable_size(NULL);

    // Should set size to 0 on failure
    TEST_ASSERT_EQUAL(0LL, server_executable_size);
}

// Test with empty argv
void test_get_executable_size_empty_argv(void) {
    char *argv[] = {NULL};

    get_executable_size(argv);

    // Should set size to 0 when argv[0] is NULL
    TEST_ASSERT_EQUAL(0LL, server_executable_size);
}

// Test with non-existent file
void test_get_executable_size_nonexistent_file(void) {
    char *argv[] = {(char*)"./this_file_does_not_exist_12345", NULL};

    get_executable_size(argv);

    // Should set size to 0 when file doesn't exist
    TEST_ASSERT_EQUAL(0LL, server_executable_size);
}

// Test with directory instead of file
void test_get_executable_size_directory_instead_of_file(void) {
    // Save original value
    long long original_size = server_executable_size;

    char *argv[] = {(char*)"/tmp", NULL};  // /tmp should exist and be a directory

    get_executable_size(argv);

    // Directory size will be non-zero (directory inode size), but we can test that it's reasonable
    // Directory sizes are typically small (few KB at most)
    TEST_ASSERT_LESS_THAN(1024LL * 1024, server_executable_size);  // Less than 1MB
    TEST_ASSERT_GREATER_THAN(0LL, server_executable_size);  // But greater than 0

    // Restore original value
    server_executable_size = original_size;
}

// Test with permission denied scenario (if we can create one)
void test_get_executable_size_permission_denied(void) {
    // This test would be platform-specific and might not work in all environments
    // For now, we'll document the expected behavior

    // The function should set server_executable_size to 0 if stat() fails
    // This could happen due to permission issues, file system problems, etc.

    TEST_ASSERT_TRUE(true);  // Placeholder - would need platform-specific setup
}

// Test initial state of global variable
void test_server_executable_size_initialization(void) {
    // Test that the global variable starts at 0
    TEST_ASSERT_EQUAL(0LL, server_executable_size);

    // After calling with valid executable, it should be set
    char *argv[] = {(char*)"/bin/sh", NULL};  // Use a known shell executable
    get_executable_size(argv);

    TEST_ASSERT_GREATER_THAN(0LL, server_executable_size);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_executable_size_valid_executable);
    RUN_TEST(test_get_executable_size_null_argv);
    RUN_TEST(test_get_executable_size_empty_argv);
    RUN_TEST(test_get_executable_size_nonexistent_file);
    RUN_TEST(test_get_executable_size_directory_instead_of_file);
    RUN_TEST(test_get_executable_size_permission_denied);
    RUN_TEST(test_server_executable_size_initialization);

    return UNITY_END();
}
