/*
 * Unity Unit Tests for auth_service_jwt.c - compute_password_hash()
 *
 * Tests the password hashing functionality that combines password with account ID
 * Uses SHA256 hashing via utils_password_hash()
 *
 * CHANGELOG:
 * 2026-01-09: Initial version - Tests for password hashing
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/utils/utils_crypto.h>

// Function prototypes for test functions
void test_compute_password_hash_null_password(void);
void test_compute_password_hash_empty_password(void);
void test_compute_password_hash_valid_password(void);
void test_compute_password_hash_consistency(void);
void test_compute_password_hash_different_account_id(void);
void test_compute_password_hash_different_password(void);
void test_compute_password_hash_long_password(void);
void test_compute_password_hash_special_characters(void);
void test_compute_password_hash_unicode(void);
void test_compute_password_hash_negative_account_id(void);
void test_compute_password_hash_zero_account_id(void);

/* Test Setup and Teardown */
void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

/* Test 1: Null password returns NULL */
void test_compute_password_hash_null_password(void) {
    char* hash = compute_password_hash(NULL, 123);
    TEST_ASSERT_NULL(hash);
}

/* Test 2: Empty password returns valid hash */
void test_compute_password_hash_empty_password(void) {
    char* hash = compute_password_hash("", 123);
    TEST_ASSERT_NOT_NULL(hash);
    // SHA256 (32 bytes) base64url-encoded is ~43 chars (no padding)
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    free(hash);
}

/* Test 3: Valid password with account ID returns hash */
void test_compute_password_hash_valid_password(void) {
    char* hash = compute_password_hash("TestPassword123!", 456);
    TEST_ASSERT_NOT_NULL(hash);
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    
    // Verify it's all standard base64 characters (A-Z, a-z, 0-9, +, /, =)
    for (size_t i = 0; i < len; i++) {
        char c = hash[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    c == '+' || c == '/' || c == '=';
        TEST_ASSERT_TRUE(valid);
    }
    
    free(hash);
}

/* Test 4: Same password + account_id produces same hash (consistency) */
void test_compute_password_hash_consistency(void) {
    char* hash1 = compute_password_hash("MyPassword", 789);
    char* hash2 = compute_password_hash("MyPassword", 789);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);
    
    free(hash1);
    free(hash2);
}

/* Test 5: Different account_id produces different hash */
void test_compute_password_hash_different_account_id(void) {
    char* hash1 = compute_password_hash("SamePassword", 100);
    char* hash2 = compute_password_hash("SamePassword", 200);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash2));
    
    free(hash1);
    free(hash2);
}

/* Test 6: Different password produces different hash */
void test_compute_password_hash_different_password(void) {
    char* hash1 = compute_password_hash("Password1", 123);
    char* hash2 = compute_password_hash("Password2", 123);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash2));
    
    free(hash1);
    free(hash2);
}

/* Test 7: Long password (edge case) */
void test_compute_password_hash_long_password(void) {
    // Create a 1000 character password
    char long_password[1001];
    memset(long_password, 'A', 1000);
    long_password[1000] = '\0';
    
    char* hash = compute_password_hash(long_password, 999);
    TEST_ASSERT_NOT_NULL(hash);
    // Still ~43 chars regardless of input length
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    free(hash);
}

/* Test 8: Special characters in password */
void test_compute_password_hash_special_characters(void) {
    char* hash = compute_password_hash("P@ssw0rd!#$%^&*()", 555);
    TEST_ASSERT_NOT_NULL(hash);
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    
    // Verify it's valid standard base64
    for (size_t i = 0; i < len; i++) {
        char c = hash[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    c == '+' || c == '/' || c == '=';
        TEST_ASSERT_TRUE(valid);
    }
    
    free(hash);
}

/* Test 9: Unicode characters in password */
void test_compute_password_hash_unicode(void) {
    char* hash = compute_password_hash("пароль密码🔒", 777);
    TEST_ASSERT_NOT_NULL(hash);
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    free(hash);
}

/* Test 10: Negative account ID (edge case) */
void test_compute_password_hash_negative_account_id(void) {
    // Negative account_id shouldn't happen but test behavior
    char* hash1 = compute_password_hash("Password", -1);
    char* hash2 = compute_password_hash("Password", -2);
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    // Different negative IDs should produce different hashes
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash2));
    
    free(hash1);
    free(hash2);
}

/* Test 11: Zero account ID */
void test_compute_password_hash_zero_account_id(void) {
    char* hash = compute_password_hash("TestPass", 0);
    TEST_ASSERT_NOT_NULL(hash);
    size_t len = strlen(hash);
    TEST_ASSERT_GREATER_OR_EQUAL(42, len);
    TEST_ASSERT_LESS_OR_EQUAL(44, len);
    free(hash);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_compute_password_hash_null_password);
    RUN_TEST(test_compute_password_hash_empty_password);
    RUN_TEST(test_compute_password_hash_valid_password);
    RUN_TEST(test_compute_password_hash_consistency);
    RUN_TEST(test_compute_password_hash_different_account_id);
    RUN_TEST(test_compute_password_hash_different_password);
    RUN_TEST(test_compute_password_hash_long_password);
    RUN_TEST(test_compute_password_hash_special_characters);
    RUN_TEST(test_compute_password_hash_unicode);
    RUN_TEST(test_compute_password_hash_negative_account_id);
    RUN_TEST(test_compute_password_hash_zero_account_id);
    
    return UNITY_END();
}
