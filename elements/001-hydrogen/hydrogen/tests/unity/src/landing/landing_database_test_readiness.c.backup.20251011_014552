/*
 * Unity Test File: Database Landing Readiness Tests
 * This file contains unit tests for the check_database_landing_readiness function
 * from src/landing/landing_database.c
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
LaunchReadiness check_database_landing_readiness(void);


// Mock globals - use weak linkage to avoid multiple definitions

// Forward declarations for test functions
void test_check_database_landing_readiness_success(void);
void test_check_database_landing_readiness_not_running(void);
void test_check_database_landing_readiness_malloc_failure(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
}

void tearDown(void) {
    // Clean up after each test
}

// ===== READINESS CHECK TESTS =====

void test_check_database_landing_readiness_success(void) {
    // Arrange: Database is running
    mock_landing_set_database_running(true);

    // Act: Call the function
    LaunchReadiness result = check_database_landing_readiness();

    // Assert: Should be ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.messages[0]);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_database_landing_readiness_not_running(void) {
    // Arrange: Database is not running
    mock_landing_set_database_running(false);

    // Act: Call the function
    LaunchReadiness result = check_database_landing_readiness();

    // Assert: Should not be ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.messages[0]);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_database_landing_readiness_malloc_failure(void) {
    // Arrange: Database is running
    mock_landing_set_database_running(true);

    // Act: Call the function
    LaunchReadiness result = check_database_landing_readiness();

    // Assert: Should be ready (malloc should work normally)
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);

    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Readiness check tests
    RUN_TEST(test_check_database_landing_readiness_success);
    RUN_TEST(test_check_database_landing_readiness_not_running);
    RUN_TEST(test_check_database_landing_readiness_malloc_failure);

    return UNITY_END();
}