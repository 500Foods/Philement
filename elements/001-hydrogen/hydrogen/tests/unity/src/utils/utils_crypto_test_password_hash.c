/*
 * Unity Test File: utils_password_hash()
 * This file contains unit tests for password hashing
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
char* utils_password_hash(const char* password, int account_id);


// Function prototypes for test functions
void test_password_hash_basic(void);
void test_password_hash_simple_password(void);
void test_password_hash_deterministic(void);
void test_password_hash_different_account_ids(void);
void test_password_hash_different_passwords(void);
void test_password_hash_null_password(void);
void test_password_hash_empty_password(void);
void test_password_hash_zero_account_id(void);
void test_password_hash_negative_account_id(void);
void test_password_hash_large_account_id(void);
void test_password_hash_long_password(void);
void test_password_hash_special_characters(void);
void test_password_hash_with_spaces(void);
void test_password_hash_unicode(void);
void test_password_hash_avalanche_effect(void);
void test_password_hash_output_format(void);
void test_password_hash_output_length(void);
void test_password_hash_collision_resistance(void);
void test_password_hash_salt_effect(void);
void test_password_hash_null_termination(void);
void test_password_hash_common_patterns(void);
void test_password_hash_basic(void);
void test_password_hash_simple_password(void);
void test_password_hash_deterministic(void);
void test_password_hash_different_account_ids(void);
void test_password_hash_different_passwords(void);
void test_password_hash_null_password(void);
void test_password_hash_empty_password(void);
void test_password_hash_zero_account_id(void);
void test_password_hash_negative_account_id(void);
void test_password_hash_large_account_id(void);
void test_password_hash_long_password(void);
void test_password_hash_special_characters(void);
void test_password_hash_with_spaces(void);
void test_password_hash_unicode(void);
void test_password_hash_avalanche_effect(void);
void test_password_hash_output_format(void);
void test_password_hash_output_length(void);
void test_password_hash_collision_resistance(void);
void test_password_hash_salt_effect(void);
void test_password_hash_null_termination(void);
void test_password_hash_common_patterns(void);

void setUp(void) {
    // No setup needed
}


// Function prototypes for test functions
void test_password_hash_basic(void);
void test_password_hash_simple_password(void);
void test_password_hash_deterministic(void);
void test_password_hash_different_account_ids(void);
void test_password_hash_different_passwords(void);
void test_password_hash_null_password(void);
void test_password_hash_empty_password(void);
void test_password_hash_zero_account_id(void);
void test_password_hash_negative_account_id(void);
void test_password_hash_large_account_id(void);
void test_password_hash_long_password(void);
void test_password_hash_special_characters(void);
void test_password_hash_with_spaces(void);
void test_password_hash_unicode(void);
void test_password_hash_avalanche_effect(void);
void test_password_hash_output_format(void);
void test_password_hash_output_length(void);
void test_password_hash_collision_resistance(void);
void test_password_hash_salt_effect(void);
void test_password_hash_null_termination(void);
void test_password_hash_common_patterns(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_password_hash_basic(void) {
    const char* password = "mypassword";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_password_hash_simple_password(void) {
    const char* password = "test";
    int account_id = 1;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test deterministic behavior
void test_password_hash_deterministic(void) {
    const char* password = "password123";
    int account_id = 100;
    
    char* result1 = utils_password_hash(password, account_id);
    char* result2 = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Same password + account_id should produce same hash
    TEST_ASSERT_EQUAL_STRING(result1, result2);
    
    free(result1);
    free(result2);
}

// Test different account IDs produce different hashes
void test_password_hash_different_account_ids(void) {
    const char* password = "password";
    int account_id1 = 1;
    int account_id2 = 2;
    
    char* result1 = utils_password_hash(password, account_id1);
    char* result2 = utils_password_hash(password, account_id2);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Different account IDs should produce different hashes
    TEST_ASSERT_NOT_EQUAL(0, strcmp(result1, result2));
    
    free(result1);
    free(result2);
}

// Test different passwords produce different hashes
void test_password_hash_different_passwords(void) {
    const char* password1 = "password1";
    const char* password2 = "password2";
    int account_id = 12345;
    
    char* result1 = utils_password_hash(password1, account_id);
    char* result2 = utils_password_hash(password2, account_id);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Different passwords should produce different hashes
    TEST_ASSERT_NOT_EQUAL(0, strcmp(result1, result2));
    
    free(result1);
    free(result2);
}

// Test null parameter handling
void test_password_hash_null_password(void) {
    int account_id = 12345;
    char* result = utils_password_hash(NULL, account_id);
    TEST_ASSERT_NULL(result);
}

// Test empty password
void test_password_hash_empty_password(void) {
    const char* password = "";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    // Empty password should still hash (account_id provides salt)
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test various account ID values
void test_password_hash_zero_account_id(void) {
    const char* password = "password";
    int account_id = 0;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_password_hash_negative_account_id(void) {
    const char* password = "password";
    int account_id = -1;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_password_hash_large_account_id(void) {
    const char* password = "password";
    int account_id = 999999999;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test long password
void test_password_hash_long_password(void) {
    char long_password[256];
    memset(long_password, 'p', sizeof(long_password) - 1);
    long_password[sizeof(long_password) - 1] = '\0';
    int account_id = 12345;
    
    char* result = utils_password_hash(long_password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test password with special characters
void test_password_hash_special_characters(void) {
    const char* password = "!@#$%^&*()_+-=[]{}|;':\"<>?,./";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test password with spaces
void test_password_hash_with_spaces(void) {
    const char* password = "my password with spaces";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test password with Unicode characters
void test_password_hash_unicode(void) {
    const char* password = "пароль123";  // "password" in Cyrillic
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test avalanche effect (small password change produces different hash)
void test_password_hash_avalanche_effect(void) {
    const char* password1 = "password";
    const char* password2 = "Password";  // Only capitalization change
    int account_id = 12345;
    
    char* result1 = utils_password_hash(password1, account_id);
    char* result2 = utils_password_hash(password2, account_id);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Small change should produce completely different hash
    TEST_ASSERT_NOT_EQUAL(0, strcmp(result1, result2));
    
    free(result1);
    free(result2);
}

// Test output format (should be base64 with padding)
void test_password_hash_output_format(void) {
    const char* password = "test";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Standard base64 can have '=' padding
    // Check for valid base64 characters: A-Za-z0-9+/=
    for (size_t i = 0; i < strlen(result); i++) {
        char c = result[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '+') || (c == '/') || (c == '=');
        TEST_ASSERT_TRUE(valid);
    }
    
    free(result);
}

// Test output length (SHA256 is 32 bytes, base64 encoded with padding should be 44 chars)
void test_password_hash_output_length(void) {
    const char* password = "test";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // SHA256 produces 32 bytes
    // Base64 encoding with padding: ((32 + 2) / 3) * 4 = 44 characters
    size_t length = strlen(result);
    TEST_ASSERT_EQUAL(44, length);
    
    free(result);
}

// Test collision resistance
void test_password_hash_collision_resistance(void) {
    const char* passwords[] = {"pass1", "pass2", "pass3", "pass4", "pass5"};
    int num_passwords = 5;
    int account_id = 12345;
    char* hashes[5];
    
    // Generate hashes
    for (int i = 0; i < num_passwords; i++) {
        hashes[i] = utils_password_hash(passwords[i], account_id);
        TEST_ASSERT_NOT_NULL(hashes[i]);
    }
    
    // Verify all hashes are different
    for (int i = 0; i < num_passwords; i++) {
        for (int j = i + 1; j < num_passwords; j++) {
            TEST_ASSERT_NOT_EQUAL(0, strcmp(hashes[i], hashes[j]));
        }
    }
    
    // Clean up
    for (int i = 0; i < num_passwords; i++) {
        free(hashes[i]);
    }
}

// Test that same password with different account_id produces different hash
void test_password_hash_salt_effect(void) {
    const char* password = "password";
    const int ids[] = {1, 2, 3, 4, 5};
    int num_ids = 5;
    char* hashes[5];
    
    // Generate hashes
    for (int i = 0; i < num_ids; i++) {
        hashes[i] = utils_password_hash(password, ids[i]);
        TEST_ASSERT_NOT_NULL(hashes[i]);
    }
    
    // Verify all hashes are different (account_id acts as salt)
    for (int i = 0; i < num_ids; i++) {
        for (int j = i + 1; j < num_ids; j++) {
            TEST_ASSERT_NOT_EQUAL(0, strcmp(hashes[i], hashes[j]));
        }
    }
    
    // Clean up
    for (int i = 0; i < num_ids; i++) {
        free(hashes[i]);
    }
}

// Test null termination
void test_password_hash_null_termination(void) {
    const char* password = "test";
    int account_id = 12345;
    
    char* result = utils_password_hash(password, account_id);
    
    TEST_ASSERT_NOT_NULL(result);
    size_t len = strlen(result);
    TEST_ASSERT_EQUAL(0, result[len]); // Verify null terminator
    
    free(result);
}

// Test common password patterns
void test_password_hash_common_patterns(void) {
    const char* passwords[] = {
        "123456",
        "password",
        "qwerty",
        "admin",
        "letmein"
    };
    int account_id = 12345;
    
    for (size_t i = 0; i < 5; i++) {
        char* result = utils_password_hash(passwords[i], account_id);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_GREATER_THAN(0, strlen(result));
        free(result);
    }
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_password_hash_basic);
    RUN_TEST(test_password_hash_simple_password);
    RUN_TEST(test_password_hash_deterministic);
    
    // Different inputs tests
    RUN_TEST(test_password_hash_different_account_ids);
    RUN_TEST(test_password_hash_different_passwords);
    
    // Parameter validation tests
    RUN_TEST(test_password_hash_null_password);
    RUN_TEST(test_password_hash_empty_password);
    
    // Account ID variation tests
    RUN_TEST(test_password_hash_zero_account_id);
    RUN_TEST(test_password_hash_negative_account_id);
    RUN_TEST(test_password_hash_large_account_id);
    
    // Password variation tests
    RUN_TEST(test_password_hash_long_password);
    RUN_TEST(test_password_hash_special_characters);
    RUN_TEST(test_password_hash_with_spaces);
    RUN_TEST(test_password_hash_unicode);
    
    // Cryptographic property tests
    RUN_TEST(test_password_hash_avalanche_effect);
    RUN_TEST(test_password_hash_collision_resistance);
    RUN_TEST(test_password_hash_salt_effect);
    
    // Output format tests
    RUN_TEST(test_password_hash_output_format);
    RUN_TEST(test_password_hash_output_length);
    RUN_TEST(test_password_hash_null_termination);
    
    // Common password tests
    RUN_TEST(test_password_hash_common_patterns);
    
    return UNITY_END();
}
