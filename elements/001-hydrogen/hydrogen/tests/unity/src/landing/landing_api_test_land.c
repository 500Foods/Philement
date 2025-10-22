/*
 * Unity Test File: API Landing Subsystem Tests
 * This file contains unit tests for the land_api_subsystem function
 * from src/landing/landing_api.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/registry/registry.h>
#include <src/logging/logging.h>
#include <src/api/api_service.h>
#include <src/registry/registry_integration.h>
#include <src/state/state_types.h>
#include <src/globals.h>

// Include mock header
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
int land_api_subsystem(void);

// Forward declarations for helper functions we'll need
// None needed for basic tests

// Forward declarations for test functions
void test_land_api_subsystem_already_shutdown(void);
void test_land_api_subsystem_successful_shutdown(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC LANDING TESTS =====

void test_land_api_subsystem_already_shutdown(void) {
    // Arrange: Mock API as not running
    mock_landing_set_api_running(false);

    // Act: Call the function
    int result = land_api_subsystem();

    // Assert: Should return 1 (success) when already shut down
    TEST_ASSERT_EQUAL(1, result);
}

void test_land_api_subsystem_successful_shutdown(void) {
    // Arrange: Mock API as running
    mock_landing_set_api_running(true);

    // Act: Call the function
    int result = land_api_subsystem();

    // Assert: Should return 1 (success) after shutdown
    TEST_ASSERT_EQUAL(1, result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic landing tests
    RUN_TEST(test_land_api_subsystem_already_shutdown);
    RUN_TEST(test_land_api_subsystem_successful_shutdown);

    return UNITY_END();
}