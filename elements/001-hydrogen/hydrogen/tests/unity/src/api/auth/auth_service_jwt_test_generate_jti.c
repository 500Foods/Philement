/*
 * Unity Test File: Auth Service JWT generate_jti Function Tests
 * This file contains unit tests for the generate_jti function in auth_service_jwt.c
 *
 * Tests: generate_jti() - Generate a unique JWT ID (JTI)
 *
 * CHANGELOG:
 * 2026-01-09: Initial version - Tests for JWT ID generation
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>

// Function prototypes for test functions
void test_generate_jti_returns_non_null(void);
void test_generate_jti_returns_base64url_string(void);
void test_generate_jti_generates_unique_values(void);
void test_generate_jti_proper_length(void);
void test_generate_jti_no_padding(void);
void test_generate_jti_contains_valid_characters(void);
void test_generate_jti_multiple_calls_unique(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No teardown needed for this function
}

// Test that generate_jti returns a non-null value
void test_generate_jti_returns_non_null(void) {
    char *jti = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti);
    
    free(jti);
}

// Test that result is a valid base64url string
void test_generate_jti_returns_base64url_string(void) {
    char *jti = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti);
    
    // Base64url should only contain: A-Z, a-z, 0-9, -, _
    for (size_t i = 0; i < strlen(jti); i++) {
        char c = jti[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    c == '-' || c == '_';
        TEST_ASSERT_TRUE(valid);
    }
    
    free(jti);
}

// Test that multiple calls generate unique values
void test_generate_jti_generates_unique_values(void) {
    char *jti1 = generate_jti();
    char *jti2 = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti1);
    TEST_ASSERT_NOT_NULL(jti2);
    
    // JTIs should be different (cryptographically random)
    TEST_ASSERT_FALSE(strcmp(jti1, jti2) == 0);
    
    free(jti1);
    free(jti2);
}

// Test that JTI has proper length
// 16 bytes base64url encoded should be ~22 characters
void test_generate_jti_proper_length(void) {
    char *jti = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti);
    
    // Base64url encoding of 16 bytes should be around 22 chars (no padding)
    size_t len = strlen(jti);
    TEST_ASSERT_GREATER_OR_EQUAL(20, len);  // At least 20 chars
    TEST_ASSERT_LESS_OR_EQUAL(24, len);     // At most 24 chars
    
    free(jti);
}

// Test that JTI has no padding characters (base64url should not have =)
void test_generate_jti_no_padding(void) {
    char *jti = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti);
    
    // Base64url should not contain '=' padding
    TEST_ASSERT_NULL(strchr(jti, '='));
    
    free(jti);
}

// Test that JTI contains only valid base64url characters
void test_generate_jti_contains_valid_characters(void) {
    char *jti = generate_jti();
    
    TEST_ASSERT_NOT_NULL(jti);
    
    // Should not contain '+' or '/' (standard base64)
    TEST_ASSERT_NULL(strchr(jti, '+'));
    TEST_ASSERT_NULL(strchr(jti, '/'));
    
    free(jti);
}

// Test that 10 consecutive calls produce 10 unique IDs
void test_generate_jti_multiple_calls_unique(void) {
    char *jtis[10];
    
    // Generate 10 JTIs
    for (int i = 0; i < 10; i++) {
        jtis[i] = generate_jti();
        TEST_ASSERT_NOT_NULL(jtis[i]);
    }
    
    // Verify all are unique
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            TEST_ASSERT_FALSE(strcmp(jtis[i], jtis[j]) == 0);
        }
    }
    
    // Cleanup
    for (int i = 0; i < 10; i++) {
        free(jtis[i]);
    }
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_generate_jti_returns_non_null);
    RUN_TEST(test_generate_jti_returns_base64url_string);
    RUN_TEST(test_generate_jti_generates_unique_values);
    RUN_TEST(test_generate_jti_proper_length);
    RUN_TEST(test_generate_jti_no_padding);
    RUN_TEST(test_generate_jti_contains_valid_characters);
    RUN_TEST(test_generate_jti_multiple_calls_unique);
    
    return UNITY_END();
}
