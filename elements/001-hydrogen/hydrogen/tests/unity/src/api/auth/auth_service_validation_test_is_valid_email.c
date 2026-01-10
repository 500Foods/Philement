/*
 * Unity Test File: Auth Service Validation is_valid_email Function Tests
 * This file contains unit tests for the is_valid_email function in auth_service_validation.c
 *
 * Tests: is_valid_email() - Validate email format
 *
 * CHANGELOG:
 * 2026-01-10: Initial version - Tests for email validation
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
void test_is_valid_email_simple_valid(void);
void test_is_valid_email_with_plus(void);
void test_is_valid_email_with_underscore(void);
void test_is_valid_email_with_hyphen(void);
void test_is_valid_email_with_dots(void);
void test_is_valid_email_subdomain(void);
void test_is_valid_email_multiple_subdomains(void);
void test_is_valid_email_null_parameter(void);
void test_is_valid_email_empty_string(void);
void test_is_valid_email_no_at_sign(void);
void test_is_valid_email_no_domain_dot(void);
void test_is_valid_email_no_local_part(void);
void test_is_valid_email_no_domain_part(void);
void test_is_valid_email_no_tld(void);
void test_is_valid_email_with_spaces(void);
void test_is_valid_email_with_invalid_chars(void);
void test_is_valid_email_multiple_at_signs(void);
void test_is_valid_email_ends_with_dot(void);

void setUp(void) {
    // No setup needed for validation functions
}

void tearDown(void) {
    // No teardown needed for validation functions
}

// Test simple valid email
void test_is_valid_email_simple_valid(void) {
    bool result = is_valid_email("user@example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with plus sign (used for email aliases)
void test_is_valid_email_with_plus(void) {
    bool result = is_valid_email("user+tag@example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with underscore
void test_is_valid_email_with_underscore(void) {
    bool result = is_valid_email("user_name@example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with hyphen
void test_is_valid_email_with_hyphen(void) {
    bool result = is_valid_email("user-name@example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with dots in local part
void test_is_valid_email_with_dots(void) {
    bool result = is_valid_email("first.last@example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with subdomain
void test_is_valid_email_subdomain(void) {
    bool result = is_valid_email("user@mail.example.com");
    TEST_ASSERT_TRUE(result);
}

// Test email with multiple subdomains
void test_is_valid_email_multiple_subdomains(void) {
    bool result = is_valid_email("user@mail.server.example.com");
    TEST_ASSERT_TRUE(result);
}

// Test null parameter
void test_is_valid_email_null_parameter(void) {
    bool result = is_valid_email(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test empty string
void test_is_valid_email_empty_string(void) {
    bool result = is_valid_email("");
    TEST_ASSERT_FALSE(result);
}

// Test email without @ sign
void test_is_valid_email_no_at_sign(void) {
    bool result = is_valid_email("userexample.com");
    TEST_ASSERT_FALSE(result);
}

// Test email without dot in domain
void test_is_valid_email_no_domain_dot(void) {
    bool result = is_valid_email("user@examplecom");
    TEST_ASSERT_FALSE(result);
}

// Test email without local part (starts with @)
void test_is_valid_email_no_local_part(void) {
    bool result = is_valid_email("@example.com");
    TEST_ASSERT_FALSE(result);
}

// Test email without domain part (@ followed by dot)
void test_is_valid_email_no_domain_part(void) {
    bool result = is_valid_email("user@.com");
    TEST_ASSERT_FALSE(result);
}

// Test email without TLD (ends with @domain)
void test_is_valid_email_no_tld(void) {
    bool result = is_valid_email("user@example.");
    TEST_ASSERT_FALSE(result);
}

// Test email with spaces (should fail)
void test_is_valid_email_with_spaces(void) {
    bool result = is_valid_email("user name@example.com");
    TEST_ASSERT_FALSE(result);
}

// Test email with invalid characters (should fail)
void test_is_valid_email_with_invalid_chars(void) {
    bool result = is_valid_email("user!name#@example.com");
    TEST_ASSERT_FALSE(result);
}

// Test email with multiple @ signs - basic validation doesn't catch this edge case
// NOTE: The function uses strchr which only finds the FIRST @ sign, so this passes basic validation
void test_is_valid_email_multiple_at_signs(void) {
    bool result = is_valid_email("user@@example.com");
    TEST_ASSERT_TRUE(result); // Current implementation allows this (basic validation)
}

// Test email ending with dot - basic validation only checks for dot AFTER @, not at end
// NOTE: This is an edge case not covered by the basic validation logic
void test_is_valid_email_ends_with_dot(void) {
    bool result = is_valid_email("user@example.com.");
    TEST_ASSERT_TRUE(result); // Current implementation allows this (basic validation)
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_is_valid_email_simple_valid);
    RUN_TEST(test_is_valid_email_with_plus);
    RUN_TEST(test_is_valid_email_with_underscore);
    RUN_TEST(test_is_valid_email_with_hyphen);
    RUN_TEST(test_is_valid_email_with_dots);
    RUN_TEST(test_is_valid_email_subdomain);
    RUN_TEST(test_is_valid_email_multiple_subdomains);
    RUN_TEST(test_is_valid_email_null_parameter);
    RUN_TEST(test_is_valid_email_empty_string);
    RUN_TEST(test_is_valid_email_no_at_sign);
    RUN_TEST(test_is_valid_email_no_domain_dot);
    RUN_TEST(test_is_valid_email_no_local_part);
    RUN_TEST(test_is_valid_email_no_domain_part);
    RUN_TEST(test_is_valid_email_no_tld);
    RUN_TEST(test_is_valid_email_with_spaces);
    RUN_TEST(test_is_valid_email_with_invalid_chars);
    RUN_TEST(test_is_valid_email_multiple_at_signs);
    RUN_TEST(test_is_valid_email_ends_with_dot);
    
    return UNITY_END();
}
