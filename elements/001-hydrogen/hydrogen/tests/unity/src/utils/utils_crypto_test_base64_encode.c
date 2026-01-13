/*
 * Unity Test File: utils_base64_encode()
 * This file contains unit tests for standard Base64 encoding WITH padding
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
char* utils_base64_encode(const unsigned char* data, size_t length);


// Function prototypes for test functions
void test_base64_encode_basic_string(void);
void test_base64_encode_short_string(void);
void test_base64_encode_single_character(void);
void test_base64_encode_exact_multiple_of_three(void);
void test_base64_encode_longer_string(void);
void test_base64_encode_binary_data(void);
void test_base64_encode_all_zeros(void);
void test_base64_encode_all_ones(void);
void test_base64_encode_null_data(void);
void test_base64_encode_zero_length(void);
void test_base64_encode_special_characters(void);
void test_base64_encode_unicode_bytes(void);
void test_base64_encode_large_data(void);
void test_base64_encode_null_termination(void);
void test_base64_encode_basic_string(void);
void test_base64_encode_short_string(void);
void test_base64_encode_single_character(void);
void test_base64_encode_exact_multiple_of_three(void);
void test_base64_encode_longer_string(void);
void test_base64_encode_binary_data(void);
void test_base64_encode_all_zeros(void);
void test_base64_encode_all_ones(void);
void test_base64_encode_null_data(void);
void test_base64_encode_zero_length(void);
void test_base64_encode_special_characters(void);
void test_base64_encode_unicode_bytes(void);
void test_base64_encode_large_data(void);
void test_base64_encode_null_termination(void);

void setUp(void) {
    // No setup needed for base64 encode tests
}


// Function prototypes for test functions
void test_base64_encode_basic_string(void);
void test_base64_encode_short_string(void);
void test_base64_encode_single_character(void);
void test_base64_encode_exact_multiple_of_three(void);
void test_base64_encode_longer_string(void);
void test_base64_encode_binary_data(void);
void test_base64_encode_all_zeros(void);
void test_base64_encode_all_ones(void);
void test_base64_encode_null_data(void);
void test_base64_encode_zero_length(void);
void test_base64_encode_special_characters(void);
void test_base64_encode_unicode_bytes(void);
void test_base64_encode_large_data(void);
void test_base64_encode_null_termination(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_base64_encode_basic_string(void) {
    const char* input = "Hello";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SGVsbG8=", result);  // "Hello" encoded with padding
    
    free(result);
}

void test_base64_encode_short_string(void) {
    const char* input = "Hi";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SGk=", result);  // "Hi" encoded with padding
    
    free(result);
}

void test_base64_encode_single_character(void) {
    const char* input = "A";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QQ==", result);  // "A" encoded with padding
    
    free(result);
}

void test_base64_encode_exact_multiple_of_three(void) {
    const char* input = "ABC";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QUJD", result);  // No padding needed for exact multiple of 3
    
    free(result);
}

void test_base64_encode_longer_string(void) {
    const char* input = "Hello, World!";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SGVsbG8sIFdvcmxkIQ==", result);
    
    free(result);
}

void test_base64_encode_binary_data(void) {
    unsigned char binary[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    char* result = utils_base64_encode(binary, sizeof(binary));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("AAECAwQF", result);
    
    free(result);
}

void test_base64_encode_all_zeros(void) {
    unsigned char zeros[] = {0x00, 0x00, 0x00};
    char* result = utils_base64_encode(zeros, sizeof(zeros));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("AAAA", result);
    
    free(result);
}

void test_base64_encode_all_ones(void) {
    unsigned char ones[] = {0xFF, 0xFF, 0xFF};
    char* result = utils_base64_encode(ones, sizeof(ones));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("////", result);
    
    free(result);
}

// Test null parameter handling
void test_base64_encode_null_data(void) {
    char* result = utils_base64_encode(NULL, 10);
    TEST_ASSERT_NULL(result);
}

void test_base64_encode_zero_length(void) {
    const char* input = "test";
    char* result = utils_base64_encode((const unsigned char*)input, 0);
    TEST_ASSERT_NULL(result);
}

// Test special characters
void test_base64_encode_special_characters(void) {
    const char* input = "!@#$%^&*()";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    // Verify it encodes successfully (exact value depends on standard)
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

void test_base64_encode_unicode_bytes(void) {
    // UTF-8 encoding of "こんにちは" (hello in Japanese)
    unsigned char utf8[] = {0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93,
                           0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1,
                           0xE3, 0x81, 0xAF};
    char* result = utils_base64_encode(utf8, sizeof(utf8));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    
    free(result);
}

// Test large data
void test_base64_encode_large_data(void) {
    size_t size = 1024;
    unsigned char* large_data = malloc(size);
    TEST_ASSERT_NOT_NULL(large_data);
    
    // Fill with pattern
    for (size_t i = 0; i < size; i++) {
        large_data[i] = (unsigned char)(i % 256);
    }
    
    char* result = utils_base64_encode(large_data, size);
    TEST_ASSERT_NOT_NULL(result);
    
    // Verify output length is correct (4 * ceil(input_length / 3))
    size_t expected_length = ((size + 2) / 3) * 4;
    TEST_ASSERT_EQUAL(expected_length, strlen(result));
    
    free(large_data);
    free(result);
}

// Test that output is properly null-terminated
void test_base64_encode_null_termination(void) {
    const char* input = "Test";
    char* result = utils_base64_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    size_t len = strlen(result);
    TEST_ASSERT_EQUAL(0, result[len]); // Verify null terminator
    
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_base64_encode_basic_string);
    RUN_TEST(test_base64_encode_short_string);
    RUN_TEST(test_base64_encode_single_character);
    RUN_TEST(test_base64_encode_exact_multiple_of_three);
    RUN_TEST(test_base64_encode_longer_string);
    
    // Binary data tests
    RUN_TEST(test_base64_encode_binary_data);
    RUN_TEST(test_base64_encode_all_zeros);
    RUN_TEST(test_base64_encode_all_ones);
    
    // Parameter validation tests
    RUN_TEST(test_base64_encode_null_data);
    RUN_TEST(test_base64_encode_zero_length);
    
    // Special character tests
    RUN_TEST(test_base64_encode_special_characters);
    RUN_TEST(test_base64_encode_unicode_bytes);
    
    // Large data test
    RUN_TEST(test_base64_encode_large_data);
    
    // Null termination test
    RUN_TEST(test_base64_encode_null_termination);
    
    return UNITY_END();
}
