/*
 * Unity Test File: landing_websocket_test_check_websocket_landing_readiness.c
 * This file contains unit tests for the check_websocket_landing_readiness function
 * from src/landing/landing_websocket.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>

// Forward declarations for functions being tested
LaunchReadiness check_websocket_landing_readiness(void);

// Test function declarations
void test_check_websocket_landing_readiness_subsystem_running_with_thread(void);
void test_check_websocket_landing_readiness_subsystem_not_running(void);
void test_check_websocket_landing_readiness_no_thread(void);

// Mock state
static bool mock_subsystem_running = true;
static pthread_t mock_thread = 456;
static int mock_thread_count = 1;

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads websocket_threads = {0};
__attribute__((weak)) pthread_t websocket_thread = 0;

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_subsystem_running;
}

void setUp(void) {
    // Reset mock state
    mock_subsystem_running = true;
    mock_thread = 456;
    mock_thread_count = 1;
    websocket_thread = mock_thread;
    websocket_threads.thread_count = mock_thread_count;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_websocket_landing_readiness =====

void test_check_websocket_landing_readiness_subsystem_running_with_thread(void) {
    // Arrange
    mock_subsystem_running = true;
    websocket_thread = 456;
    websocket_threads.thread_count = 1;

    // Act
    LaunchReadiness result = check_websocket_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      WebSocket thread ready for shutdown", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Go:      All resources ready for cleanup", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of WebSocket", result.messages[3]);
    TEST_ASSERT_NULL(result.messages[4]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_websocket_landing_readiness_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_running = false;
    websocket_thread = 0;
    websocket_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_websocket_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebSocket not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of WebSocket", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_websocket_landing_readiness_no_thread(void) {
    // Arrange
    mock_subsystem_running = true;
    websocket_thread = 0;
    websocket_threads.thread_count = 0;

    // Act
    LaunchReadiness result = check_websocket_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebSocket thread not accessible", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Resources not ready for cleanup", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of WebSocket", result.messages[3]);
    TEST_ASSERT_NULL(result.messages[4]);

    // Cleanup
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_websocket_landing_readiness_subsystem_running_with_thread);
    RUN_TEST(test_check_websocket_landing_readiness_subsystem_not_running);
    RUN_TEST(test_check_websocket_landing_readiness_no_thread);

    return UNITY_END();
}