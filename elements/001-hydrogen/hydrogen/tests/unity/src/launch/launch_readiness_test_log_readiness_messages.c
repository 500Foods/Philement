/*
 * Unity Test File: Launch Readiness Message Logging Tests
 * This file contains unit tests for log_readiness_messages function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
void log_readiness_messages(LaunchReadiness* readiness);

// Forward declarations for test functions
void test_log_readiness_messages_null_readiness(void);
void test_log_readiness_messages_empty_messages(void);
void test_log_readiness_messages_with_messages(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_log_readiness_messages_null_readiness(void) {
    // Test that function handles NULL readiness gracefully
    log_readiness_messages(NULL);
    // Should not crash or cause issues
    TEST_ASSERT_TRUE(true); // If we reach here, the test passed
}

void test_log_readiness_messages_empty_messages(void) {
    // Test with readiness structure but no messages
    LaunchReadiness readiness = {
        .subsystem = "Test",
        .ready = true,
        .messages = NULL
    };

    log_readiness_messages(&readiness);
    // Should handle NULL messages array gracefully
    TEST_ASSERT_TRUE(true);
}

void test_log_readiness_messages_with_messages(void) {
    // Test with actual messages
    const char* test_messages[] = {
        "Test Subsystem",
        "  Go:      Test message 1",
        "  No-Go:   Test error message",
        NULL
    };

    LaunchReadiness readiness = {
        .subsystem = "Test",
        .ready = false,
        .messages = test_messages
    };

    log_readiness_messages(&readiness);
    // Should process messages without crashing
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_log_readiness_messages_null_readiness);
    RUN_TEST(test_log_readiness_messages_empty_messages);
    RUN_TEST(test_log_readiness_messages_with_messages);

    return UNITY_END();
}
