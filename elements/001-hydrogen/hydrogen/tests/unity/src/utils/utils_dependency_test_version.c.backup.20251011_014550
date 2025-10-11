/*
 * Unity Test File: utils_dependency_test_version.c
 *
 * Tests for library dependency checking integration in utils_dependency.c
 * Note: Since many helper functions are static, we test them indirectly through
 * the public interface that uses them.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_library_dependency_checking_integration(void);
void test_library_loading_unloading_integration(void);
void test_configuration_dependent_checking(void);

void setUp(void) {
    // No setup required for integration tests
}

void tearDown(void) {
    // No cleanup required for integration tests
}

// Integration test for library dependency checking
void test_library_dependency_checking_integration(void) {
    // Test that library dependency checking works through the public interface
    // This indirectly tests get_version, determine_status, and other helper functions
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // This should complete without crashing and test the version detection indirectly
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Integration test for library loading/unloading
void test_library_loading_unloading_integration(void) {
    // Test library loading and unloading through public interface
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        TEST_ASSERT_TRUE(handle->is_loaded);

        // Test function retrieval - function may or may not be available, but call should not crash
        (void)get_library_function(handle, "printf");  // Call should not crash

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

// Test configuration-dependent library checking
void test_configuration_dependent_checking(void) {
    // Test with different configurations to exercise different code paths
    AppConfig config1, config2;
    memset(&config1, 0, sizeof(AppConfig));
    memset(&config2, 0, sizeof(AppConfig));

    // Configure different scenarios
    config1.webserver.enable_ipv4 = true;
    config2.print.enabled = true;

    // Both should complete without crashing
    int result1 = check_library_dependencies(&config1);
    int result2 = check_library_dependencies(&config2);

    TEST_ASSERT_GREATER_OR_EQUAL(0, result1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_library_dependency_checking_integration);
    RUN_TEST(test_library_loading_unloading_integration);
    RUN_TEST(test_configuration_dependent_checking);

    return UNITY_END();
}