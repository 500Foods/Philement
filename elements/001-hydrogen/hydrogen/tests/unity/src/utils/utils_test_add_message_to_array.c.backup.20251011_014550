/*
 * Unity Test File: add_message_to_array Function Tests
 * This file contains comprehensive unit tests for the add_message_to_array() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test message array operations with various conditions
 * - Parameter validation and null checks
 * - Edge cases and boundary conditions
 * - Buffer overflow handling
 * - Memory allocation behavior
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Suppress const qualifier warnings in test cleanup code
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

// Forward declaration for the function being tested
bool add_message_to_array(const char** messages, int max_messages, int* count, const char* format, ...);

// Test fixtures - declared without const for easier manipulation in tests
static char* test_messages_storage[10];
static const char** test_messages = (const char**)test_messages_storage;
static int message_count;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
    memset((void*)test_messages, 0, sizeof(test_messages));
    message_count = 0;
}

void tearDown(void) {
    // Clean up test fixtures - reset for next test
    message_count = 0;
}

// Function prototypes for test functions
void test_add_message_to_array_null_messages(void);
void test_add_message_to_array_null_count(void);
void test_add_message_to_array_max_messages_exceeded(void);
void test_add_message_to_array_null_format(void);
void test_add_message_to_array_empty_format(void);
void test_add_message_to_array_simple_message(void);
void test_add_message_to_array_formatted_message(void);
void test_add_message_to_array_multiple_messages(void);
void test_add_message_to_array_array_full(void);
void test_add_message_to_array_count_at_limit(void);
void test_add_message_to_array_with_null_termination(void);
void test_add_message_to_array_large_message(void);
void test_add_message_to_array_various_format_specifiers(void);
void test_add_message_to_array_count_increment(void);
void test_add_message_to_array_null_termination_preservation(void);
void test_add_message_to_array_complex_formatting(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_add_message_to_array_null_messages(void) {
    int count = 0;
    bool result = add_message_to_array(NULL, 10, &count, "test message");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, count);
}

void test_add_message_to_array_null_count(void) {
    bool result = add_message_to_array(test_messages, 10, NULL, "test message");
    TEST_ASSERT_FALSE(result);
}

void test_add_message_to_array_max_messages_exceeded(void) {
    int count = 10;  // Already at max
    bool result = add_message_to_array(test_messages, 10, &count, "test message");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(10, count);
}

void test_add_message_to_array_null_format(void) {
    bool result = add_message_to_array(test_messages, 10, &message_count, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, message_count);
}

void test_add_message_to_array_empty_format(void) {
    bool result = add_message_to_array(test_messages, 10, &message_count, "");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("", test_messages[0]);
}

//=============================================================================
// Basic Message Addition Tests
//=============================================================================

void test_add_message_to_array_simple_message(void) {
    bool result = add_message_to_array(test_messages, 10, &message_count, "Hello World");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("Hello World", test_messages[0]);
    TEST_ASSERT_NULL(test_messages[1]);  // Should be null-terminated
}

void test_add_message_to_array_formatted_message(void) {
    bool result = add_message_to_array(test_messages, 10, &message_count, "Count: %d, Name: %s", 42, "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("Count: 42, Name: Test", test_messages[0]);
}

//=============================================================================
// Multiple Message Tests
//=============================================================================

void test_add_message_to_array_multiple_messages(void) {
    // Add first message
    bool result1 = add_message_to_array(test_messages, 5, &message_count, "Message 1");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(1, message_count);

    // Add second message
    bool result2 = add_message_to_array(test_messages, 5, &message_count, "Message 2");
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL(2, message_count);

    // Add third message
    bool result3 = add_message_to_array(test_messages, 5, &message_count, "Message 3");
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_EQUAL(3, message_count);

    // Verify all messages
    TEST_ASSERT_EQUAL_STRING("Message 1", test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("Message 2", test_messages[1]);
    TEST_ASSERT_EQUAL_STRING("Message 3", test_messages[2]);
    TEST_ASSERT_NULL(test_messages[3]);  // Should be null-terminated
}

void test_add_message_to_array_array_full(void) {
    // Fill the array to capacity (function allows max_messages - 1 messages)
    for (int i = 0; i < 4; i++) {  // Only 4 messages, not 5
        bool result = add_message_to_array(test_messages, 5, &message_count, "Message %d", i);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(i + 1, message_count);
    }

    // Try to add one more message (should fail because *count >= max_messages - 1)
    bool result = add_message_to_array(test_messages, 5, &message_count, "Overflow message");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(4, message_count);  // Count should remain the same
}

void test_add_message_to_array_count_at_limit(void) {
    // Set count to max_messages - 1
    message_count = 4;
    // We can't directly assign strings to const char* array, so we'll test the logic differently
    // The function should handle the count properly when it's at the limit

    // Try to add a message when we're at the limit
    bool result = add_message_to_array(test_messages, 5, &message_count, "New message");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(4, message_count);  // Count should remain the same
}

//=============================================================================
// Null Termination and Array Management Tests
//=============================================================================

void test_add_message_to_array_with_null_termination(void) {
    // Add a few messages and verify null termination is maintained
    add_message_to_array(test_messages, 10, &message_count, "First");
    TEST_ASSERT_NULL(test_messages[1]);

    add_message_to_array(test_messages, 10, &message_count, "Second");
    TEST_ASSERT_NULL(test_messages[2]);

    add_message_to_array(test_messages, 10, &message_count, "Third");
    TEST_ASSERT_NULL(test_messages[3]);

    // Verify all messages are accessible
    TEST_ASSERT_EQUAL_STRING("First", test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("Second", test_messages[1]);
    TEST_ASSERT_EQUAL_STRING("Third", test_messages[2]);
}

void test_add_message_to_array_count_increment(void) {
    // Verify count is properly incremented
    TEST_ASSERT_EQUAL(0, message_count);

    add_message_to_array(test_messages, 10, &message_count, "Message 1");
    TEST_ASSERT_EQUAL(1, message_count);

    add_message_to_array(test_messages, 10, &message_count, "Message 2");
    TEST_ASSERT_EQUAL(2, message_count);
}

void test_add_message_to_array_null_termination_preservation(void) {
    // Add messages and verify null termination is preserved throughout
    add_message_to_array(test_messages, 10, &message_count, "Alpha");

    // Manually check that the rest of the array remains null
    for (int i = 1; i < 10; i++) {
        TEST_ASSERT_NULL(test_messages[i]);
    }

    add_message_to_array(test_messages, 10, &message_count, "Beta");

    // After adding second message, positions 0 and 1 should have content, 2+ should be null
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_NOT_NULL(test_messages[1]);
    for (int i = 2; i < 10; i++) {
        TEST_ASSERT_NULL(test_messages[i]);
    }
}

//=============================================================================
// Format String and Content Tests
//=============================================================================

void test_add_message_to_array_large_message(void) {
    // Test with a very long message
    char* long_message = "This is a very long message that contains many words and should test the memory allocation capabilities of the add_message_to_array function when dealing with larger messages that might require more memory than initially expected.";

    bool result = add_message_to_array(test_messages, 10, &message_count, "%s", long_message);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING(long_message, test_messages[0]);
}

void test_add_message_to_array_various_format_specifiers(void) {
    // Test various format specifiers
    bool result = add_message_to_array(test_messages, 10, &message_count,
                                       "Int: %d, Float: %.2f, String: %s, Char: %c",
                                       123, 45.67, "test", 'X');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("Int: 123, Float: 45.67, String: test, Char: X", test_messages[0]);
}

void test_add_message_to_array_complex_formatting(void) {
    // Test complex formatting with width and precision specifiers
    bool result = add_message_to_array(test_messages, 10, &message_count,
                                       "%10s|%-10s|%05d|%.3f",
                                       "left", "right", 42, 3.14159);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, message_count);
    TEST_ASSERT_NOT_NULL(test_messages[0]);
    TEST_ASSERT_EQUAL_STRING("      left|right     |00042|3.142", test_messages[0]);  // Actual output from vasprintf
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_add_message_to_array_null_messages);
    RUN_TEST(test_add_message_to_array_null_count);
    RUN_TEST(test_add_message_to_array_max_messages_exceeded);
    RUN_TEST(test_add_message_to_array_null_format);
    RUN_TEST(test_add_message_to_array_empty_format);

    // Basic message addition tests
    RUN_TEST(test_add_message_to_array_simple_message);
    RUN_TEST(test_add_message_to_array_formatted_message);

    // Multiple message tests
    RUN_TEST(test_add_message_to_array_multiple_messages);
    RUN_TEST(test_add_message_to_array_array_full);
    RUN_TEST(test_add_message_to_array_count_at_limit);

    // Null termination and array management tests
    RUN_TEST(test_add_message_to_array_with_null_termination);
    RUN_TEST(test_add_message_to_array_count_increment);
    RUN_TEST(test_add_message_to_array_null_termination_preservation);

    // Format string and content tests
    RUN_TEST(test_add_message_to_array_large_message);
    RUN_TEST(test_add_message_to_array_various_format_specifiers);
    RUN_TEST(test_add_message_to_array_complex_formatting);

    return UNITY_END();
}
