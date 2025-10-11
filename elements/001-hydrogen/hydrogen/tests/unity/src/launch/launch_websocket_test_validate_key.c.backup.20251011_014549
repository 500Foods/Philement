/*
 * Unity Test File: WebSocket Key Validation Tests
 * This file contains unit tests for validate_key function
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/launch/launch.h"

// Forward declarations for functions being tested
bool validate_key(const char* key);

// Forward declarations for test functions
void test_validate_key_valid_cases(void);
void test_validate_key_invalid_cases(void);
void test_validate_key_null_and_empty(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_validate_key_valid_cases(void) {
    // Test valid key strings (8+ printable ASCII characters)
    TEST_ASSERT_TRUE(validate_key("validkey123"));
    TEST_ASSERT_TRUE(validate_key("!@#$%^&*()"));
    TEST_ASSERT_TRUE(validate_key("MySecureKey123!"));
    TEST_ASSERT_TRUE(validate_key("abcdefghijklmnop"));
}

void test_validate_key_invalid_cases(void) {
    // Test invalid key strings
    TEST_ASSERT_FALSE(validate_key("short"));
    TEST_ASSERT_FALSE(validate_key("key\twithtab"));
    TEST_ASSERT_FALSE(validate_key("key\nwithnewline"));
    TEST_ASSERT_FALSE(validate_key("key with space"));
}

void test_validate_key_null_and_empty(void) {
    // Test null and empty strings
    TEST_ASSERT_FALSE(validate_key(NULL));
    TEST_ASSERT_FALSE(validate_key(""));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_key_valid_cases);
    RUN_TEST(test_validate_key_invalid_cases);
    RUN_TEST(test_validate_key_null_and_empty);

    return UNITY_END();
}
