/*
 * Unity Test File: utils_base64url_decode()
 * This file contains unit tests for URL-safe Base64 decoding
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
unsigned char* utils_base64url_decode(const char* input, size_t* output_length);


// Function prototypes for test functions
void test_base64url_decode_basic_string(void);
void test_base64url_decode_short_string(void);
void test_base64url_decode_single_character(void);
void test_base64url_decode_exact_multiple_of_four(void);
void test_base64url_decode_binary_data(void);
void test_base64url_decode_all_zeros(void);
void test_base64url_decode_null_input(void);
void test_base64url_decode_null_output_length(void);
void test_base64url_decode_empty_string(void);
void test_base64url_decode_invalid_length(void);
void test_base64url_decode_url_safe_chars(void);
void test_base64url_decode_roundtrip_basic(void);
void test_base64url_decode_roundtrip_binary(void);
void test_base64url_decode_length_2(void);
void test_base64url_decode_length_3(void);
void test_base64url_decode_length_4(void);
void test_base64url_decode_large_data(void);
void test_base64url_decode_jwt_payload(void);
void test_base64url_decode_basic_string(void);
void test_base64url_decode_short_string(void);
void test_base64url_decode_single_character(void);
void test_base64url_decode_exact_multiple_of_four(void);
void test_base64url_decode_binary_data(void);
void test_base64url_decode_all_zeros(void);
void test_base64url_decode_null_input(void);
void test_base64url_decode_null_output_length(void);
void test_base64url_decode_empty_string(void);
void test_base64url_decode_invalid_length(void);
void test_base64url_decode_url_safe_chars(void);
void test_base64url_decode_roundtrip_basic(void);
void test_base64url_decode_roundtrip_binary(void);
void test_base64url_decode_length_2(void);
void test_base64url_decode_length_3(void);
void test_base64url_decode_length_4(void);
void test_base64url_decode_large_data(void);
void test_base64url_decode_jwt_payload(void);

void setUp(void) {
    // No setup needed
}


// Function prototypes for test functions
void test_base64url_decode_basic_string(void);
void test_base64url_decode_short_string(void);
void test_base64url_decode_single_character(void);
void test_base64url_decode_exact_multiple_of_four(void);
void test_base64url_decode_binary_data(void);
void test_base64url_decode_all_zeros(void);
void test_base64url_decode_null_input(void);
void test_base64url_decode_null_output_length(void);
void test_base64url_decode_empty_string(void);
void test_base64url_decode_invalid_length(void);
void test_base64url_decode_url_safe_chars(void);
void test_base64url_decode_roundtrip_basic(void);
void test_base64url_decode_roundtrip_binary(void);
void test_base64url_decode_length_2(void);
void test_base64url_decode_length_3(void);
void test_base64url_decode_length_4(void);
void test_base64url_decode_large_data(void);
void test_base64url_decode_jwt_payload(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_base64url_decode_basic_string(void) {
    const char* encoded = "SGVsbG8";  // "Hello" in base64url
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(5, output_length);
    TEST_ASSERT_EQUAL_STRING("Hello", (char*)result);
    
    free(result);
}

void test_base64url_decode_short_string(void) {
    const char* encoded = "SGk";  // "Hi" in base64url
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(2, output_length);
    TEST_ASSERT_EQUAL_MEMORY("Hi", result, output_length);
    
    free(result);
}

void test_base64url_decode_single_character(void) {
    const char* encoded = "QQ";  // "A" in base64url
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, output_length);
    TEST_ASSERT_EQUAL('A', result[0]);
    
    free(result);
}

void test_base64url_decode_exact_multiple_of_four(void) {
    const char* encoded = "QUJD";  // "ABC" in base64url
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(3, output_length);
    TEST_ASSERT_EQUAL_MEMORY("ABC", result, output_length);
    
    free(result);
}

void test_base64url_decode_binary_data(void) {
    const char* encoded = "AAECAwQF";  // Binary: 0x00, 0x01, 0x02, 0x03, 0x04, 0x05
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(6, output_length);
    
    unsigned char expected[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    TEST_ASSERT_EQUAL_MEMORY(expected, result, output_length);
    
    free(result);
}

void test_base64url_decode_all_zeros(void) {
    const char* encoded = "AAAA";  // Three zero bytes
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(3, output_length);
    
    unsigned char expected[] = {0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, result, output_length);
    
    free(result);
}

// Test null parameter handling
void test_base64url_decode_null_input(void) {
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(NULL, &output_length);
    TEST_ASSERT_NULL(result);
}

void test_base64url_decode_null_output_length(void) {
    const char* encoded = "SGVsbG8";
    unsigned char* result = utils_base64url_decode(encoded, NULL);
    TEST_ASSERT_NULL(result);
}

void test_base64url_decode_empty_string(void) {
    const char* encoded = "";
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    // Empty string has length 0, which is valid multiple of 4
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, output_length);
    
    free(result);
}

// Test invalid inputs
void test_base64url_decode_invalid_length(void) {
    // Length 5 is not valid (must be multiple of 4, or 2 or 3 for padding omission)
    const char* encoded = "ABCDE";
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    // Should fail due to invalid length
    TEST_ASSERT_NULL(result);
}

// Test decoding with URL-safe characters
void test_base64url_decode_url_safe_chars(void) {
    // Test string containing URL-safe chars (- and _)
    const char* encoded = "AB-_";  // Contains URL-safe characters
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, output_length);
    
    free(result);
}

// Test round-trip encoding/decoding
void test_base64url_decode_roundtrip_basic(void) {
    const char* original = "Hello, World!";
    
    // Encode
    char* encoded = utils_base64url_encode((const unsigned char*)original, strlen(original));
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t output_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &output_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify
    TEST_ASSERT_EQUAL(strlen(original), output_length);
    TEST_ASSERT_EQUAL_MEMORY(original, decoded, output_length);
    
    free(encoded);
    free(decoded);
}

void test_base64url_decode_roundtrip_binary(void) {
    unsigned char original[] = {0x00, 0xFF, 0x80, 0x7F, 0x01, 0xFE};
    size_t original_length = sizeof(original);
    
    // Encode
    char* encoded = utils_base64url_encode(original, original_length);
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t output_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &output_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify
    TEST_ASSERT_EQUAL(original_length, output_length);
    TEST_ASSERT_EQUAL_MEMORY(original, decoded, output_length);
    
    free(encoded);
    free(decoded);
}

// Test length variations
void test_base64url_decode_length_2(void) {
    const char* encoded = "QQ";  // 1 byte decoded
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, output_length);
    
    free(result);
}

void test_base64url_decode_length_3(void) {
    const char* encoded = "QUI";  // 2 bytes decoded
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(2, output_length);
    
    free(result);
}

void test_base64url_decode_length_4(void) {
    const char* encoded = "QUJD";  // 3 bytes decoded
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(encoded, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(3, output_length);
    
    free(result);
}

// Test large data
void test_base64url_decode_large_data(void) {
    size_t original_size = 1024;
    unsigned char* original = malloc(original_size);
    TEST_ASSERT_NOT_NULL(original);
    
    // Fill with pattern
    for (size_t i = 0; i < original_size; i++) {
        original[i] = (unsigned char)(i % 256);
    }
    
    // Encode
    char* encoded = utils_base64url_encode(original, original_size);
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t output_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &output_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify
    TEST_ASSERT_EQUAL(original_size, output_length);
    TEST_ASSERT_EQUAL_MEMORY(original, decoded, output_length);
    
    free(original);
    free(encoded);
    free(decoded);
}

// Test JWT-like payloads
void test_base64url_decode_jwt_payload(void) {
    const char* jwt_payload = "{\"sub\":\"1234567890\",\"name\":\"John Doe\"}";
    
    // Encode
    char* encoded = utils_base64url_encode((const unsigned char*)jwt_payload, strlen(jwt_payload));
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t output_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &output_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify
    TEST_ASSERT_EQUAL(strlen(jwt_payload), output_length);
    TEST_ASSERT_EQUAL_MEMORY(jwt_payload, decoded, output_length);
    
    free(encoded);
    free(decoded);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_base64url_decode_basic_string);
    RUN_TEST(test_base64url_decode_short_string);
    RUN_TEST(test_base64url_decode_single_character);
    RUN_TEST(test_base64url_decode_exact_multiple_of_four);
    
    // Binary data tests
    RUN_TEST(test_base64url_decode_binary_data);
    RUN_TEST(test_base64url_decode_all_zeros);
    
    // Parameter validation tests
    RUN_TEST(test_base64url_decode_null_input);
    RUN_TEST(test_base64url_decode_null_output_length);
    RUN_TEST(test_base64url_decode_empty_string);
    
    // Invalid input tests
    RUN_TEST(test_base64url_decode_invalid_length);
    
    // URL-safe character tests
    RUN_TEST(test_base64url_decode_url_safe_chars);
    
    // Round-trip tests
    RUN_TEST(test_base64url_decode_roundtrip_basic);
    RUN_TEST(test_base64url_decode_roundtrip_binary);
    
    // Length variation tests
    RUN_TEST(test_base64url_decode_length_2);
    RUN_TEST(test_base64url_decode_length_3);
    RUN_TEST(test_base64url_decode_length_4);
    
    // Large data test
    RUN_TEST(test_base64url_decode_large_data);
    
    // JWT payload test
    RUN_TEST(test_base64url_decode_jwt_payload);
    
    return UNITY_END();
}
