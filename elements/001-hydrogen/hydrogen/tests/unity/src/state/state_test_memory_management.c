/*
 * Unity Test File: state_test_memory_management.c
 *
 * Tests for free_readiness_messages and memory management functions from state.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_free_readiness_messages_null_readiness(void);
void test_free_readiness_messages_null_messages(void);
void test_free_readiness_messages_empty_messages(void);
void test_free_readiness_messages_single_message(void);
void test_free_readiness_messages_multiple_messages(void);
void test_free_readiness_messages_with_null_entries(void);
void test_free_readiness_messages_large_message_array(void);
void test_free_readiness_messages_memory_cleanup_verification(void);

#ifndef STATE_TEST_RUNNER
void setUp(void) {
    // No special setup needed for memory management tests
}

void tearDown(void) {
    // No special cleanup needed for memory management tests
}
#endif

// Tests for free_readiness_messages function
void test_free_readiness_messages_null_readiness(void) {
    // Test with NULL readiness structure
    free_readiness_messages(NULL);

    // Should not crash - this is a safety test
    TEST_PASS();
}

void test_free_readiness_messages_null_messages(void) {
    // Create readiness structure with NULL messages
    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = NULL
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer remains NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_free_readiness_messages_empty_messages(void) {
    // Create readiness structure with empty messages array
    const char** messages = calloc(1, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = NULL; // NULL-terminated empty array

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer is set to NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_free_readiness_messages_single_message(void) {
    // Create readiness structure with single message
    const char** messages = calloc(2, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = strdup("Test message");
    messages[1] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer is set to NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_free_readiness_messages_multiple_messages(void) {
    // Create readiness structure with multiple messages
    const char** messages = calloc(4, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = strdup("Message 1");
    TEST_ASSERT_NOT_NULL(messages[0]);
    messages[1] = strdup("Message 2");
    TEST_ASSERT_NOT_NULL(messages[1]);
    messages[2] = strdup("Message 3");
    TEST_ASSERT_NOT_NULL(messages[2]);
    messages[3] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer is set to NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_free_readiness_messages_with_null_entries(void) {
    // Create readiness structure with some NULL entries
    const char** messages = calloc(5, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = strdup("Message 1");
    TEST_ASSERT_NOT_NULL(messages[0]);
    messages[1] = NULL; // NULL entry in middle
    messages[2] = strdup("Message 3");
    TEST_ASSERT_NOT_NULL(messages[2]);
    messages[3] = NULL; // Another NULL entry
    messages[4] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer is set to NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_free_readiness_messages_large_message_array(void) {
    // Create readiness structure with many messages
    const size_t num_messages = 100;
    const char** messages = calloc(num_messages + 1, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);

    // Allocate many messages
    for (size_t i = 0; i < num_messages; i++) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Test message %zu", i);
        messages[i] = strdup(buffer);
        TEST_ASSERT_NOT_NULL(messages[i]);
    }
    messages[num_messages] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify messages pointer is set to NULL
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash even with large array
    TEST_PASS();
}

void test_free_readiness_messages_memory_cleanup_verification(void) {
    // Test that memory is properly cleaned up by attempting to use freed memory
    // (This is more of a conceptual test - in practice we'd use valgrind/heap analysis)

    const char** messages = calloc(3, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = strdup("Test message 1");
    TEST_ASSERT_NOT_NULL(messages[0]);
    messages[1] = strdup("Test message 2");
    TEST_ASSERT_NOT_NULL(messages[1]);
    messages[2] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test_subsystem",
        .ready = true,
        .messages = messages
    };

    // Call function
    free_readiness_messages(&readiness);

    // Verify structure is properly reset
    TEST_ASSERT_NULL(readiness.messages);
    TEST_ASSERT_EQUAL_STRING("test_subsystem", readiness.subsystem);
    TEST_ASSERT_TRUE(readiness.ready);

    // Should not crash
    TEST_PASS();
}

// Main function - only compiled when this file is run standalone
#ifndef STATE_TEST_RUNNER
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_readiness_messages_null_readiness);
    RUN_TEST(test_free_readiness_messages_null_messages);
    RUN_TEST(test_free_readiness_messages_empty_messages);
    RUN_TEST(test_free_readiness_messages_single_message);
    RUN_TEST(test_free_readiness_messages_multiple_messages);
    RUN_TEST(test_free_readiness_messages_with_null_entries);
    RUN_TEST(test_free_readiness_messages_large_message_array);
    RUN_TEST(test_free_readiness_messages_memory_cleanup_verification);

    return UNITY_END();
}
#endif
