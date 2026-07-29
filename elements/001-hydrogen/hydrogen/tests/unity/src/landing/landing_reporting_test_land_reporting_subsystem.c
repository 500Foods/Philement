/*
 * Unity Test File: landing_reporting_test_land_reporting_subsystem.c
 * This file contains unit tests for the land_reporting_subsystem function
 * from src/landing/landing_reporting.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>

// Forward declarations for functions being tested
int land_reporting_subsystem(void);

// Test function declarations
void test_land_reporting_subsystem_null_config(void);
void test_land_reporting_subsystem_disabled(void);
void test_land_reporting_subsystem_enabled(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// ===== TESTS FOR land_reporting_subsystem =====

void test_land_reporting_subsystem_null_config(void) {
    // Test with NULL app_config - should return success (1, skip)
    AppConfig* original = app_config;
    app_config = NULL;

    int result = land_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

void test_land_reporting_subsystem_disabled(void) {
    // Test with reporting disabled - should return success (1, skip)
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = false;
    app_config = &mock;

    int result = land_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

void test_land_reporting_subsystem_enabled(void) {
    // Test with reporting enabled - should return success (1, landed)
    AppConfig* original = app_config;
    AppConfig mock = {0};
    mock.reporting.Enabled = true;
    app_config = &mock;

    int result = land_reporting_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_reporting_subsystem_null_config);
    RUN_TEST(test_land_reporting_subsystem_disabled);
    RUN_TEST(test_land_reporting_subsystem_enabled);

    return UNITY_END();
}
