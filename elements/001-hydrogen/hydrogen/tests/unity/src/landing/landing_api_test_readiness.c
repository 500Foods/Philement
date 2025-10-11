/*
 * Unity Test File: API Landing Readiness Tests
 * This file contains unit tests for the check_api_landing_readiness function
 * from src/landing/landing_api.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/registry/registry.h>
#include <src/logging/logging.h>
#include <src/api/api_service.h>
#include <src/registry/registry_integration.h>
#include <src/state/state_types.h>
#include <src/globals.h>

// Include mock header
#include "../../../mocks/mock_landing.h"

// Forward declarations for functions being tested
LaunchReadiness check_api_landing_readiness(void);

// Forward declarations for helper functions we'll need
// None needed for basic tests

// Forward declarations for test functions
void test_check_api_landing_readiness_api_not_running(void);
void test_check_api_landing_readiness_webserver_not_running(void);
void test_check_api_landing_readiness_both_running(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC READINESS TESTS =====

void test_check_api_landing_readiness_api_not_running(void) {
    // Arrange: Mock API as not running, WebServer as running
    mock_landing_set_api_running(false);
    mock_landing_set_webserver_running(true);

    // Act: Call the function
    LaunchReadiness result = check_api_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_API, result.subsystem);
    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_api_landing_readiness_webserver_not_running(void) {
    // Arrange: Mock API as running, WebServer as not running
    mock_landing_set_api_running(true);
    mock_landing_set_webserver_running(false);

    // Act: Call the function
    LaunchReadiness result = check_api_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_API, result.subsystem);
    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_api_landing_readiness_both_running(void) {
    // Arrange: Mock both API and WebServer as running
    mock_landing_set_api_running(true);
    mock_landing_set_webserver_running(true);

    // Act: Call the function
    LaunchReadiness result = check_api_landing_readiness();

    // Assert: Should return ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_API, result.subsystem);
    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic readiness tests
    RUN_TEST(test_check_api_landing_readiness_api_not_running);
    RUN_TEST(test_check_api_landing_readiness_webserver_not_running);
    if (0) RUN_TEST(test_check_api_landing_readiness_both_running);

    return UNITY_END();
}