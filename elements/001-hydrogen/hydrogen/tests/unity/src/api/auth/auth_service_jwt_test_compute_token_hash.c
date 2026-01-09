/*
 * Unity Test File: Auth Service JWT compute_token_hash Function Tests
 * This file contains unit tests for the compute_token_hash function in auth_service_jwt.c
 *
 * Tests: compute_token_hash() - Compute SHA256 hash of token for storage/lookup
 *
 * CHANGELOG:
 * 2026-01-09: Initial version - Tests for token hash generation
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
void test_compute_token_hash_null_input(void);
void test_compute_token_hash_empty_string(void);
void test_compute_token_hash_returns_non_null(void);
void test_compute_token_hash_returns_hex_string(void);
void test_compute_token_hash_consistent_results(void);
void test_compute_token_hash_different_inputs(void);
void test_compute_token_hash_proper_length(void);
void test_compute_token_hash_lowercase_hex(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No teardown needed for this function
}

// Test with NULL input
void test_compute_token_hash_null_input(void) {
    char *hash = compute_token_hash(NULL);
    
    TEST_ASSERT_NULL(hash);
}

// Test with empty string
void test_compute_token_hash_empty_string(void) {
    char *hash = compute_token_hash("");
    
    TEST_ASSERT_NOT_NULL(hash);
    
    // SHA256 of empty string should still produce a hash
    TEST_ASSERT_GREATER_THAN(0, strlen(hash));
    
    free(hash);
}

// Test that valid token returns non-null hash
void test_compute_token_hash_returns_non_null(void) {
    const char *token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.test.signature";
    char *hash = compute_token_hash(token);
    
    TEST_ASSERT_NOT_NULL(hash);
    
    free(hash);
}

// Test that result is a valid base64url string
void test_compute_token_hash_returns_hex_string(void) {
    const char *token = "test_token_12345";
    char *hash = compute_token_hash(token);
    
    TEST_ASSERT_NOT_NULL(hash);
    
    // Hash should contain base64url characters: A-Z, a-z, 0-9, -, _
    for (size_t i = 0; i < strlen(hash); i++) {
        char c = hash[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    c == '-' || c == '_';
        TEST_ASSERT_TRUE(valid);
    }
    
    free(hash);
}

// Test that same input produces same hash (deterministic)
void test_compute_token_hash_consistent_results(void) {
    const char *token = "consistent_test_token";
    
    char *hash1 = compute_token_hash(token);
    char *hash2 = compute_token_hash(token);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    
    // Same input should produce same hash
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);
    
    free(hash1);
    free(hash2);
}

// Test that different inputs produce different hashes
void test_compute_token_hash_different_inputs(void) {
    const char *token1 = "token_one";
    const char *token2 = "token_two";
    
    char *hash1 = compute_token_hash(token1);
    char *hash2 = compute_token_hash(token2);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    
    // Different inputs should produce different hashes
    TEST_ASSERT_FALSE(strcmp(hash1, hash2) == 0);
    
    free(hash1);
    free(hash2);
}

// Test that hash has proper length (SHA256 base64url = ~43 chars)
void test_compute_token_hash_proper_length(void) {
    const char *token = "test_token_for_length_check";
    char *hash = compute_token_hash(token);
    
    TEST_ASSERT_NOT_NULL(hash);
    
    // SHA256 (32 bytes) base64url encoded should be 43 characters
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    
    free(hash);
}

// Test that hash is valid base64url with no padding
void test_compute_token_hash_lowercase_hex(void) {
    const char *token = "TestTokenWithMixedCase";
    char *hash = compute_token_hash(token);
    
    TEST_ASSERT_NOT_NULL(hash);
    
    // base64url should not have padding (no '=' characters)
    TEST_ASSERT_NULL(strchr(hash, '='));
    
    // Should not contain standard base64 chars '+' or '/'
    TEST_ASSERT_NULL(strchr(hash, '+'));
    TEST_ASSERT_NULL(strchr(hash, '/'));
    
    free(hash);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_compute_token_hash_null_input);
    RUN_TEST(test_compute_token_hash_empty_string);
    RUN_TEST(test_compute_token_hash_returns_non_null);
    RUN_TEST(test_compute_token_hash_returns_hex_string);
    RUN_TEST(test_compute_token_hash_consistent_results);
    RUN_TEST(test_compute_token_hash_different_inputs);
    RUN_TEST(test_compute_token_hash_proper_length);
    RUN_TEST(test_compute_token_hash_lowercase_hex);
    
    return UNITY_END();
}
