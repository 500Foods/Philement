/*
 * Unity Test File: mDNS Server Landing Readiness Tests
 * This file contains unit tests for the check_mdns_server_landing_readiness function
 * from src/landing/landing_mdns_server.c
 */

// Need stdbool.h for bool type
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Include globals.h to get SR_ constants
#include <src/globals.h>




// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Include mock header
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
LaunchReadiness check_mdns_server_landing_readiness(void);


// Mock globals - use weak linkage to avoid multiple definitions

__attribute__((weak))
ServiceThreads mdns_server_threads = {0};

__attribute__((weak))
volatile sig_atomic_t mdns_server_system_shutdown = 0;

// Forward declarations for test functions
void test_check_mdns_server_landing_readiness_success(void);
void test_check_mdns_server_landing_readiness_not_running(void);
void test_check_mdns_server_landing_readiness_network_not_running(void);
void test_check_mdns_server_landing_readiness_malloc_failure(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
    mdns_server_system_shutdown = 0;
    mdns_server_threads.thread_count = 1; // Simulate active threads
}

void tearDown(void) {
    // Clean up after each test
}

// ===== READINESS CHECK TESTS =====

void test_check_mdns_server_landing_readiness_success(void) {
    // Arrange: mDNS server and network are running
    mock_landing_set_mdns_server_running(true);
    mock_landing_set_network_running(true);

    // Act: Call the function
    LaunchReadiness result = check_mdns_server_landing_readiness();

    // Assert: Should be ready
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_NOT_NULL(result.messages[4]);
    TEST_ASSERT_NULL(result.messages[5]);
}

void test_check_mdns_server_landing_readiness_not_running(void) {
    // Arrange: mDNS server is not running
    mock_landing_set_mdns_server_running(false);
    mock_landing_set_network_running(true);

    // Act: Call the function
    LaunchReadiness result = check_mdns_server_landing_readiness();

    // Assert: Should not be ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_mdns_server_landing_readiness_network_not_running(void) {
    // Arrange: mDNS server is running but network is not
    mock_landing_set_mdns_server_running(true);
    mock_landing_set_network_running(false);

    // Act: Call the function
    LaunchReadiness result = check_mdns_server_landing_readiness();

    // Assert: Should not be ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);
}

void test_check_mdns_server_landing_readiness_malloc_failure(void) {
    // Arrange: mDNS server and network are running
    mock_landing_set_mdns_server_running(true);
    mock_landing_set_network_running(true);

    // Act: Call the function
    LaunchReadiness result = check_mdns_server_landing_readiness();

    // Assert: Should be ready (malloc should work normally)
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);

    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Readiness check tests
    RUN_TEST(test_check_mdns_server_landing_readiness_success);
    RUN_TEST(test_check_mdns_server_landing_readiness_not_running);
    RUN_TEST(test_check_mdns_server_landing_readiness_network_not_running);
    RUN_TEST(test_check_mdns_server_landing_readiness_malloc_failure);

    return UNITY_END();
}