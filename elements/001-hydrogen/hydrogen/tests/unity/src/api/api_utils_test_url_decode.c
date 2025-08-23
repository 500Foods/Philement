/*
 * Unity Test File: API Utils api_url_decode Function Tests
 * This file contains unit tests for the api_url_decode function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/api/api_utils.h"

void setUp(void) {
    // No setup needed for this pure function
}

void tearDown(void) {
    // No teardown needed for this pure function
}

// Test basic URL decoding
void test_api_url_decode_basic_string(void);
void test_api_url_decode_plus_to_space(void);
void test_api_url_decode_percent_encoding(void);
void test_api_url_decode_mixed_encoding(void);
void test_api_url_decode_special_characters(void);
void test_api_url_decode_hex_uppercase(void);
void test_api_url_decode_hex_lowercase(void);
void test_api_url_decode_invalid_percent_incomplete(void);
void test_api_url_decode_invalid_percent_nonhex(void);
void test_api_url_decode_empty_string(void);
void test_api_url_decode_null_input(void);
void test_api_url_decode_only_percent_signs(void);
void test_api_url_decode_percent_at_end(void);
void test_api_url_decode_complex_url_component(void);
void test_api_url_decode_form_data(void);
void test_api_url_decode_single_character(void);
void test_api_url_decode_single_plus(void);
void test_api_url_decode_consecutive_encodings(void);
void test_api_url_decode_mixed_valid_invalid(void);
void test_api_url_decode_basic_string(void) {
    char *result = api_url_decode("hello");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
    
    free(result);
}

// Test plus sign to space conversion
void test_api_url_decode_plus_to_space(void) {
    char *result = api_url_decode("hello+world");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    
    free(result);
}

// Test percent encoding decoding
void test_api_url_decode_percent_encoding(void) {
    char *result = api_url_decode("hello%20world");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    
    free(result);
}

// Test mixed plus and percent encoding
void test_api_url_decode_mixed_encoding(void) {
    char *result = api_url_decode("hello+world%20test");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world test", result);
    
    free(result);
}

// Test special characters
void test_api_url_decode_special_characters(void) {
    char *result = api_url_decode("user%40example.com");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("user@example.com", result);
    
    free(result);
}

// Test hex digit decoding (uppercase)
void test_api_url_decode_hex_uppercase(void) {
    char *result = api_url_decode("test%2F%3D%26");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test/=&", result);
    
    free(result);
}

// Test hex digit decoding (lowercase)
void test_api_url_decode_hex_lowercase(void) {
    char *result = api_url_decode("test%2f%3d%26");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test/=&", result);
    
    free(result);
}

// Test invalid percent encoding (incomplete)
void test_api_url_decode_invalid_percent_incomplete(void) {
    char *result = api_url_decode("test%2");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test%2", result);  // Should preserve invalid encoding
    
    free(result);
}

// Test invalid percent encoding (non-hex)
void test_api_url_decode_invalid_percent_nonhex(void) {
    char *result = api_url_decode("test%GG");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test%GG", result);  // Should preserve invalid encoding
    
    free(result);
}

// Test empty string
void test_api_url_decode_empty_string(void) {
    char *result = api_url_decode("");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    
    free(result);
}

// Test null input
void test_api_url_decode_null_input(void) {
    char *result = api_url_decode(NULL);
    
    TEST_ASSERT_NULL(result);
}

// Test string with only percent signs
void test_api_url_decode_only_percent_signs(void) {
    char *result = api_url_decode("%%%");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("%%%", result);  // Should preserve invalid encoding
    
    free(result);
}

// Test percent at end of string
void test_api_url_decode_percent_at_end(void) {
    char *result = api_url_decode("test%");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test%", result);  // Should preserve incomplete encoding
    
    free(result);
}

// Test complex real-world URL component
void test_api_url_decode_complex_url_component(void) {
    char *result = api_url_decode("name%3DJohn%26age%3D30%2Bcity%3DNew%2BYork");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("name=John&age=30+city=New+York", result);
    
    free(result);
}

// Test form data with special characters
void test_api_url_decode_form_data(void) {
    char *result = api_url_decode("search%3Dhello+world%21%40%23");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("search=hello world!@#", result);
    
    free(result);
}

// Test boundary conditions - single character
void test_api_url_decode_single_character(void) {
    char *result = api_url_decode("a");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("a", result);
    
    free(result);
}

// Test boundary conditions - single plus
void test_api_url_decode_single_plus(void) {
    char *result = api_url_decode("+");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(" ", result);
    
    free(result);
}

// Test consecutive percent encodings
void test_api_url_decode_consecutive_encodings(void) {
    char *result = api_url_decode("%20%21%22");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(" !\"", result);
    
    free(result);
}

// Test mixed valid and invalid encodings
void test_api_url_decode_mixed_valid_invalid(void) {
    char *result = api_url_decode("good%20bad%ZZ%21");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("good bad%ZZ!", result);  // Valid ones decoded, invalid preserved
    
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_url_decode_basic_string);
    RUN_TEST(test_api_url_decode_plus_to_space);
    RUN_TEST(test_api_url_decode_percent_encoding);
    RUN_TEST(test_api_url_decode_mixed_encoding);
    RUN_TEST(test_api_url_decode_special_characters);
    RUN_TEST(test_api_url_decode_hex_uppercase);
    RUN_TEST(test_api_url_decode_hex_lowercase);
    RUN_TEST(test_api_url_decode_invalid_percent_incomplete);
    RUN_TEST(test_api_url_decode_invalid_percent_nonhex);
    RUN_TEST(test_api_url_decode_empty_string);
    RUN_TEST(test_api_url_decode_null_input);
    RUN_TEST(test_api_url_decode_only_percent_signs);
    RUN_TEST(test_api_url_decode_percent_at_end);
    RUN_TEST(test_api_url_decode_complex_url_component);
    RUN_TEST(test_api_url_decode_form_data);
    RUN_TEST(test_api_url_decode_single_character);
    RUN_TEST(test_api_url_decode_single_plus);
    RUN_TEST(test_api_url_decode_consecutive_encodings);
    RUN_TEST(test_api_url_decode_mixed_valid_invalid);
    
    return UNITY_END();
}
