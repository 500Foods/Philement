/*
 * Unity Test File: Auth Service Validation is_alphanumeric_underscore_hyphen Function Tests
 * This file contains unit tests for the is_alphanumeric_underscore_hyphen function in auth_service_validation.c
 *
 * Tests: is_alphanumeric_underscore_hyphen() - Check if string contains only alphanumeric, underscore, or hyphen
 *
 * CHANGELOG:
 * 2026-01-10: Initial version - Tests for username character validation
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_validation.h>

// Function prototypes for test functions
void test_is_alphanumeric_underscore_hyphen_valid_alphanumeric(void);
void test_is_alphanumeric_underscore_hyphen_with_underscore(void);
void test_is_alphanumeric_underscore_hyphen_with_hyphen(void);
void test_is_alphanumeric_underscore_hyphen_mixed(void);
void test_is_alphanumeric_underscore_hyphen_all_lowercase(void);
void test_is_alphanumeric_underscore_hyphen_all_uppercase(void);
void test_is_alphanumeric_underscore_hyphen_all_numbers(void);
void test_is_alphanumeric_underscore_hyphen_with_space(void);
void test_is_alphanumeric_underscore_hyphen_with_special_chars(void);
void test_is_alphanumeric_underscore_hyphen_with_dot(void);
void test_is_alphanumeric_underscore_hyphen_with_at_sign(void);
void test_is_alphanumeric_underscore_hyphen_null_parameter(void);
void test_is_alphanumeric_underscore_hyphen_empty_string(void);
void test_is_alphanumeric_underscore_hyphen_unicode_chars(void);

void setUp(void) {
    // No setup needed for validation functions
}

void tearDown(void) {
    // No teardown needed for validation functions
}

// Test valid alphanumeric string
void test_is_alphanumeric_underscore_hyphen_valid_alphanumeric(void) {
    bool result = is_alphanumeric_underscore_hyphen("testuser123");
    TEST_ASSERT_TRUE(result);
}

// Test string with underscore
void test_is_alphanumeric_underscore_hyphen_with_underscore(void) {
    bool result = is_alphanumeric_underscore_hyphen("test_user_123");
    TEST_ASSERT_TRUE(result);
}

// Test string with hyphen
void test_is_alphanumeric_underscore_hyphen_with_hyphen(void) {
    bool result = is_alphanumeric_underscore_hyphen("test-user-123");
    TEST_ASSERT_TRUE(result);
}

// Test string with mixed valid characters
void test_is_alphanumeric_underscore_hyphen_mixed(void) {
    bool result = is_alphanumeric_underscore_hyphen("Test_User-123");
    TEST_ASSERT_TRUE(result);
}

// Test all lowercase letters
void test_is_alphanumeric_underscore_hyphen_all_lowercase(void) {
    bool result = is_alphanumeric_underscore_hyphen("testuser");
    TEST_ASSERT_TRUE(result);
}

// Test all uppercase letters
void test_is_alphanumeric_underscore_hyphen_all_uppercase(void) {
    bool result = is_alphanumeric_underscore_hyphen("TESTUSER");
    TEST_ASSERT_TRUE(result);
}

// Test all numbers
void test_is_alphanumeric_underscore_hyphen_all_numbers(void) {
    bool result = is_alphanumeric_underscore_hyphen("123456");
    TEST_ASSERT_TRUE(result);
}

// Test string with space (should fail)
void test_is_alphanumeric_underscore_hyphen_with_space(void) {
    bool result = is_alphanumeric_underscore_hyphen("test user");
    TEST_ASSERT_FALSE(result);
}

// Test string with special characters (should fail)
void test_is_alphanumeric_underscore_hyphen_with_special_chars(void) {
    bool result = is_alphanumeric_underscore_hyphen("test!user@123");
    TEST_ASSERT_FALSE(result);
}

// Test string with dot (should fail)
void test_is_alphanumeric_underscore_hyphen_with_dot(void) {
    bool result = is_alphanumeric_underscore_hyphen("test.user");
    TEST_ASSERT_FALSE(result);
}

// Test string with @ sign (should fail)
void test_is_alphanumeric_underscore_hyphen_with_at_sign(void) {
    bool result = is_alphanumeric_underscore_hyphen("test@user");
    TEST_ASSERT_FALSE(result);
}

// Test null parameter
void test_is_alphanumeric_underscore_hyphen_null_parameter(void) {
    bool result = is_alphanumeric_underscore_hyphen(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test empty string (should return true - no invalid chars)
void test_is_alphanumeric_underscore_hyphen_empty_string(void) {
    bool result = is_alphanumeric_underscore_hyphen("");
    TEST_ASSERT_TRUE(result);
}

// Test string with unicode characters (should fail)
void test_is_alphanumeric_underscore_hyphen_unicode_chars(void) {
    bool result = is_alphanumeric_underscore_hyphen("test€user");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_valid_alphanumeric);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_underscore);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_hyphen);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_mixed);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_all_lowercase);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_all_uppercase);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_all_numbers);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_space);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_special_chars);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_dot);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_with_at_sign);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_null_parameter);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_empty_string);
    RUN_TEST(test_is_alphanumeric_underscore_hyphen_unicode_chars);
    
    return UNITY_END();
}
