/*
 * Unity Test File: Notify Launch Subsystem Test
 * This file contains unit tests for launch_notify_subsystem function
 * Uses zeroed config technique to test the main launch function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
int launch_notify_subsystem(void);

// Forward declarations for test functions
void test_launch_notify_subsystem_with_zeroed_config(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_launch_notify_subsystem_with_zeroed_config(void) {
    // Test the main launch function with zeroed config
    // Save original config
    AppConfig* original = app_config;

    // Create zeroed config
    AppConfig mock = {0};  // All fields zero/NULL
    app_config = &mock;

    // Test the launch function - should handle zeroed config gracefully
    int result = launch_notify_subsystem();

    // Restore original config
    app_config = original;

    // Should return failure (0) - Registry dependency not met in test environment
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_notify_subsystem_with_zeroed_config);

    return UNITY_END();
}
