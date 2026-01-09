/*
 * Unity Test File: Auth Service validate_login_input Function Tests
 * This file contains unit tests for the validate_login_input function in auth_service.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>

// Function prototypes for test functions
void test_validate_login_input_valid_credentials(void);
void test_validate_login_input_null_login_id(void);
void test_validate_login_input_null_password(void);
void test_validate_login_input_null_api_key(void);
void test_validate_login_input_null_timezone(void);
void test_validate_login_input_empty_login_id(void);
void test_validate_login_input_empty_password(void);
void test_validate_login_input_password_too_short(void);
void test_validate_login_input_password_too_long(void);
void test_validate_login_input_login_id_too_long(void);
void test_validate_login_input_invalid_timezone(void);
void test_validate_login_input_valid_timezone_utc(void);
void test_validate_login_input_valid_timezone_america(void);
void test_validate_login_input_valid_timezone_europe(void);
void test_validate_login_input_minimum_length_password(void);
void test_validate_login_input_maximum_length_password(void);

void setUp(void) {
    // No setup needed for this pure function
}

void tearDown(void) {
    // No teardown needed for this pure function
}

// Test valid credentials with proper format
void test_validate_login_input_valid_credentials(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", "America/Vancouver");
    
    TEST_ASSERT_TRUE(result);
}

// Test null login ID
void test_validate_login_input_null_login_id(void) {
    bool result = validate_login_input(NULL, "password123", "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test null password
void test_validate_login_input_null_password(void) {
    bool result = validate_login_input("testuser", NULL, "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test null API key
void test_validate_login_input_null_api_key(void) {
    bool result = validate_login_input("testuser", "password123", NULL, "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test null timezone
void test_validate_login_input_null_timezone(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", NULL);
    
    TEST_ASSERT_FALSE(result);
}

// Test empty login ID
void test_validate_login_input_empty_login_id(void) {
    bool result = validate_login_input("", "password123", "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test empty password
void test_validate_login_input_empty_password(void) {
    bool result = validate_login_input("testuser", "", "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test password too short (less than 8 chars)
void test_validate_login_input_password_too_short(void) {
    bool result = validate_login_input("testuser", "pass", "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test password too long (more than 128 chars)
void test_validate_login_input_password_too_long(void) {
    char long_password[150];
    memset(long_password, 'a', 149);
    long_password[149] = '\0';
    
    bool result = validate_login_input("testuser", long_password, "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test login ID too long (more than 255 chars)
void test_validate_login_input_login_id_too_long(void) {
    char long_login[300];
    memset(long_login, 'a', 299);
    long_login[299] = '\0';
    
    bool result = validate_login_input(long_login, "password123", "api-key-12345", "UTC");
    
    TEST_ASSERT_FALSE(result);
}

// Test invalid timezone format
void test_validate_login_input_invalid_timezone(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", "Invalid/TZ!");
    
    TEST_ASSERT_FALSE(result);
}

// Test valid UTC timezone
void test_validate_login_input_valid_timezone_utc(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", "UTC");
    
    TEST_ASSERT_TRUE(result);
}

// Test valid America timezone
void test_validate_login_input_valid_timezone_america(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", "America/New_York");
    
    TEST_ASSERT_TRUE(result);
}

// Test valid Europe timezone
void test_validate_login_input_valid_timezone_europe(void) {
    bool result = validate_login_input("testuser", "password123", "api-key-12345", "Europe/London");
    
    TEST_ASSERT_TRUE(result);
}

// Test minimum password length (exactly 8 chars)
void test_validate_login_input_minimum_length_password(void) {
    bool result = validate_login_input("testuser", "12345678", "api-key-12345", "UTC");
    
    TEST_ASSERT_TRUE(result);
}

// Test maximum password length (exactly 128 chars)
void test_validate_login_input_maximum_length_password(void) {
    char max_password[129];
    memset(max_password, 'a', 128);
    max_password[128] = '\0';
    
    bool result = validate_login_input("testuser", max_password, "api-key-12345", "UTC");
    
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_login_input_valid_credentials);
    RUN_TEST(test_validate_login_input_null_login_id);
    RUN_TEST(test_validate_login_input_null_password);
    RUN_TEST(test_validate_login_input_null_api_key);
    RUN_TEST(test_validate_login_input_null_timezone);
    RUN_TEST(test_validate_login_input_empty_login_id);
    RUN_TEST(test_validate_login_input_empty_password);
    RUN_TEST(test_validate_login_input_password_too_short);
    RUN_TEST(test_validate_login_input_password_too_long);
    RUN_TEST(test_validate_login_input_login_id_too_long);
    RUN_TEST(test_validate_login_input_invalid_timezone);
    RUN_TEST(test_validate_login_input_valid_timezone_utc);
    RUN_TEST(test_validate_login_input_valid_timezone_america);
    RUN_TEST(test_validate_login_input_valid_timezone_europe);
    RUN_TEST(test_validate_login_input_minimum_length_password);
    RUN_TEST(test_validate_login_input_maximum_length_password);
    
    return UNITY_END();
}
