/*
 * Unity Test File: WebSocket Protocol Validation Tests
 * This file contains unit tests for validate_protocol function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
bool validate_protocol(const char* protocol);

// Forward declarations for test functions
void test_validate_protocol_valid_cases(void);
void test_validate_protocol_invalid_cases(void);
void test_validate_protocol_null_and_empty(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_validate_protocol_valid_cases(void) {
    // Test valid protocol strings
    TEST_ASSERT_TRUE(validate_protocol("hydrogen"));
    TEST_ASSERT_TRUE(validate_protocol("websocket"));
    TEST_ASSERT_TRUE(validate_protocol("chat-protocol"));
    TEST_ASSERT_TRUE(validate_protocol("a"));
    TEST_ASSERT_TRUE(validate_protocol("test123"));
    TEST_ASSERT_TRUE(validate_protocol("Protocol-With-Mixed-Case"));
}

void test_validate_protocol_invalid_cases(void) {
    // Test invalid protocol strings
    TEST_ASSERT_FALSE(validate_protocol("123invalid"));
    TEST_ASSERT_FALSE(validate_protocol("-invalid"));
    TEST_ASSERT_FALSE(validate_protocol("invalid protocol"));
    TEST_ASSERT_FALSE(validate_protocol("invalid.protocol"));
    TEST_ASSERT_FALSE(validate_protocol("invalid@protocol"));
}

void test_validate_protocol_null_and_empty(void) {
    // Test null and empty strings
    TEST_ASSERT_FALSE(validate_protocol(NULL));
    TEST_ASSERT_FALSE(validate_protocol(""));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_protocol_valid_cases);
    RUN_TEST(test_validate_protocol_invalid_cases);
    RUN_TEST(test_validate_protocol_null_and_empty);

    return UNITY_END();
}
