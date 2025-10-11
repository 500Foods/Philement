/*
 * Unity Test File: landing_print_test_check_print_landing_readiness.c
 * This file contains unit tests for the check_print_landing_readiness function
 * from src/landing/landing_print.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>

// Forward declarations for functions being tested
LaunchReadiness check_print_landing_readiness(void);

// Test function declarations
void test_check_print_landing_readiness_subsystem_running_no_jobs(void);
void test_check_print_landing_readiness_subsystem_not_running(void);
void test_check_print_landing_readiness_active_jobs(void);

// Mock state
static bool mock_subsystem_running = true;
static int mock_thread_count = 0;

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_subsystem_running;
}

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads print_threads = {0};

void setUp(void) {
    // Reset mock state
    mock_subsystem_running = true;
    mock_thread_count = 0;
    print_threads.thread_count = 0;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_print_landing_readiness =====

void test_check_print_landing_readiness_subsystem_running_no_jobs(void) {
    // Arrange
    mock_subsystem_running = true;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_print_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Print subsystem running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Go:      No active print jobs", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Go:      No dependent subsystems", result.messages[3]);
    TEST_ASSERT_NOT_NULL(result.messages[4]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Print Subsystem", result.messages[4]);
    TEST_ASSERT_NULL(result.messages[5]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_print_landing_readiness_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_running = false;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_print_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Print subsystem not running", result.messages[1]);
    TEST_ASSERT_NULL(result.messages[2]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_print_landing_readiness_active_jobs(void) {
    // Arrange
    mock_subsystem_running = true;
    print_threads.thread_count = 2; // Simulate active print jobs

    // Act
    LaunchReadiness result = check_print_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Print subsystem running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Active print jobs in progress", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_print_landing_readiness_subsystem_running_no_jobs);
    RUN_TEST(test_check_print_landing_readiness_subsystem_not_running);
    RUN_TEST(test_check_print_landing_readiness_active_jobs);

    return UNITY_END();
}