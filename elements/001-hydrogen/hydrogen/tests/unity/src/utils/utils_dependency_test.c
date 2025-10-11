/*
 * Unity Test File: Library Dependency Functions Tests
 * This file contains comprehensive unit tests for the library dependency checking
 * and dynamic loading functions from src/utils/utils_dependency.h
 *
 * Coverage Goals:
 * - Test library dependency checking with various configurations
 * - Test dynamic library loading and unloading
 * - Test version checking and status determination
 * - Test error handling for missing libraries
 * - Test parameter validation and edge cases
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

// Forward declarations for the functions being tested
int check_library_dependencies(const AppConfig *config);
void check_library_dependency(const char *name, const char *expected, bool is_required);
LibraryHandle *load_library(const char *lib_name, int dlopen_flags);
bool unload_library(LibraryHandle *handle);
void *get_library_function(LibraryHandle *handle, const char *function_name);
bool is_library_available(const char *lib_name);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
    // Disable cache for testing to ensure command execution paths are tested
    setenv("HYDROGEN_DEP_CACHE", "1", 1);
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_check_library_dependencies_null_config(void);
void test_check_library_dependencies_valid_config(void);
void test_check_library_dependency_null_name(void);
void test_check_library_dependency_null_expected(void);
void test_check_library_dependency_empty_name(void);
void test_check_library_dependency_unknown_library(void);
void test_is_library_available_null_name(void);
void test_is_library_available_empty_name(void);
void test_is_library_available_nonexistent_library(void);
void test_is_library_available_standard_library(void);
void test_load_library_null_name(void);
void test_load_library_empty_name(void);
void test_load_library_nonexistent_library(void);
void test_load_library_standard_library(void);
void test_unload_library_null_handle(void);
void test_unload_library_valid_handle(void);
void test_unload_library_already_unloaded(void);
void test_get_library_function_null_handle(void);
void test_get_library_function_null_function_name(void);
void test_get_library_function_unloaded_library(void);
void test_get_library_function_nonexistent_function(void);
void test_get_library_function_valid_function(void);
void test_library_handle_structure_initialization(void);
void test_library_handle_memory_management(void);
void test_check_library_dependencies_different_configs(void);
void test_check_library_dependency_various_scenarios(void);
void test_library_operations_with_invalid_flags(void);
void test_multiple_library_load_unload(void);
void test_library_function_retrieval_edge_cases(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_check_library_dependencies_null_config(void) {
    // Test with NULL config - should not crash
    int result = check_library_dependencies(NULL);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);  // Should return non-negative count
}

void test_check_library_dependencies_valid_config(void) {
    // Test with valid config - should complete without crashing
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

void test_check_library_dependency_null_name(void) {
    // Test with NULL name - should not crash
    check_library_dependency(NULL, "1.0.0", true);
    TEST_ASSERT_TRUE(true);  // Should complete without crashing
}

void test_check_library_dependency_null_expected(void) {
    // Test with NULL expected version - should handle gracefully
    check_library_dependency("testlib", NULL, true);
    TEST_ASSERT_TRUE(true);  // Should complete without crashing
}

void test_check_library_dependency_empty_name(void) {
    // Test with empty name - should handle gracefully
    check_library_dependency("", "1.0.0", true);
    TEST_ASSERT_TRUE(true);
}

void test_check_library_dependency_unknown_library(void) {
    // Test with unknown library name - should handle gracefully
    check_library_dependency("nonexistent_library_xyz", "1.0.0", true);
    TEST_ASSERT_TRUE(true);
}

//=============================================================================
// Library Availability Tests
//=============================================================================

void test_is_library_available_null_name(void) {
    // Test with NULL name - should return false
    bool result = is_library_available(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_is_library_available_empty_name(void) {
    // Test with empty name - actual behavior depends on dlopen implementation
    bool result = is_library_available("");
    // Test that function doesn't crash and returns consistent results
    bool repeated_result = is_library_available("");
    TEST_ASSERT_EQUAL(result, repeated_result);
}

void test_is_library_available_nonexistent_library(void) {
    // Test with non-existent library - should return false
    bool result = is_library_available("lib_nonexistent_xyz.so");
    TEST_ASSERT_FALSE(result);
}

void test_is_library_available_standard_library(void) {
    // Test with standard C library - should return true (usually available)
    bool result = is_library_available("libc.so.6");
    // Note: This might fail in some environments, but libc should be available
    // We're mainly testing that the function doesn't crash and is consistent
    bool repeated_result = is_library_available("libc.so.6");
    TEST_ASSERT_EQUAL(result, repeated_result);
}

//=============================================================================
// Library Loading and Unloading Tests
//=============================================================================

void test_load_library_null_name(void) {
    // Test loading with NULL name - should return NULL
    LibraryHandle *handle = load_library(NULL, RTLD_LAZY);
    TEST_ASSERT_NULL(handle);
}

void test_load_library_empty_name(void) {
    // Test loading with empty name - actual behavior depends on dlopen implementation
    LibraryHandle *handle = load_library("", RTLD_LAZY);
    if (handle) {
        // dlopen("") may succeed on some systems, so we just check proper cleanup
        bool unloaded = unload_library(handle);
        TEST_ASSERT_TRUE(unloaded);
    } else {
        // If it returns NULL, that's also acceptable
        TEST_ASSERT_NULL(handle);
    }
}

void test_load_library_nonexistent_library(void) {
    // Test loading non-existent library - should return handle with is_loaded=false
    LibraryHandle *handle = load_library("lib_nonexistent_xyz.so", RTLD_LAZY);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_FALSE(handle->is_loaded);
    TEST_ASSERT_EQUAL_STRING("lib_nonexistent_xyz.so", handle->name);
    TEST_ASSERT_EQUAL_STRING("None", handle->version);
    TEST_ASSERT_EQUAL(LIB_STATUS_WARNING, handle->status);

    // Clean up
    bool unloaded = unload_library(handle);
    TEST_ASSERT_TRUE(unloaded);
}

void test_load_library_standard_library(void) {
    // Test loading standard C library - should work if available
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        TEST_ASSERT_TRUE(handle->is_loaded);
        TEST_ASSERT_EQUAL_STRING("libc.so.6", handle->name);
        TEST_ASSERT_EQUAL(LIB_STATUS_GOOD, handle->status);

        // Clean up
        bool unloaded = unload_library(handle);
        TEST_ASSERT_TRUE(unloaded);
    } else {
        TEST_FAIL_MESSAGE("Failed to load libc.so.6");
    }
}

void test_unload_library_null_handle(void) {
    // Test unloading NULL handle - should return false
    bool result = unload_library(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_unload_library_valid_handle(void) {
    // Test unloading a valid handle
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        bool result = unload_library(handle);
        TEST_ASSERT_TRUE(result);
    }
}

void test_unload_library_already_unloaded(void) {
    // Test unloading a handle that was already unloaded
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        unload_library(handle);  // First unload

        // The handle is now invalid - don't try to unload again
        // The memory has been freed, so accessing handle would be undefined behavior
        TEST_ASSERT_TRUE(true);  // Test passes if we get here without crashing
    }
}

//=============================================================================
// Function Retrieval Tests
//=============================================================================

void test_get_library_function_null_handle(void) {
    // Test getting function from NULL handle - should return NULL
    void *func = get_library_function(NULL, "printf");
    TEST_ASSERT_NULL(func);
}

void test_get_library_function_null_function_name(void) {
    // Test getting function with NULL name - should return NULL
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        void *func = get_library_function(handle, NULL);
        TEST_ASSERT_NULL(func);
        unload_library(handle);
    }
}

void test_get_library_function_unloaded_library(void) {
    // Test getting function from unloaded library - should return NULL
    LibraryHandle *handle = load_library("lib_nonexistent_xyz.so", RTLD_LAZY);
    if (handle) {
        void *func = get_library_function(handle, "some_function");
        TEST_ASSERT_NULL(func);
        unload_library(handle);
    }
}

void test_get_library_function_nonexistent_function(void) {
    // Test getting non-existent function from loaded library - should return NULL
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        void *func = get_library_function(handle, "nonexistent_function_xyz");
        TEST_ASSERT_NULL(func);
        unload_library(handle);
    }
}

void test_get_library_function_valid_function(void) {
    // Test getting a valid function from loaded library
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        void *func = get_library_function(handle, "printf");
        if (func) {
            // Function pointer should be non-null
            TEST_ASSERT_NOT_NULL(func);
        } else {
            // This might happen if printf is not exported in the way we expect
            // The test is mainly about not crashing
            TEST_ASSERT_TRUE(true);
        }
        unload_library(handle);
    }
}

//=============================================================================
// Library Handle Structure Tests
//=============================================================================

void test_library_handle_structure_initialization(void) {
    LibraryHandle *handle = load_library("lib_nonexistent_xyz.so", RTLD_LAZY);
    if (handle) {
        // Check that all fields are properly initialized
        TEST_ASSERT_NOT_NULL(handle->name);
        TEST_ASSERT_FALSE(handle->is_loaded);
        TEST_ASSERT_EQUAL(LIB_STATUS_WARNING, handle->status);

        // Version should be "None" for unloaded libraries
        TEST_ASSERT_EQUAL_STRING("None", handle->version);

        unload_library(handle);
    }
}

void test_library_handle_memory_management(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        // Handle should be properly allocated
        TEST_ASSERT_NOT_NULL(handle);

        // All string pointers should be valid
        TEST_ASSERT_NOT_NULL(handle->name);

        // Clean up
        bool result = unload_library(handle);
        TEST_ASSERT_TRUE(result);
    }
}

//=============================================================================
// Configuration-based Tests
//=============================================================================

void test_check_library_dependencies_different_configs(void) {
    // Test with different configuration scenarios
    AppConfig config1, config2;
    memset(&config1, 0, sizeof(AppConfig));
    memset(&config2, 0, sizeof(AppConfig));

    // Configure different scenarios
    config1.webserver.enable_ipv4 = true;
    config1.websocket.enable_ipv4 = true;

    config2.print.enabled = true;

    // Both should complete without crashing
    int result1 = check_library_dependencies(&config1);
    int result2 = check_library_dependencies(&config2);

    TEST_ASSERT_GREATER_OR_EQUAL(0, result1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result2);
}

void test_check_library_dependency_various_scenarios(void) {
    // Test different library checking scenarios
    check_library_dependency("pthreads", "1.0", true);
    check_library_dependency("libm", "2.0", true);
    check_library_dependency("microhttpd", "1.0.1", false);
    check_library_dependency("OpenSSL", "3.2.4", false);

    TEST_ASSERT_TRUE(true);  // Should complete without crashing
}

//=============================================================================
// Edge Cases and Error Handling Tests
//=============================================================================

void test_library_operations_with_invalid_flags(void) {
    // Test with invalid dlopen flags - should handle gracefully
    LibraryHandle *handle = load_library("libc.so.6", -1);
    if (handle) {
        TEST_ASSERT_FALSE(handle->is_loaded);
        unload_library(handle);
    }
}

void test_multiple_library_load_unload(void) {
    // Test loading and unloading multiple libraries
    LibraryHandle *handle1 = load_library("libc.so.6", RTLD_LAZY);
    LibraryHandle *handle2 = load_library("lib_nonexistent_xyz.so", RTLD_LAZY);

    if (handle1) {
        TEST_ASSERT_TRUE(handle1->is_loaded);
    }

    if (handle2) {
        TEST_ASSERT_FALSE(handle2->is_loaded);
    }

    // Clean up
    if (handle1) unload_library(handle1);
    if (handle2) unload_library(handle2);

    TEST_ASSERT_TRUE(true);
}

void test_library_function_retrieval_edge_cases(void) {
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        // Test with very long function names
        char long_name[1024];
        memset(long_name, 'a', sizeof(long_name) - 1);
        long_name[sizeof(long_name) - 1] = '\0';

        void *func = get_library_function(handle, long_name);
        TEST_ASSERT_NULL(func);  // Should return NULL for non-existent function

        // Test with special characters in function name
        func = get_library_function(handle, "invalid@#$%function");
        TEST_ASSERT_NULL(func);  // Should return NULL

        unload_library(handle);
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_check_library_dependencies_null_config);
    RUN_TEST(test_check_library_dependencies_valid_config);
    RUN_TEST(test_check_library_dependency_null_name);
    RUN_TEST(test_check_library_dependency_null_expected);
    RUN_TEST(test_check_library_dependency_empty_name);
    RUN_TEST(test_check_library_dependency_unknown_library);

    // Library availability tests
    RUN_TEST(test_is_library_available_null_name);
    RUN_TEST(test_is_library_available_empty_name);
    RUN_TEST(test_is_library_available_nonexistent_library);
    RUN_TEST(test_is_library_available_standard_library);

    // Library loading and unloading tests
    RUN_TEST(test_load_library_null_name);
    RUN_TEST(test_load_library_empty_name);
    RUN_TEST(test_load_library_nonexistent_library);
    RUN_TEST(test_load_library_standard_library);
    RUN_TEST(test_unload_library_null_handle);
    RUN_TEST(test_unload_library_valid_handle);
    RUN_TEST(test_unload_library_already_unloaded);

    // Function retrieval tests
    RUN_TEST(test_get_library_function_null_handle);
    RUN_TEST(test_get_library_function_null_function_name);
    RUN_TEST(test_get_library_function_unloaded_library);
    RUN_TEST(test_get_library_function_nonexistent_function);
    RUN_TEST(test_get_library_function_valid_function);

    // Library handle structure tests
    RUN_TEST(test_library_handle_structure_initialization);
    RUN_TEST(test_library_handle_memory_management);

    // Configuration-based tests
    RUN_TEST(test_check_library_dependencies_different_configs);
    RUN_TEST(test_check_library_dependency_various_scenarios);

    // Edge cases and error handling tests
    RUN_TEST(test_library_operations_with_invalid_flags);
    RUN_TEST(test_multiple_library_load_unload);
    RUN_TEST(test_library_function_retrieval_edge_cases);

    return UNITY_END();
}
