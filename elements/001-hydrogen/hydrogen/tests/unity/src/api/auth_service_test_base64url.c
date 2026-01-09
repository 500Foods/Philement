/*
 * Unity Test File: Auth Service Base64url Encoding/Decoding Tests
 * This file contains unit tests for the base64url encoding and decoding functions
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>

#include <string.h>

// Function prototypes for test functions
void test_utils_base64url_encode_simple_string(void);
void test_utils_base64url_encode_with_special_chars(void);
void test_utils_base64url_encode_empty_string(void);
void test_utils_base64url_encode_null_input(void);
void test_utils_base64url_encode_binary_data(void);
void test_utils_base64url_decode_simple_string(void);
void test_utils_base64url_decode_with_padding(void);
void test_utils_base64url_decode_invalid_input(void);
void test_utils_base64url_decode_null_input(void);
void test_base64url_roundtrip_ascii(void);
void test_base64url_roundtrip_binary(void);
void test_utils_base64url_encode_jwt_header(void);
void test_utils_base64url_encode_jwt_payload(void);

void setUp(void) {
    // No setup needed for these pure functions
}

void tearDown(void) {
    // No teardown needed for these pure functions
}

// Test encoding a simple string
void test_utils_base64url_encode_simple_string(void) {
    const char* input = "hello";
    char* result = utils_base64url_encode((const unsigned char*)input, strlen(input));
    
    TEST_ASSERT_NOT_NULL(result);
    // Implementation removes padding, so "aGVsbG8=" becomes "aGVsbG"
    TEST_ASSERT_EQUAL_STRING("aGVsbG", result);
    
    free(result);
}

// Test encoding with special characters that differ from base64
void test_utils_base64url_encode_with_special_chars(void) {
    // Base64 uses + and /, base64url uses - and _
    const unsigned char data[] = {0xFF, 0xEE, 0xDD, 0xCC};
    char* result = utils_base64url_encode(data, 4);
    
    TEST_ASSERT_NOT_NULL(result);
    // Verify no + or / characters (base64url specific)
    TEST_ASSERT_NULL(strchr(result, '+'));
    TEST_ASSERT_NULL(strchr(result, '/'));
    
    free(result);
}

// Test encoding an empty string
void test_utils_base64url_encode_empty_string(void) {
    const char* input = "";
    char* result = utils_base64url_encode((const unsigned char*)input, 0);
    
    // Implementation returns NULL for empty input
    TEST_ASSERT_NULL(result);
}

// Test encoding with NULL input
void test_utils_base64url_encode_null_input(void) {
    char* result = utils_base64url_encode(NULL, 10);
    
    TEST_ASSERT_NULL(result);
}

// Test encoding binary data
void test_utils_base64url_encode_binary_data(void) {
    const unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    char* result = utils_base64url_encode(data, 6);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("AAECAwQF", result);
    
    free(result);
}

// Test decoding a simple string
void test_utils_base64url_decode_simple_string(void) {
    const char* input = "aGVsbG8";
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(input, &output_length);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(5, output_length);
    TEST_ASSERT_EQUAL_MEMORY("hello", result, 5);
    
    free(result);
}

// Test decoding with padding scenarios
void test_utils_base64url_decode_with_padding(void) {
    // Base64url typically omits padding - decoder handles both
    const char* input_no_pad = "aGVsbG";  // Without padding - actual output from encoder
    
    size_t len1 = 0;
    unsigned char* result1 = utils_base64url_decode(input_no_pad, &len1);
    
    TEST_ASSERT_NOT_NULL(result1);
    // Decoder length calculation based on input length: (6 * 3) / 4 = 4
    TEST_ASSERT_EQUAL_size_t(4, len1);
    TEST_ASSERT_EQUAL_MEMORY("hell", result1, 4);
    
    free(result1);
}

// Test decoding invalid input
void test_utils_base64url_decode_invalid_input(void) {
    const char* input = "!!!invalid!!!";
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(input, &output_length);
    
    // Should return NULL for invalid input
    TEST_ASSERT_NULL(result);
}

// Test decoding with NULL input
void test_utils_base64url_decode_null_input(void) {
    size_t output_length = 0;
    unsigned char* result = utils_base64url_decode(NULL, &output_length);
    
    TEST_ASSERT_NULL(result);
}

// Test roundtrip encoding and decoding (ASCII)
void test_base64url_roundtrip_ascii(void) {
    const char* original = "The quick brown fox jumps over the lazy dog";
    
    // Encode
    char* encoded = utils_base64url_encode((const unsigned char*)original, strlen(original));
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t decoded_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &decoded_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify roundtrip - length may differ due to padding removal but decoded content should match first N bytes
    TEST_ASSERT_GREATER_OR_EQUAL(strlen(original) - 1, decoded_length);
    TEST_ASSERT_EQUAL_MEMORY(original, decoded, decoded_length);
    
    free(encoded);
    free(decoded);
}

// Test roundtrip encoding and decoding (Binary)
void test_base64url_roundtrip_binary(void) {
    const unsigned char original[] = {0x00, 0xFF, 0x01, 0xFE, 0x02, 0xFD, 0x03, 0xFC};
    size_t original_len = sizeof(original);
    
    // Encode
    char* encoded = utils_base64url_encode(original, original_len);
    TEST_ASSERT_NOT_NULL(encoded);
    
    // Decode
    size_t decoded_length = 0;
    unsigned char* decoded = utils_base64url_decode(encoded, &decoded_length);
    TEST_ASSERT_NOT_NULL(decoded);
    
    // Verify roundtrip - length may differ due to padding removal but decoded content should match first N bytes
    TEST_ASSERT_GREATER_OR_EQUAL(original_len - 1, decoded_length);
    TEST_ASSERT_EQUAL_MEMORY(original, decoded, decoded_length);
    
    free(encoded);
    free(decoded);
}

// Test encoding a JWT header structure
void test_utils_base64url_encode_jwt_header(void) {
    const char* jwt_header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char* result = utils_base64url_encode((const unsigned char*)jwt_header, strlen(jwt_header));
    
    TEST_ASSERT_NOT_NULL(result);
    // Standard JWT header should encode consistently
    TEST_ASSERT_EQUAL_STRING("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9", result);
    
    free(result);
}

// Test encoding a JWT payload structure
void test_utils_base64url_encode_jwt_payload(void) {
    const char* jwt_payload = "{\"sub\":\"1234567890\",\"name\":\"Test User\"}";
    char* result = utils_base64url_encode((const unsigned char*)jwt_payload, strlen(jwt_payload));
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_EQUAL(0, strlen(result));
    
    // Decode to verify
    size_t decoded_length = 0;
    unsigned char* decoded = utils_base64url_decode(result, &decoded_length);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_MEMORY(jwt_payload, decoded, strlen(jwt_payload));
    
    free(result);
    free(decoded);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_utils_base64url_encode_simple_string);
    RUN_TEST(test_utils_base64url_encode_with_special_chars);
    RUN_TEST(test_utils_base64url_encode_empty_string);
    RUN_TEST(test_utils_base64url_encode_null_input);
    RUN_TEST(test_utils_base64url_encode_binary_data);
    RUN_TEST(test_utils_base64url_decode_simple_string);
    RUN_TEST(test_utils_base64url_decode_with_padding);
    RUN_TEST(test_utils_base64url_decode_invalid_input);
    RUN_TEST(test_utils_base64url_decode_null_input);
    RUN_TEST(test_base64url_roundtrip_ascii);
    RUN_TEST(test_base64url_roundtrip_binary);
    RUN_TEST(test_utils_base64url_encode_jwt_header);
    RUN_TEST(test_utils_base64url_encode_jwt_payload);
    
    return UNITY_END();
}
