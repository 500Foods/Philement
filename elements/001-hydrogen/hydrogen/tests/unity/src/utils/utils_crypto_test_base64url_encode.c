/*
 * Unity Test File: utils_base64url_encode()
 * This file contains unit tests for URL-safe Base64 encoding WITHOUT padding
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
char* utils_base64url_encode(const unsigned char* data, size_t length);


// Function prototypes for test functions
void test_base64url_encode_basic_string(void);
void test_base64url_encode_short_string(void);
void test_base64url_encode_single_character(void);
void test_base64url_encode_exact_multiple_of_three(void);
void test_base64url_encode_url_safe_chars(void);
void test_base64url_encode_no_padding(void);
void test_base64url_encode_longer_string(void);
void test_base64url_encode_binary_data(void);
void test_base64url_encode_all_zeros(void);
void test_base64url_encode_null_data(void);
void test_base64url_encode_zero_length(void);
void test_base64url_encode_length_1(void);
void test_base64url_encode_length_2(void);
void test_base64url_encode_length_3(void);
void test_base64url_encode_large_data(void);
void test_base64url_encode_jwt_header(void);
void test_base64url_encode_null_termination(void);
void test_base64url_encode_output_length(void);
void test_base64url_encode_basic_string(void);
void test_base64url_encode_short_string(void);
void test_base64url_encode_single_character(void);
void test_base64url_encode_exact_multiple_of_three(void);
void test_base64url_encode_url_safe_chars(void);
void test_base64url_encode_no_padding(void);
void test_base64url_encode_longer_string(void);
void test_base64url_encode_binary_data(void);
void test_base64url_encode_all_zeros(void);
void test_base64url_encode_null_data(void);
void test_base64url_encode_zero_length(void);
void test_base64url_encode_length_1(void);
void test_base64url_encode_length_2(void);
void test_base64url_encode_length_3(void);
void test_base64url_encode_large_data(void);
void test_base64url_encode_jwt_header(void);
void test_base64url_encode_null_termination(void);
void test_base64url_encode_output_length(void);

void setUp(void) {
    // No setup needed
}


// Function prototypes for test functions
void test_base64url_encode_basic_string(void);
void test_base64url_encode_short_string(void);
void test_base64url_encode_single_character(void);
void test_base64url_encode_exact_multiple_of_three(void);
void test_base64url_encode_url_safe_chars(void);
void test_base64url_encode_no_padding(void);
void test_base64url_encode_longer_string(void);
void test_base64url_encode_binary_data(void);
void test_base64url_encode_all_zeros(void);
void test_base64url_encode_null_data(void);
void test_base64url_encode_zero_length(void);
void test_base64url_encode_length_1(void);
void test_base64url_encode_length_2(void);
void test_base64url_encode_length_3(void);
void test_base64url_encode_large_data(void);
void test_base64url_encode_jwt_header(void);
void test_base64url_encode_null_termination(void);
void test_base64url_encode_output_length(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_base64url_encode_basic_string(void) {
    const char* input = "Hello";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SGVsbG8", result);  // No padding for URL-safe
    
    free(result);
}

void test_base64url_encode_short_string(void) {
    const char* input = "Hi";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SGk", result);  // No padding
    
    free(result);
}

void test_base64url_encode_single_character(void) {
    const char* input = "A";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QQ", result);  // No padding
    
    free(result);
}

void test_base64url_encode_exact_multiple_of_three(void) {
    const char* input = "ABC";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QUJD", result);
    
    free(result);
}

// Test URL-safe characters (- and _ instead of + and /)
void test_base64url_encode_url_safe_chars(void) {
    // Data that produces '+' and '/' in standard base64
    unsigned char data[] = {0xFF, 0xEF, 0xBF};
    char* result = utils_base64url_encode(data, sizeof(data));
    
    TEST_ASSERT_NOT_NULL(result);
    // Standard base64 would be "/++/", URL-safe should use - and _
    TEST_ASSERT_TRUE(strchr(result, '-') != NULL || strchr(result, '_') != NULL);
    // Should NOT contain + or /
    TEST_ASSERT_NULL(strchr(result, '+'));
    TEST_ASSERT_NULL(strchr(result, '/'));
    
    free(result);
}

void test_base64url_encode_no_padding(void) {
    const char* input = "test";  // 4 chars = 1 padding char in standard base64
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    // Should NOT contain '=' padding
    TEST_ASSERT_NULL(strchr(result, '='));
    
    free(result);
}

void test_base64url_encode_longer_string(void) {
    const char* input = "Hello, World!";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    // No padding
    TEST_ASSERT_NULL(strchr(result, '='));
    
    free(result);
}

void test_base64url_encode_binary_data(void) {
    unsigned char binary[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    char* result = utils_base64url_encode(binary, sizeof(binary));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    TEST_ASSERT_NULL(strchr(result, '='));  // No padding
    
    free(result);
}

void test_base64url_encode_all_zeros(void) {
    unsigned char zeros[] = {0x00, 0x00, 0x00};
    char* result = utils_base64url_encode(zeros, sizeof(zeros));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("AAAA", result);
    
    free(result);
}

// Test null parameter handling
void test_base64url_encode_null_data(void) {
    char* result = utils_base64url_encode(NULL, 10);
    TEST_ASSERT_NULL(result);
}

void test_base64url_encode_zero_length(void) {
    const char* input = "test";
    char* result = utils_base64url_encode((const unsigned char*)input, 0);
    TEST_ASSERT_NULL(result);
}

// Test edge cases for length
void test_base64url_encode_length_1(void) {
    const unsigned char data[] = {0x41};  // 'A'
    char* result = utils_base64url_encode(data, 1);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QQ", result);  // No padding
    
    free(result);
}

void test_base64url_encode_length_2(void) {
    const unsigned char data[] = {0x41, 0x42};  // 'AB'
    char* result = utils_base64url_encode(data, 2);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QUI", result);  // No padding
    
    free(result);
}

void test_base64url_encode_length_3(void) {
    const unsigned char data[] = {0x41, 0x42, 0x43};  // 'ABC'
    char* result = utils_base64url_encode(data, 3);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("QUJD", result);  // No padding needed
    
    free(result);
}

// Test large data
void test_base64url_encode_large_data(void) {
    size_t size = 1024;
    unsigned char* large_data = malloc(size);
    TEST_ASSERT_NOT_NULL(large_data);
    
    // Fill with pattern
    for (size_t i = 0; i < size; i++) {
        large_data[i] = (unsigned char)(i % 256);
    }
    
    char* result = utils_base64url_encode(large_data, size);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    TEST_ASSERT_NULL(strchr(result, '='));  // No padding
    
    free(large_data);
    free(result);
}

// Test JWT-like data (typical use case for base64url)
void test_base64url_encode_jwt_header(void) {
    const char* jwt_header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char* result = utils_base64url_encode((const unsigned char*)jwt_header, strlen(jwt_header));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    // Should be URL-safe (no +, /, or =)
    TEST_ASSERT_NULL(strchr(result, '+'));
    TEST_ASSERT_NULL(strchr(result, '/'));
    TEST_ASSERT_NULL(strchr(result, '='));
    
    free(result);
}

// Test null termination
void test_base64url_encode_null_termination(void) {
    const char* input = "Test";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    size_t len = strlen(result);
    TEST_ASSERT_EQUAL(0, result[len]); // Verify null terminator
    
    free(result);
}

// Test output length calculation
void test_base64url_encode_output_length(void) {
    // Length 1 -> 2 chars
    const unsigned char data1[] = {0x41};
    char* result1 = utils_base64url_encode(data1, 1);
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_EQUAL(2, strlen(result1));
    free(result1);
    
    // Length 2 -> 3 chars
    const unsigned char data2[] = {0x41, 0x42};
    char* result2 = utils_base64url_encode(data2, 2);
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL(3, strlen(result2));
    free(result2);
    
    // Length 3 -> 4 chars
    const unsigned char data3[] = {0x41, 0x42, 0x43};
    char* result3 = utils_base64url_encode(data3, 3);
    TEST_ASSERT_NOT_NULL(result3);
    TEST_ASSERT_EQUAL(4, strlen(result3));
    free(result3);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_base64url_encode_basic_string);
    RUN_TEST(test_base64url_encode_short_string);
    RUN_TEST(test_base64url_encode_single_character);
    RUN_TEST(test_base64url_encode_exact_multiple_of_three);
    
    // URL-safe character tests
    RUN_TEST(test_base64url_encode_url_safe_chars);
    RUN_TEST(test_base64url_encode_no_padding);
    
    // String and binary tests
    RUN_TEST(test_base64url_encode_longer_string);
    RUN_TEST(test_base64url_encode_binary_data);
    RUN_TEST(test_base64url_encode_all_zeros);
    
    // Parameter validation tests
    RUN_TEST(test_base64url_encode_null_data);
    RUN_TEST(test_base64url_encode_zero_length);
    
    // Edge case length tests
    RUN_TEST(test_base64url_encode_length_1);
    RUN_TEST(test_base64url_encode_length_2);
    RUN_TEST(test_base64url_encode_length_3);
    
    // Large data test
    RUN_TEST(test_base64url_encode_large_data);
    
    // JWT use case
    RUN_TEST(test_base64url_encode_jwt_header);
    
    // Null termination test
    RUN_TEST(test_base64url_encode_null_termination);
    
    // Output length test
    RUN_TEST(test_base64url_encode_output_length);
    
    return UNITY_END();
}
