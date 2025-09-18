/*
 * Unity Test File: landing_terminal_test_check_terminal_landing_readiness.c
 * This file contains unit tests for the check_terminal_landing_readiness function
 * from src/landing/landing_terminal.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
LaunchReadiness check_terminal_landing_readiness(void);

// Test function declarations
void test_check_terminal_landing_readiness_subsystem_running_dependencies_ready(void);
void test_check_terminal_landing_readiness_subsystem_not_running(void);
void test_check_terminal_landing_readiness_webserver_not_running(void);
void test_check_terminal_landing_readiness_websocket_not_running(void);
void test_check_terminal_landing_readiness_malloc_failure(void);

// Mock state
static bool mock_subsystem_running_terminal = true;
static bool mock_subsystem_running_webserver = true;
static bool mock_subsystem_running_websocket = true;

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    if (strcmp(name, SR_TERMINAL) == 0) {
        return mock_subsystem_running_terminal;
    } else if (strcmp(name, "WebServer") == 0) {
        return mock_subsystem_running_webserver;
    } else if (strcmp(name, "WebSocket") == 0) {
        return mock_subsystem_running_websocket;
    }
    return false;
}

void setUp(void) {
    // Reset mock state
    mock_subsystem_running_terminal = true;
    mock_subsystem_running_webserver = true;
    mock_subsystem_running_websocket = true;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_terminal_landing_readiness =====

void test_check_terminal_landing_readiness_subsystem_running_dependencies_ready(void) {
    // Arrange
    mock_subsystem_running_terminal = true;
    mock_subsystem_running_webserver = true;
    mock_subsystem_running_websocket = true;

    // Act
    LaunchReadiness result = check_terminal_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      WebServer ready for shutdown", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Go:      WebSocket ready for shutdown", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Terminal ready for cleanup", result.messages[3]);
    TEST_ASSERT_NOT_NULL(result.messages[4]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Terminal", result.messages[4]);
    TEST_ASSERT_NULL(result.messages[5]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_terminal_landing_readiness_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_running_terminal = false;
    mock_subsystem_running_webserver = true;
    mock_subsystem_running_websocket = true;

    // Act
    LaunchReadiness result = check_terminal_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Terminal not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Terminal", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_terminal_landing_readiness_webserver_not_running(void) {
    // Arrange
    mock_subsystem_running_terminal = true;
    mock_subsystem_running_webserver = false;
    mock_subsystem_running_websocket = true;

    // Act
    LaunchReadiness result = check_terminal_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebServer subsystem not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Terminal", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_terminal_landing_readiness_websocket_not_running(void) {
    // Arrange
    mock_subsystem_running_terminal = true;
    mock_subsystem_running_webserver = true;
    mock_subsystem_running_websocket = false;

    // Act
    LaunchReadiness result = check_terminal_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebSocket subsystem not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Terminal", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_terminal_landing_readiness_malloc_failure(void) {
    // This test would require mocking malloc to fail, but that's complex
    // For now, we'll skip this edge case as it's covered by blackbox tests
    TEST_IGNORE_MESSAGE("malloc failure test requires advanced mocking not implemented");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_terminal_landing_readiness_subsystem_running_dependencies_ready);
    RUN_TEST(test_check_terminal_landing_readiness_subsystem_not_running);
    RUN_TEST(test_check_terminal_landing_readiness_webserver_not_running);
    RUN_TEST(test_check_terminal_landing_readiness_websocket_not_running);
    if (0) RUN_TEST(test_check_terminal_landing_readiness_malloc_failure);

    return UNITY_END();
}