/*
 * Unity Test File: Reporting Launch Subsystem Tests
 * This file contains unit tests for launch_reporting_subsystem function
 * from src/launch/launch_reporting.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
int launch_reporting_subsystem(void);

// Forward declarations for test functions
void test_launch_reporting_subsystem_null_config(void);
void test_launch_reporting_subsystem_disabled(void);
void test_launch_reporting_subsystem_enabled(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_launch_reporting_subsystem_null_config(void) {
    // Test with NULL app_config - should return failure (0)
    AppConfig* original = app_config;
    app_config = NULL;

    int result = launch_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(0, result);
}

void test_launch_reporting_subsystem_disabled(void) {
    // Test with reporting disabled - should return success (1, skip)
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = false;
    app_config = &mock;

    int result = launch_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

void test_launch_reporting_subsystem_enabled(void) {
    // Test with reporting enabled - should return success (1, launched)
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = true;
    app_config = &mock;

    int result = launch_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_reporting_subsystem_null_config);
    RUN_TEST(test_launch_reporting_subsystem_disabled);
    RUN_TEST(test_launch_reporting_subsystem_enabled);

    return UNITY_END();
}
