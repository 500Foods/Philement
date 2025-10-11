/*
 * Unity Test File: landing_resources_test_check_resources_landing_readiness.c
 * This file contains unit tests for the check_resources_landing_readiness function
 * from src/landing/landing_resources.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>

// Forward declarations for functions being tested
LaunchReadiness check_resources_landing_readiness(void);

// Test function declarations
void test_check_resources_landing_readiness_subsystem_running(void);
void test_check_resources_landing_readiness_subsystem_not_running(void);

// Mock state
static bool mock_subsystem_running = true;

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_subsystem_running;
}

void setUp(void) {
    // Reset mock state
    mock_subsystem_running = true;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_resources_landing_readiness =====

void test_check_resources_landing_readiness_subsystem_running(void) {
    // Arrange
    mock_subsystem_running = true;

    // Act
    LaunchReadiness result = check_resources_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING("Resources", result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Resources", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Resources subsystem is running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Resources", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_resources_landing_readiness_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_running = false;

    // Act
    LaunchReadiness result = check_resources_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING("Resources", result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Resources", result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Resources not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Resources", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_resources_landing_readiness_subsystem_running);
    RUN_TEST(test_check_resources_landing_readiness_subsystem_not_running);

    return UNITY_END();
}