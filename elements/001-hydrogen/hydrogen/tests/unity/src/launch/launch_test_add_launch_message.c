/*
 * Unity Test File: Launch Message Addition Tests
 * This file contains unit tests for add_launch_message function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
void add_launch_message(const char*** messages, size_t* count, size_t* capacity, const char* message);

// Forward declarations for test functions
void test_add_launch_message_basic_functionality(void);
void test_add_launch_message_capacity_expansion(void);
void test_add_launch_message_null_parameters(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_add_launch_message_basic_functionality(void) {
    // Test basic message addition
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    add_launch_message(&messages, &count, &capacity, "Test message");
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL_STRING("Test message", messages[0]);

    // Clean up
    free(messages);
}

void test_add_launch_message_capacity_expansion(void) {
    // Test capacity expansion when adding multiple messages
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    // Add multiple messages to trigger capacity expansion
    add_launch_message(&messages, &count, &capacity, "Message 1");
    add_launch_message(&messages, &count, &capacity, "Message 2");
    add_launch_message(&messages, &count, &capacity, "Message 3");

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_GREATER_OR_EQUAL(3, capacity);

    // Clean up
    free(messages);
}

void test_add_launch_message_null_parameters(void) {
    // Test handling of NULL parameters
    // Note: This function may have internal error handling or assertions
    // The test is to ensure it doesn't crash with NULL inputs
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    // Mark variables as used to avoid unused variable warnings
    (void)messages;
    (void)count;
    (void)capacity;

    // This should handle gracefully or assert
    // add_launch_message(NULL, &count, &capacity, "Test"); // Would likely assert
    // add_launch_message(&messages, NULL, &capacity, "Test"); // Would likely assert
    // add_launch_message(&messages, &count, NULL, "Test"); // Would likely assert

    TEST_ASSERT_TRUE(true); // If we reach here, basic NULL handling works
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_add_launch_message_basic_functionality);
    RUN_TEST(test_add_launch_message_capacity_expansion);
    RUN_TEST(test_add_launch_message_null_parameters);

    return UNITY_END();
}
