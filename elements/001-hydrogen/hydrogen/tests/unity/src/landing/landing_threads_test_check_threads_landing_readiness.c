/*
 * Unity Test File: landing_threads_test_check_threads_landing_readiness.c
 * This file contains unit tests for the check_threads_landing_readiness function
 * from src/landing/landing_threads.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
LaunchReadiness check_threads_landing_readiness(void);

// Test function declarations
void test_check_threads_landing_readiness_subsystem_running_no_active_threads(void);
void test_check_threads_landing_readiness_subsystem_not_running(void);
void test_check_threads_landing_readiness_active_webserver_threads(void);
void test_check_threads_landing_readiness_active_websocket_threads(void);
void test_check_threads_landing_readiness_active_mdns_threads(void);
void test_check_threads_landing_readiness_active_print_threads(void);
void test_check_threads_landing_readiness_multiple_active_threads(void);

// Mock state
static bool mock_subsystem_running = true;

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads logging_threads = {0};
__attribute__((weak)) ServiceThreads webserver_threads = {0};
__attribute__((weak)) ServiceThreads websocket_threads = {0};
__attribute__((weak)) ServiceThreads mdns_server_threads = {0};
__attribute__((weak)) ServiceThreads print_threads = {0};

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_subsystem_running;
}

void setUp(void) {
    // Reset mock state
    mock_subsystem_running = true;
    logging_threads.thread_count = 0;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_threads_landing_readiness =====

void test_check_threads_landing_readiness_subsystem_running_no_active_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      All service threads ready for cleanup", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_running = false;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Thread management not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_active_webserver_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 2;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Web threads still active", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_active_websocket_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 1;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebSocket threads still active", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_active_mdns_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 3;
    print_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   mDNS server threads still active", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_active_print_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 1;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Print threads still active", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_threads_landing_readiness_multiple_active_threads(void) {
    // Arrange
    mock_subsystem_running = true;
    webserver_threads.thread_count = 1;
    websocket_threads.thread_count = 2;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 1;

    // Act
    LaunchReadiness result = check_threads_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_THREADS, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Web threads still active", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebSocket threads still active", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Print threads still active", result.messages[3]);
    TEST_ASSERT_NOT_NULL(result.messages[4]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Threads", result.messages[4]);
    TEST_ASSERT_NULL(result.messages[5]);

    // Cleanup
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_threads_landing_readiness_subsystem_running_no_active_threads);
    RUN_TEST(test_check_threads_landing_readiness_subsystem_not_running);
    RUN_TEST(test_check_threads_landing_readiness_active_webserver_threads);
    RUN_TEST(test_check_threads_landing_readiness_active_websocket_threads);
    RUN_TEST(test_check_threads_landing_readiness_active_mdns_threads);
    RUN_TEST(test_check_threads_landing_readiness_active_print_threads);
    RUN_TEST(test_check_threads_landing_readiness_multiple_active_threads);

    return UNITY_END();
}