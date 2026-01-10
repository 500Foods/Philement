/*
 * Unity Test File: Auth Service Validation validate_registration_input Function Tests
 * This file contains unit tests for the validate_registration_input function in auth_service_validation.c
 *
 * Tests: validate_registration_input() - Validate registration input parameters
 *
 * CHANGELOG:
 * 2026-01-10: Initial version - Tests for registration input validation
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
void test_validate_registration_input_valid_parameters(void);
void test_validate_registration_input_valid_without_full_name(void);
void test_validate_registration_input_null_username(void);
void test_validate_registration_input_null_password(void);
void test_validate_registration_input_null_email(void);
void test_validate_registration_input_short_username(void);
void test_validate_registration_input_long_username(void);
void test_validate_registration_input_short_password(void);
void test_validate_registration_input_long_password(void);
void test_validate_registration_input_long_email(void);
void test_validate_registration_input_long_full_name(void);
void test_validate_registration_input_invalid_username_chars(void);
void test_validate_registration_input_invalid_email_format(void);
void test_validate_registration_input_minimum_valid_username(void);
void test_validate_registration_input_maximum_valid_username(void);

void setUp(void) {
    // No setup needed for validation functions
}

void tearDown(void) {
    // No teardown needed for validation functions
}

// Test valid registration parameters
void test_validate_registration_input_valid_parameters(void) {
    bool result = validate_registration_input("testuser", "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_TRUE(result);
}

// Test valid registration without full_name (optional)
void test_validate_registration_input_valid_without_full_name(void) {
    bool result = validate_registration_input("testuser", "Password123!", 
                                              "user@example.com", NULL);
    TEST_ASSERT_TRUE(result);
}

// Test null username
void test_validate_registration_input_null_username(void) {
    bool result = validate_registration_input(NULL, "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test null password
void test_validate_registration_input_null_password(void) {
    bool result = validate_registration_input("testuser", NULL, 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test null email
void test_validate_registration_input_null_email(void) {
    bool result = validate_registration_input("testuser", "Password123!", 
                                              NULL, "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test username too short (< 3 chars)
void test_validate_registration_input_short_username(void) {
    bool result = validate_registration_input("ab", "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test username too long (> 50 chars)
void test_validate_registration_input_long_username(void) {
    char long_username[55];
    memset(long_username, 'a', 54);
    long_username[54] = '\0';
    
    bool result = validate_registration_input(long_username, "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test password too short (< 8 chars)
void test_validate_registration_input_short_password(void) {
    bool result = validate_registration_input("testuser", "Pass12!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test password too long (> 128 chars)
void test_validate_registration_input_long_password(void) {
    char long_password[135];
    memset(long_password, 'a', 134);
    long_password[134] = '\0';
    
    bool result = validate_registration_input("testuser", long_password, 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test email too long (> 255 chars)
void test_validate_registration_input_long_email(void) {
    char long_email[260];
    memset(long_email, 'a', 244);  // 244 + 12 = 256 chars (exceeds 255 limit)
    strcpy(long_email + 244, "@example.com");
    
    bool result = validate_registration_input("testuser", "Password123!",
                                              long_email, "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test full_name too long (> 255 chars)
void test_validate_registration_input_long_full_name(void) {
    char long_full_name[260];
    memset(long_full_name, 'A', 259);
    long_full_name[259] = '\0';
    
    bool result = validate_registration_input("testuser", "Password123!", 
                                              "user@example.com", long_full_name);
    TEST_ASSERT_FALSE(result);
}

// Test username with invalid characters (spaces, special chars)
void test_validate_registration_input_invalid_username_chars(void) {
    bool result = validate_registration_input("test user!", "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test invalid email format (no @)
void test_validate_registration_input_invalid_email_format(void) {
    bool result = validate_registration_input("testuser", "Password123!", 
                                              "userexample.com", "Test User");
    TEST_ASSERT_FALSE(result);
}

// Test minimum valid username length (3 chars)
void test_validate_registration_input_minimum_valid_username(void) {
    bool result = validate_registration_input("abc", "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_TRUE(result);
}

// Test maximum valid username length (50 chars)
void test_validate_registration_input_maximum_valid_username(void) {
    char max_username[51];
    memset(max_username, 'a', 50);
    max_username[50] = '\0';
    
    bool result = validate_registration_input(max_username, "Password123!", 
                                              "user@example.com", "Test User");
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_registration_input_valid_parameters);
    RUN_TEST(test_validate_registration_input_valid_without_full_name);
    RUN_TEST(test_validate_registration_input_null_username);
    RUN_TEST(test_validate_registration_input_null_password);
    RUN_TEST(test_validate_registration_input_null_email);
    RUN_TEST(test_validate_registration_input_short_username);
    RUN_TEST(test_validate_registration_input_long_username);
    RUN_TEST(test_validate_registration_input_short_password);
    RUN_TEST(test_validate_registration_input_long_password);
    RUN_TEST(test_validate_registration_input_long_email);
    RUN_TEST(test_validate_registration_input_long_full_name);
    RUN_TEST(test_validate_registration_input_invalid_username_chars);
    RUN_TEST(test_validate_registration_input_invalid_email_format);
    RUN_TEST(test_validate_registration_input_minimum_valid_username);
    RUN_TEST(test_validate_registration_input_maximum_valid_username);
    
    return UNITY_END();
}
