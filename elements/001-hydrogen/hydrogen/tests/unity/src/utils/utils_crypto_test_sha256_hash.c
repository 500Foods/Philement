/*
 * Unity Test File: utils_sha256_hash()
 * This file contains unit tests for SHA256 hashing
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
char* utils_sha256_hash(const unsigned char* data, size_t length);


// Function prototypes for test functions
void test_sha256_hash_basic_string(void);
void test_sha256_hash_empty_string(void);
void test_sha256_hash_deterministic(void);
void test_sha256_hash_different_inputs(void);
void test_sha256_hash_null_data(void);
void test_sha256_hash_zero_length_valid_data(void);
void test_sha256_hash_binary_data(void);
void test_sha256_hash_all_zeros(void);
void test_sha256_hash_all_ones(void);
void test_sha256_hash_single_byte(void);
void test_sha256_hash_small_data(void);
void test_sha256_hash_medium_data(void);
void test_sha256_hash_large_data(void);
void test_sha256_hash_output_is_base64url(void);
void test_sha256_hash_collision_resistance(void);
void test_sha256_hash_avalanche_effect(void);
void test_sha256_hash_special_characters(void);
void test_sha256_hash_utf8_data(void);
void test_sha256_hash_null_termination(void);
void test_sha256_hash_output_length(void);
void test_sha256_hash_basic_string(void);
void test_sha256_hash_empty_string(void);
void test_sha256_hash_deterministic(void);
void test_sha256_hash_different_inputs(void);
void test_sha256_hash_null_data(void);
void test_sha256_hash_zero_length_valid_data(void);
void test_sha256_hash_binary_data(void);
void test_sha256_hash_all_zeros(void);
void test_sha256_hash_all_ones(void);
void test_sha256_hash_single_byte(void);
void test_sha256_hash_small_data(void);
void test_sha256_hash_medium_data(void);
void test_sha256_hash_large_data(void);
void test_sha256_hash_output_is_base64url(void);
void test_sha256_hash_collision_resistance(void);
void test_sha256_hash_avalanche_effect(void);
void test_sha256_hash_special_characters(void);
void test_sha256_hash_utf8_data(void);
void test_sha256_hash_null_termination(void);
void test_sha256_hash_output_length(void);

void setUp(void) {
    // No setup needed
}


// Function prototypes for test functions
void test_sha256_hash_basic_string(void);
void test_sha256_hash_empty_string(void);
void test_sha256_hash_deterministic(void);
void test_sha256_hash_different_inputs(void);
void test_sha256_hash_null_data(void);
void test_sha256_hash_zero_length_valid_data(void);
void test_sha256_hash_binary_data(void);
void test_sha256_hash_all_zeros(void);
void test_sha256_hash_all_ones(void);
void test_sha256_hash_single_byte(void);
void test_sha256_hash_small_data(void);
void test_sha256_hash_medium_data(void);
void test_sha256_hash_large_data(void);
void test_sha256_hash_output_is_base64url(void);
void test_sha256_hash_collision_resistance(void);
void test_sha256_hash_avalanche_effect(void);
void test_sha256_hash_special_characters(void);
void test_sha256_hash_utf8_data(void);
void test_sha256_hash_null_termination(void);
void test_sha256_hash_output_length(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_sha256_hash_basic_string(void) {
    const char* input = "hello";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    // SHA256("hello") should produce consistent hash
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_empty_string(void) {
    const char* input = "";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    // Empty string should still produce valid hash
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_deterministic(void) {
    const char* input = "test";
    
    // Hash same data twice
    char* result1 = utils_sha256_hash((const unsigned char*)input, strlen(input));
    char* result2 = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Should produce identical hashes
    TEST_ASSERT_EQUAL_STRING(result1, result2);
    
    free(result1);
    free(result2);
}

void test_sha256_hash_different_inputs(void) {
    const char* input1 = "test1";
    const char* input2 = "test2";
    
    char* result1 = utils_sha256_hash((const unsigned char*)input1, strlen(input1));
    char* result2 = utils_sha256_hash((const unsigned char*)input2, strlen(input2));
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Different inputs should produce different hashes
    TEST_ASSERT_NOT_EQUAL(0, strcmp(result1, result2));
    
    free(result1);
    free(result2);
}

// Test null parameter handling
void test_sha256_hash_null_data(void) {
    char* result = utils_sha256_hash(NULL, 10);
    TEST_ASSERT_NULL(result);
}

// Test zero length with valid data
void test_sha256_hash_zero_length_valid_data(void) {
    const char* input = "test";
    char* result = utils_sha256_hash((const unsigned char*)input, 0);
    
    // Zero length should be treated as empty data
    TEST_ASSERT_NOT_NULL(result);
    
    free(result);
}

// Test binary data
void test_sha256_hash_binary_data(void) {
    unsigned char binary[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFE};
    char* result = utils_sha256_hash(binary, sizeof(binary));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_all_zeros(void) {
    unsigned char zeros[32] = {0};
    char* result = utils_sha256_hash(zeros, sizeof(zeros));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_all_ones(void) {
    unsigned char ones[32];
    memset(ones, 0xFF, sizeof(ones));
    char* result = utils_sha256_hash(ones, sizeof(ones));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test various length inputs
void test_sha256_hash_single_byte(void) {
    const unsigned char data[] = {0x42};
    char* result = utils_sha256_hash(data, 1);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_small_data(void) {
    const char* input = "A";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_medium_data(void) {
    const char* input = "The quick brown fox jumps over the lazy dog";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_sha256_hash_large_data(void) {
    size_t size = 10000;
    unsigned char* large_data = malloc(size);
    TEST_ASSERT_NOT_NULL(large_data);
    
    // Fill with pattern
    for (size_t i = 0; i < size; i++) {
        large_data[i] = (unsigned char)(i % 256);
    }
    
    char* result = utils_sha256_hash(large_data, size);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(large_data);
    free(result);
}

// Test output format
void test_sha256_hash_output_is_base64url(void) {
    const char* input = "test";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Base64url should not contain '=' padding
    TEST_ASSERT_NULL(strchr(result, '='));
    
    // Should only contain valid base64url characters: A-Za-z0-9-_
    for (size_t i = 0; i < strlen(result); i++) {
        char c = result[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '-') || (c == '_');
        TEST_ASSERT_TRUE(valid);
    }
    
    free(result);
}

// Test collision resistance (different inputs should produce different hashes)
void test_sha256_hash_collision_resistance(void) {
    const char* input1 = "password1";
    const char* input2 = "password2";
    const char* input3 = "password3";
    
    char* hash1 = utils_sha256_hash((const unsigned char*)input1, strlen(input1));
    char* hash2 = utils_sha256_hash((const unsigned char*)input2, strlen(input2));
    char* hash3 = utils_sha256_hash((const unsigned char*)input3, strlen(input3));
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_NOT_NULL(hash3);
    
    // All three hashes should be different
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash2));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash2, hash3));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash3));
    
    free(hash1);
    free(hash2);
    free(hash3);
}

// Test avalanche effect (small input change produces completely different hash)
void test_sha256_hash_avalanche_effect(void) {
    const char* input1 = "test";
    const char* input2 = "Test";  // Only one bit different
    
    char* hash1 = utils_sha256_hash((const unsigned char*)input1, strlen(input1));
    char* hash2 = utils_sha256_hash((const unsigned char*)input2, strlen(input2));
    
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    
    // Hashes should be completely different
    TEST_ASSERT_NOT_EQUAL(0, strcmp(hash1, hash2));
    
    free(hash1);
    free(hash2);
}

// Test with special characters
void test_sha256_hash_special_characters(void) {
    const char* input = "!@#$%^&*()_+-=[]{}|;':\"<>?,./";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test with UTF-8 data
void test_sha256_hash_utf8_data(void) {
    // UTF-8 encoding of "こんにちは" (hello in Japanese)
    unsigned char utf8[] = {0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93,
                           0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1,
                           0xE3, 0x81, 0xAF};
    char* result = utils_sha256_hash(utf8, sizeof(utf8));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test null termination
void test_sha256_hash_null_termination(void) {
    const char* input = "test";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    size_t len = strlen(result);
    TEST_ASSERT_EQUAL(0, result[len]); // Verify null terminator
    
    free(result);
}

// Test expected output length (SHA256 is 32 bytes, base64url encoded should be ~43 chars)
void test_sha256_hash_output_length(void) {
    const char* input = "test";
    char* result = utils_sha256_hash((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    
    // SHA256 produces 32 bytes (256 bits)
    // Base64url encoding: (32 * 4 + 2) / 3 = 43 characters (no padding)
    size_t length = strlen(result);
    TEST_ASSERT_EQUAL(43, length);
    
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_sha256_hash_basic_string);
    RUN_TEST(test_sha256_hash_empty_string);
    RUN_TEST(test_sha256_hash_deterministic);
    RUN_TEST(test_sha256_hash_different_inputs);
    
    // Parameter validation tests
    RUN_TEST(test_sha256_hash_null_data);
    RUN_TEST(test_sha256_hash_zero_length_valid_data);
    
    // Binary data tests
    RUN_TEST(test_sha256_hash_binary_data);
    RUN_TEST(test_sha256_hash_all_zeros);
    RUN_TEST(test_sha256_hash_all_ones);
    
    // Length variation tests
    RUN_TEST(test_sha256_hash_single_byte);
    RUN_TEST(test_sha256_hash_small_data);
    RUN_TEST(test_sha256_hash_medium_data);
    RUN_TEST(test_sha256_hash_large_data);
    
    // Output format tests
    RUN_TEST(test_sha256_hash_output_is_base64url);
    RUN_TEST(test_sha256_hash_output_length);
    
    // Cryptographic property tests
    RUN_TEST(test_sha256_hash_collision_resistance);
    RUN_TEST(test_sha256_hash_avalanche_effect);
    
    // Special character tests
    RUN_TEST(test_sha256_hash_special_characters);
    RUN_TEST(test_sha256_hash_utf8_data);
    
    // Null termination test
    RUN_TEST(test_sha256_hash_null_termination);
    
    return UNITY_END();
}
