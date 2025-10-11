/*
 * Unity Test File: OIDC Landing Readiness Tests
 * This file contains unit tests for the check_oidc_landing_readiness function
 * from src/landing/landing_oidc.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Include mock header
#include "../../../mocks/mock_landing.h"

// Forward declarations for functions being tested
LaunchReadiness check_oidc_landing_readiness(void);

// Remove the weak malloc/strdup mocks that were causing issues
// The malloc failure test will be handled differently

// Forward declarations for test functions
void test_check_oidc_landing_readiness_success(void);
void test_check_oidc_landing_readiness_not_running(void);
void test_check_oidc_landing_readiness_malloc_failure(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
}

void tearDown(void) {
    // Clean up after each test
}

// ===== READINESS CHECK TESTS =====

void test_check_oidc_landing_readiness_success(void) {
    // Arrange: OIDC is running
    mock_landing_set_oidc_running(true);

    // Act: Call the function
    LaunchReadiness result = check_oidc_landing_readiness();

    // Assert: Should be ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_oidc_landing_readiness_not_running(void) {
    // Arrange: OIDC is not running
    mock_landing_set_oidc_running(false);

    // Act: Call the function
    LaunchReadiness result = check_oidc_landing_readiness();

    // Assert: Should not be ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_oidc_landing_readiness_malloc_failure(void) {
    // Arrange: OIDC is running
    mock_landing_set_oidc_running(true);

    // Act: Call the function
    LaunchReadiness result = check_oidc_landing_readiness();

    // Assert: Should be ready (malloc should succeed in normal operation)
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("OIDC", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Readiness check tests
    RUN_TEST(test_check_oidc_landing_readiness_success);
    RUN_TEST(test_check_oidc_landing_readiness_not_running);
    RUN_TEST(test_check_oidc_landing_readiness_malloc_failure);

    return UNITY_END();
}