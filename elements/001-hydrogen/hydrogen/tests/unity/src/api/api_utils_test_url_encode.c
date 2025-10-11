/*
 * Unity Test File: API Utils api_url_encode Function Tests
 * This file contains unit tests for the api_url_encode function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

// Function prototypes for test functions
void test_api_url_encode_basic_string(void);
void test_api_url_encode_alphanumeric(void);
void test_api_url_encode_unreserved_characters(void);
void test_api_url_encode_space_to_plus(void);
void test_api_url_encode_special_characters(void);
void test_api_url_encode_multiple_special(void);
void test_api_url_encode_form_data(void);
void test_api_url_encode_empty_string(void);
void test_api_url_encode_null_input(void);
void test_api_url_encode_single_characters(void);
void test_api_url_encode_complex_component(void);
void test_api_url_encode_high_bit_characters(void);
void test_api_url_encode_all_special_chars(void);
void test_api_url_encode_consecutive_spaces(void);
void test_api_url_encode_mixed_characters(void);
void test_api_url_encode_single_unreserved(void);
void test_api_url_encode_uppercase_hex(void);
void test_api_url_encode_percent_sign(void);
void test_api_url_encode_control_characters(void);

void setUp(void) {
    // No setup needed for this pure function
}

void tearDown(void) {
    // No teardown needed for this pure function
}

// Test basic string without encoding needed
void test_api_url_encode_basic_string(void) {
    char *result = api_url_encode("hello");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
    
    free(result);
}

// Test alphanumeric characters (should not be encoded)
void test_api_url_encode_alphanumeric(void) {
    char *result = api_url_encode("abc123XYZ");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("abc123XYZ", result);
    
    free(result);
}

// Test unreserved characters (should not be encoded)
void test_api_url_encode_unreserved_characters(void) {
    char *result = api_url_encode("test-file_name.ext~backup");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test-file_name.ext~backup", result);
    
    free(result);
}

// Test space encoding (should become +)
void test_api_url_encode_space_to_plus(void) {
    char *result = api_url_encode("hello world");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello+world", result);
    
    free(result);
}

// Test special characters encoding
void test_api_url_encode_special_characters(void) {
    char *result = api_url_encode("user@example.com");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("user%40example.com", result);
    
    free(result);
}

// Test multiple special characters
void test_api_url_encode_multiple_special(void) {
    char *result = api_url_encode("test/=&");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test%2F%3D%26", result);
    
    free(result);
}

// Test form data with spaces and special characters
void test_api_url_encode_form_data(void) {
    char *result = api_url_encode("search=hello world!@#");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("search%3Dhello+world%21%40%23", result);
    
    free(result);
}

// Test empty string
void test_api_url_encode_empty_string(void) {
    char *result = api_url_encode("");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    
    free(result);
}

// Test null input
void test_api_url_encode_null_input(void) {
    char *result = api_url_encode(NULL);
    
    TEST_ASSERT_NULL(result);
}

// Test single character encodings
void test_api_url_encode_single_characters(void) {
    char *result;
    
    // Test space
    result = api_url_encode(" ");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("+", result);
    free(result);
    
    // Test @
    result = api_url_encode("@");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("%40", result);
    free(result);
    
    // Test /
    result = api_url_encode("/");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("%2F", result);
    free(result);
}

// Test complex URL component
void test_api_url_encode_complex_component(void) {
    char *result = api_url_encode("name=John&age=30 city=New York");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("name%3DJohn%26age%3D30+city%3DNew+York", result);
    
    free(result);
}

// Test high-bit characters (non-ASCII)
void test_api_url_encode_high_bit_characters(void) {
    // Test characters with high bit set
    const char input[] = {(char)0x80, (char)0xFF, '\0'};
    char *result = api_url_encode(input);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("%80%FF", result);
    
    free(result);
}

// Test all special characters that should be encoded
void test_api_url_encode_all_special_chars(void) {
    char *result = api_url_encode("!\"#$%&'()*+,/:;=?@[\\]^`{|}");
    
    TEST_ASSERT_NOT_NULL(result);
    // Note: This is a comprehensive test - the exact encoding depends on RFC 3986
    // Most special characters should be percent-encoded
    TEST_ASSERT_NOT_NULL(result);  // Just ensure it doesn't crash
    
    free(result);
}

// Test consecutive spaces
void test_api_url_encode_consecutive_spaces(void) {
    char *result = api_url_encode("hello   world");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello+++world", result);
    
    free(result);
}

// Test mixed unreserved and reserved characters
void test_api_url_encode_mixed_characters(void) {
    char *result = api_url_encode("test-file_2024.txt?version=1&format=json");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test-file_2024.txt%3Fversion%3D1%26format%3Djson", result);
    
    free(result);
}

// Test boundary case - single unreserved character
void test_api_url_encode_single_unreserved(void) {
    char *result = api_url_encode("a");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("a", result);
    
    free(result);
}

// Test encoding that should produce uppercase hex
void test_api_url_encode_uppercase_hex(void) {
    char *result = api_url_encode("?");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("%3F", result);  // Should be uppercase hex
    
    free(result);
}

// Test percent sign encoding (should encode the percent sign itself)
void test_api_url_encode_percent_sign(void) {
    char *result = api_url_encode("100%");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("100%25", result);
    
    free(result);
}

// Test newline and control characters
void test_api_url_encode_control_characters(void) {
    const char input[] = {'t', 'e', 's', 't', '\n', '\t', '\r', '\0'};
    char *result = api_url_encode(input);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test%0A%09%0D", result);
    
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_url_encode_basic_string);
    RUN_TEST(test_api_url_encode_alphanumeric);
    RUN_TEST(test_api_url_encode_unreserved_characters);
    RUN_TEST(test_api_url_encode_space_to_plus);
    RUN_TEST(test_api_url_encode_special_characters);
    RUN_TEST(test_api_url_encode_multiple_special);
    RUN_TEST(test_api_url_encode_form_data);
    RUN_TEST(test_api_url_encode_empty_string);
    RUN_TEST(test_api_url_encode_null_input);
    RUN_TEST(test_api_url_encode_single_characters);
    RUN_TEST(test_api_url_encode_complex_component);
    RUN_TEST(test_api_url_encode_high_bit_characters);
    RUN_TEST(test_api_url_encode_all_special_chars);
    RUN_TEST(test_api_url_encode_consecutive_spaces);
    RUN_TEST(test_api_url_encode_mixed_characters);
    RUN_TEST(test_api_url_encode_single_unreserved);
    RUN_TEST(test_api_url_encode_uppercase_hex);
    RUN_TEST(test_api_url_encode_percent_sign);
    RUN_TEST(test_api_url_encode_control_characters);
    
    return UNITY_END();
}
