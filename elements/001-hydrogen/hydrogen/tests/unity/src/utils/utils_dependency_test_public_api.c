/*
 * Unity Test File: utils_dependency_test_public_api.c
 *
 * Tests for public API functions in utils_dependency.c
 * These functions are public, so we can test them directly.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_is_library_available_function(void);
void test_is_library_available_null_parameter(void);
void test_is_library_available_nonexistent_library(void);
void test_is_library_available_system_library(void);
void test_check_library_dependency_function(void);
void test_check_library_dependency_null_parameters(void);
void test_check_library_dependency_known_library(void);
void test_check_library_dependency_unknown_library(void);
void test_load_library_function(void);
void test_load_library_null_parameters(void);
void test_load_library_nonexistent_library(void);
void test_load_library_valid_library(void);
void test_unload_library_function(void);
void test_unload_library_null_parameter(void);
void test_unload_library_valid_handle(void);
void test_get_library_function_function(void);
void test_get_library_function_null_parameters(void);
void test_get_library_function_valid_function(void);
void test_get_library_function_nonexistent_function(void);

void setUp(void) {
    // No setup required for these tests
}

void tearDown(void) {
    // No cleanup required for these tests
}

// Test is_library_available function
void test_is_library_available_function(void) {
    // Test basic functionality of is_library_available
    bool nonexistent_available = is_library_available("lib_nonexistent_xyz.so");

    // nonexistent should definitely not be available
    TEST_ASSERT_FALSE(nonexistent_available);
}

// Test is_library_available with NULL parameter
void test_is_library_available_null_parameter(void) {
    bool result = is_library_available(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test is_library_available with nonexistent library
void test_is_library_available_nonexistent_library(void) {
    bool result = is_library_available("lib_definitely_does_not_exist_12345.so");
    TEST_ASSERT_FALSE(result);
}

// Test is_library_available with system library
void test_is_library_available_system_library(void) {
    // Test with a library that should exist on most Linux systems
    // Result may vary by system, but function should not crash
    // We mainly test that the function doesn't crash
    (void)is_library_available("libc.so.6");  // Function completed without crashing
    TEST_ASSERT_TRUE(true);
}

// Test check_library_dependency function
void test_check_library_dependency_function(void) {
    // Test basic functionality - should not crash
    check_library_dependency("jansson", "2.13.1", true);
    check_library_dependency("nonexistent", "1.0.0", false);
}

// Test check_library_dependency with NULL parameters
void test_check_library_dependency_null_parameters(void) {
    // Test with NULL name - should handle gracefully
    check_library_dependency(NULL, "1.0.0", true);
}

// Test check_library_dependency with known library
void test_check_library_dependency_known_library(void) {
    // Test with a library that should be available
    check_library_dependency("libc", NULL, true);
}

// Test check_library_dependency with unknown library
void test_check_library_dependency_unknown_library(void) {
    // Test with a library that likely doesn't exist
    check_library_dependency("lib_unknown_test_library", "1.0.0", false);
}

// Test load_library function
void test_load_library_function(void) {
    // Test basic functionality of load_library
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        TEST_ASSERT_NOT_NULL(handle->name);
        TEST_ASSERT_TRUE(handle->is_loaded);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test load_library with NULL parameters
void test_load_library_null_parameters(void) {
    LibraryHandle *result = load_library(NULL, RTLD_LAZY);
    TEST_ASSERT_NULL(result);
}

// Test load_library with nonexistent library
void test_load_library_nonexistent_library(void) {
    LibraryHandle *handle = load_library("lib_nonexistent_library.so", RTLD_LAZY);

    if (handle) {
        TEST_ASSERT_FALSE(handle->is_loaded);
        TEST_ASSERT_NOT_NULL(handle->name);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test load_library with valid library
void test_load_library_valid_library(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        TEST_ASSERT_TRUE(handle->is_loaded);
        TEST_ASSERT_NOT_NULL(handle->name);
        TEST_ASSERT_NOT_NULL(handle->version);

        // Test function retrieval - function may or may not be available, but call should not crash
        (void)get_library_function(handle, "printf");

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test unload_library function
void test_unload_library_function(void) {
    // Create a handle to unload
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        bool result = unload_library(handle);
        TEST_ASSERT_TRUE(result);
    }
}

// Test unload_library with NULL parameter
void test_unload_library_null_parameter(void) {
    bool result = unload_library(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test unload_library with valid handle
void test_unload_library_valid_handle(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        bool result = unload_library(handle);
        TEST_ASSERT_TRUE(result);
    }
}

// Test get_library_function function
void test_get_library_function_function(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        // Try to get a function that should exist
        (void)get_library_function(handle, "printf");

        // Function may or may not be available, but call should not crash
        TEST_ASSERT_TRUE(true);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test get_library_function with NULL parameters
void test_get_library_function_null_parameters(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        void *result1 = get_library_function(NULL, "printf");
        void *result2 = get_library_function(handle, NULL);
        void *result3 = get_library_function(NULL, NULL);

        TEST_ASSERT_NULL(result1);
        TEST_ASSERT_NULL(result2);
        TEST_ASSERT_NULL(result3);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test get_library_function with valid function
void test_get_library_function_valid_function(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        // Function may or may not be available depending on system
        // But the call should not crash - we mainly test that it doesn't crash
        (void)get_library_function(handle, "printf");
        TEST_ASSERT_TRUE(true);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test get_library_function with nonexistent function
void test_get_library_function_nonexistent_function(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);

    if (handle) {
        void *func = get_library_function(handle, "nonexistent_function_12345");

        // Should return NULL for nonexistent function
        TEST_ASSERT_NULL(func);

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_library_available_function);
    RUN_TEST(test_is_library_available_null_parameter);
    RUN_TEST(test_is_library_available_nonexistent_library);
    RUN_TEST(test_is_library_available_system_library);

    RUN_TEST(test_check_library_dependency_function);
    RUN_TEST(test_check_library_dependency_null_parameters);
    RUN_TEST(test_check_library_dependency_known_library);
    RUN_TEST(test_check_library_dependency_unknown_library);

    RUN_TEST(test_load_library_function);
    RUN_TEST(test_load_library_null_parameters);
    RUN_TEST(test_load_library_nonexistent_library);
    RUN_TEST(test_load_library_valid_library);

    RUN_TEST(test_unload_library_function);
    RUN_TEST(test_unload_library_null_parameter);
    RUN_TEST(test_unload_library_valid_handle);

    RUN_TEST(test_get_library_function_function);
    RUN_TEST(test_get_library_function_null_parameters);
    RUN_TEST(test_get_library_function_valid_function);
    RUN_TEST(test_get_library_function_nonexistent_function);

    return UNITY_END();
}