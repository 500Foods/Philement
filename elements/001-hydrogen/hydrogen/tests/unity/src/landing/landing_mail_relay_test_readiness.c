/*
 * Unity Test File: Mail Relay Landing Readiness Tests
 * This file contains unit tests for the check_mail_relay_landing_readiness function
 * from src/landing/landing_mail_relay.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Include mock header
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
LaunchReadiness check_mail_relay_landing_readiness(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) volatile sig_atomic_t mail_relay_system_shutdown = 0;

__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Forward declarations for test functions
void test_check_mail_relay_landing_readiness_success_when_not_running(void);
void test_check_mail_relay_landing_readiness_reports_no_dependents(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mail_relay_system_shutdown = 0;
    mailrelay_threads.thread_count = 0;
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC READINESS TESTS =====

void test_check_mail_relay_landing_readiness_success_when_not_running(void) {
    // Arrange: Mail relay not running (registry not initialized in test)

    // Act: Call the function
    LaunchReadiness result = check_mail_relay_landing_readiness();

    // Assert: Should return ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);

    // Verify messages
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Mail relay not running", result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      No active workers to drain", result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Go:      No dependent subsystems", result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Mail Relay Subsystem", result.messages[4]);
    TEST_ASSERT_NULL(result.messages[5]);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_mail_relay_landing_readiness_reports_no_dependents(void) {
    // Arrange
    LaunchReadiness result = check_mail_relay_landing_readiness();

    // Assert: Verify no-dependents message appears
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);

    bool found_no_dependents = false;
    for (size_t i = 1; result.messages[i] != NULL; i++) {
        if (strstr(result.messages[i], "No dependent subsystems") != NULL) {
            found_no_dependents = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_no_dependents);

    // Clean up messages
    free_readiness_messages(&result);
}


// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic readiness tests
    RUN_TEST(test_check_mail_relay_landing_readiness_success_when_not_running);
    RUN_TEST(test_check_mail_relay_landing_readiness_reports_no_dependents);

    return UNITY_END();
}
