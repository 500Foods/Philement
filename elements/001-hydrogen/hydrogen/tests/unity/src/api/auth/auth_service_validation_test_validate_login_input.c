/*
 * Unity Test File: Auth Service Validation validate_login_input Function Tests
 * This file contains unit tests for the validate_login_input function in auth_service_validation.c
 *
 * Tests: validate_login_input() - Validate login input parameters
 *
 * CHANGELOG:
 * 2026-01-10: Initial version - Tests for login input validation
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
void test_validate_login_input_valid_parameters(void);
void test_validate_login_input_null_login_id(void);
void test_validate_login_input_null_password(void);
void test_validate_login_input_null_api_key(void);
void test_validate_login_input_null_timezone(void);
void test_validate_login_input_empty_login_id(void);
void test_validate_login_input_short_password(void);
void test_validate_login_input_long_password(void);
void test_validate_login_input_long_login_id(void);
void test_validate_login_input_invalid_timezone(void);
void test_validate_login_input_minimum_valid_password(void);
void test_validate_login_input_maximum_valid_password(void);

void setUp(void) {
    // No setup needed for validation functions
}

void tearDown(void) {
    // No teardown needed for validation functions
}

// Test valid login input parameters
void test_validate_login_input_valid_parameters(void) {
    bool result = validate_login_input("testuser", "Password123!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_TRUE(result);
}

// Test null login_id parameter
void test_validate_login_input_null_login_id(void) {
    bool result = validate_login_input(NULL, "Password123!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test null password parameter
void test_validate_login_input_null_password(void) {
    bool result = validate_login_input("testuser", NULL, "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test null api_key parameter
void test_validate_login_input_null_api_key(void) {
    bool result = validate_login_input("testuser", "Password123!", NULL, "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test null timezone parameter
void test_validate_login_input_null_timezone(void) {
    bool result = validate_login_input("testuser", "Password123!", "test-api-key", NULL);
    TEST_ASSERT_FALSE(result);
}

// Test empty login_id (length < 1)
void test_validate_login_input_empty_login_id(void) {
    bool result = validate_login_input("", "Password123!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test password too short (< 8 chars)
void test_validate_login_input_short_password(void) {
    bool result = validate_login_input("testuser", "Pass12!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test password too long (> 128 chars)
void test_validate_login_input_long_password(void) {
    char long_password[150];
    memset(long_password, 'a', 149);
    long_password[149] = '\0';
    
    bool result = validate_login_input("testuser", long_password, "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test login_id too long (> 255 chars)
void test_validate_login_input_long_login_id(void) {
    char long_login[260];
    memset(long_login, 'a', 259);
    long_login[259] = '\0';
    
    bool result = validate_login_input(long_login, "Password123!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_FALSE(result);
}

// Test invalid timezone format
void test_validate_login_input_invalid_timezone(void) {
    bool result = validate_login_input("testuser", "Password123!", "test-api-key", "Invalid/Timezone!@#");
    TEST_ASSERT_FALSE(result);
}

// Test minimum valid password length (8 chars)
void test_validate_login_input_minimum_valid_password(void) {
    bool result = validate_login_input("testuser", "Pass123!", "test-api-key", "America/Vancouver");
    TEST_ASSERT_TRUE(result);
}

// Test maximum valid password length (128 chars)
void test_validate_login_input_maximum_valid_password(void) {
    char max_password[129];
    memset(max_password, 'a', 128);
    max_password[128] = '\0';
    
    bool result = validate_login_input("testuser", max_password, "test-api-key", "America/Vancouver");
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_login_input_valid_parameters);
    RUN_TEST(test_validate_login_input_null_login_id);
    RUN_TEST(test_validate_login_input_null_password);
    RUN_TEST(test_validate_login_input_null_api_key);
    RUN_TEST(test_validate_login_input_null_timezone);
    RUN_TEST(test_validate_login_input_empty_login_id);
    RUN_TEST(test_validate_login_input_short_password);
    RUN_TEST(test_validate_login_input_long_password);
    RUN_TEST(test_validate_login_input_long_login_id);
    RUN_TEST(test_validate_login_input_invalid_timezone);
    RUN_TEST(test_validate_login_input_minimum_valid_password);
    RUN_TEST(test_validate_login_input_maximum_valid_password);
    
    return UNITY_END();
}
