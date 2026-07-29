/*
 * Unity Test File: Reporting Launch Readiness Check Tests
 * This file contains unit tests for check_reporting_launch_readiness function
 * from src/launch/launch_reporting.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
LaunchReadiness check_reporting_launch_readiness(void);

// Forward declarations for test functions
void test_check_reporting_launch_readiness_null_config(void);
void test_check_reporting_launch_readiness_disabled(void);
void test_check_reporting_launch_readiness_enabled(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_check_reporting_launch_readiness_null_config(void) {
    // Test with NULL app_config - should report not ready (no config available)
    AppConfig* original = app_config;
    app_config = NULL;

    LaunchReadiness result = check_reporting_launch_readiness();

    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_REPORTING, result.subsystem);
    TEST_ASSERT_NULL(result.messages);
}

void test_check_reporting_launch_readiness_disabled(void) {
    // Test with reporting disabled - should report ready
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = false;
    app_config = &mock;

    LaunchReadiness result = check_reporting_launch_readiness();

    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_REPORTING, result.subsystem);
    TEST_ASSERT_NULL(result.messages);
}

void test_check_reporting_launch_readiness_enabled(void) {
    // Test with reporting enabled - should report ready
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = true;
    app_config = &mock;

    LaunchReadiness result = check_reporting_launch_readiness();

    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_REPORTING, result.subsystem);
    TEST_ASSERT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_reporting_launch_readiness_null_config);
    RUN_TEST(test_check_reporting_launch_readiness_disabled);
    RUN_TEST(test_check_reporting_launch_readiness_enabled);

    return UNITY_END();
}
