/*
 * Unity Test File: utils_dependency_test_parsing.c
 *
 * Tests for database version parsing functions in utils_dependency.c
 * Note: Since parsing functions are static, we test them indirectly through
 * the public interface that uses them.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_database_version_checking_integration(void);
void test_library_availability_integration(void);
void test_library_loading_integration(void);

void setUp(void) {
    // No setup required for integration tests
}

void tearDown(void) {
    // No cleanup required for integration tests
}

// Integration test for database version checking
void test_database_version_checking_integration(void) {
    // Test that database version checking works through the public interface
    // This indirectly tests the parsing functions
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // This should complete without crashing and test the parsing indirectly
    int result = check_library_dependencies(&config);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
}

// Test library availability checking
void test_library_availability_integration(void) {
    // Test that library availability checking works
    bool libc_available = is_library_available("libc.so.6");
    bool nonexistent_available = is_library_available("lib_nonexistent_xyz.so");

    // libc should typically be available, nonexistent should not be
    // We mainly test that the function doesn't crash and returns consistent results
    bool repeated_result = is_library_available("libc.so.6");
    TEST_ASSERT_EQUAL(libc_available, repeated_result);
    TEST_ASSERT_FALSE(nonexistent_available);
}

// Test library loading integration
void test_library_loading_integration(void) {
    // Test library loading through public interface
    LibraryHandle *handle = load_library("libc.so.6", RTLD_LAZY);
    if (handle) {
        TEST_ASSERT_TRUE(handle->is_loaded);
        TEST_ASSERT_NOT_NULL(handle->name);

        // Test function retrieval - function may or may not be available, but call should not crash
        (void)get_library_function(handle, "printf");  // Call should not crash

        // Clean up
        bool unload_result = unload_library(handle);
        TEST_ASSERT_TRUE(unload_result);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_version_checking_integration);
    RUN_TEST(test_library_availability_integration);
    RUN_TEST(test_library_loading_integration);

    return UNITY_END();
}