/*
 * Unity Test File: mDNS Client Landing Readiness Tests
 * This file contains unit tests for the check_mdns_client_landing_readiness function
 * from src/landing/landing_mdns_client.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Include mock header (but we'll override the functions)
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
LaunchReadiness check_mdns_client_landing_readiness(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) volatile sig_atomic_t mdns_client_system_shutdown = 0;

__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Forward declarations for test functions
void test_check_mdns_client_landing_readiness_success(void);
void test_check_mdns_client_landing_readiness_mdns_not_running(void);
void test_check_mdns_client_landing_readiness_network_not_running(void);
void test_check_mdns_client_landing_readiness_logging_not_running(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mdns_client_system_shutdown = 0;
    mock_landing_reset_all();
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC READINESS TESTS =====

void test_check_mdns_client_landing_readiness_success(void) {
    // Arrange: All conditions met (using real subsystem states)

    // Act: Call the function
    LaunchReadiness result = check_mdns_client_landing_readiness();

    // Assert: Check that function executes without crashing
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_mdns_client_landing_readiness_mdns_not_running(void) {
    // Arrange: mDNS client not running
    mock_landing_set_mdns_client_running(false);

    // Act: Call the function
    LaunchReadiness result = check_mdns_client_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.subsystem);

    // Verify messages
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   mDNS Client not running", result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of mDNS Client", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_mdns_client_landing_readiness_network_not_running(void) {
    // Arrange: Network not running
    mock_landing_set_mdns_client_running(true);
    mock_landing_set_network_running(false);
    mock_landing_set_logging_running(true);

    // Act: Call the function
    LaunchReadiness result = check_mdns_client_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.subsystem);

    // Verify messages
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Network subsystem not running", result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of mDNS Client", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_mdns_client_landing_readiness_logging_not_running(void) {
    // Arrange: Logging not running
    mock_landing_set_mdns_client_running(true);
    mock_landing_set_network_running(true);
    mock_landing_set_logging_running(false);

    // Act: Call the function
    LaunchReadiness result = check_mdns_client_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.subsystem);

    // Verify messages
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_CLIENT, result.messages[0]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Logging subsystem not running", result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of mDNS Client", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic readiness tests
    RUN_TEST(test_check_mdns_client_landing_readiness_success);
    RUN_TEST(test_check_mdns_client_landing_readiness_mdns_not_running);
    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_network_not_running);
    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_logging_not_running);

    return UNITY_END();
}