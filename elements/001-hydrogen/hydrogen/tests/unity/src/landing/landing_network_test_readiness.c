/*
 * Unity Test File: Network Landing Readiness Tests
 * This file contains unit tests for the check_network_landing_readiness function
 * from src/landing/landing_network.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"

// Include mock header
#include "../../../mocks/mock_landing.h"

// Forward declarations for functions being tested
LaunchReadiness check_network_landing_readiness(void);



// Forward declarations for test functions
void test_check_network_landing_readiness_success(void);
void test_check_network_landing_readiness_not_running(void);
void test_check_network_landing_readiness_malloc_failure(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
}

void tearDown(void) {
    // Clean up after each test
}

// ===== READINESS CHECK TESTS =====

void test_check_network_landing_readiness_success(void) {
    // Arrange: Network is running
    mock_landing_set_network_running(true);

    // Act: Call the function
    LaunchReadiness result = check_network_landing_readiness();

    // Assert: Should be ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Network", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_network_landing_readiness_not_running(void) {
    // Arrange: Network is not running
    mock_landing_set_network_running(false);

    // Act: Call the function
    LaunchReadiness result = check_network_landing_readiness();

    // Assert: Should not be ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Network", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_network_landing_readiness_malloc_failure(void) {
    // Arrange: Network is running
    mock_landing_set_network_running(true);

    // Act: Call the function
    LaunchReadiness result = check_network_landing_readiness();

    // Assert: Should be ready (malloc should work normally)
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);

    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Readiness check tests
    RUN_TEST(test_check_network_landing_readiness_success);
    RUN_TEST(test_check_network_landing_readiness_not_running);
    RUN_TEST(test_check_network_landing_readiness_malloc_failure);

    return UNITY_END();
}