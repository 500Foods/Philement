/*
 * Unity Test File: Beryllium parse_parameter_string Function Tests
 * This file contains comprehensive unit tests for the parse_parameter_string() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test string parameter extraction from G-code commands
 * - Test various parameter formats and edge cases
 * - Test memory allocation and error handling
 * - Test different parameter types and values
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Forward declaration for the function being tested
char* parse_parameter_string(const char *line, const char *parameter);

void setUp(void) {
    // No setup needed for parameter parsing tests
}

void tearDown(void) {
    // No teardown needed for parameter parsing tests
}

// Function prototypes for test functions
void test_parse_parameter_string_basic_extraction(void);
void test_parse_parameter_string_parameter_not_found(void);
void test_parse_parameter_string_empty_parameter(void);
void test_parse_parameter_string_null_line(void);
void test_parse_parameter_string_null_parameter(void);
void test_parse_parameter_string_no_equals_sign(void);
void test_parse_parameter_string_empty_value(void);
void test_parse_parameter_string_whitespace_handling(void);
void test_parse_parameter_string_special_characters(void);
void test_parse_parameter_string_multiple_parameters(void);
void test_parse_parameter_string_long_values(void);
void test_parse_parameter_string_memory_allocation(void);
void test_parse_parameter_string_real_gcode_examples(void);

//=============================================================================
// Basic Parameter Extraction Tests
//=============================================================================

void test_parse_parameter_string_basic_extraction(void) {
    // Test basic parameter extraction
    char* result = parse_parameter_string("M117 Hello World", "M117");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Hello World", result);
    free(result);

    // Test with different parameter names
    result = parse_parameter_string("NAME=test_value", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_value", result);
    free(result);
}

void test_parse_parameter_string_parameter_not_found(void) {
    // Test when parameter is not found
    char* result = parse_parameter_string("G1 X10 Y20", "Z");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("undefined", result);
    free(result);
}

void test_parse_parameter_string_empty_parameter(void) {
    // Test with empty parameter name
    char* result = parse_parameter_string("G1 X10 Y20", "");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("undefined", result);
    free(result);
}

void test_parse_parameter_string_null_line(void) {
    // Test with NULL line
    char* result = parse_parameter_string(NULL, "X");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("undefined", result);
    free(result);
}

void test_parse_parameter_string_null_parameter(void) {
    // Test with NULL parameter
    char* result = parse_parameter_string("G1 X10 Y20", NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("undefined", result);
    free(result);
}

void test_parse_parameter_string_no_equals_sign(void) {
    // Test parameter without equals sign (like M117)
    char* result = parse_parameter_string("M117 Status message", "M117");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Status message", result);
    free(result);
}

void test_parse_parameter_string_empty_value(void) {
    // Test parameter with empty value
    char* result = parse_parameter_string("NAME=", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_parse_parameter_string_whitespace_handling(void) {
    // Test with various whitespace patterns
    char* result = parse_parameter_string("NAME=  spaced value  ", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("  spaced value  ", result);
    free(result);

    // Test with tabs
    result = parse_parameter_string("NAME=\tvalue\t", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\tvalue\t", result);
    free(result);
}

void test_parse_parameter_string_special_characters(void) {
    // Test with special characters in values
    char* result = parse_parameter_string("NAME=test_value_123", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_value_123", result);
    free(result);

    // Test with symbols
    result = parse_parameter_string("NAME=test.value-with@symbols#", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test.value-with@symbols#", result);
    free(result);
}

void test_parse_parameter_string_multiple_parameters(void) {
    // Test extracting different parameters from same line
    char* result1 = parse_parameter_string("X10 Y20 Z30", "X");
    char* result2 = parse_parameter_string("X10 Y20 Z30", "Y");
    char* result3 = parse_parameter_string("X10 Y20 Z30", "Z");

    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_NOT_NULL(result3);

    TEST_ASSERT_EQUAL_STRING("10", result1);
    TEST_ASSERT_EQUAL_STRING("20", result2);
    TEST_ASSERT_EQUAL_STRING("30", result3);

    free(result1);
    free(result2);
    free(result3);
}

void test_parse_parameter_string_long_values(void) {
    // Test with very long parameter values
    const char* long_value = "This is a very long parameter value that might be used for status messages or descriptions in G-code files";
    char line[256];
    snprintf(line, sizeof(line), "M117 %s", long_value);

    char* result = parse_parameter_string(line, "M117");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(long_value, result);
    free(result);
}

void test_parse_parameter_string_memory_allocation(void) {
    // Test that memory allocation works correctly
    char* result1 = parse_parameter_string("NAME=test", "NAME");
    char* result2 = parse_parameter_string("NAME=different", "NAME");

    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);

    // Results should be independent
    TEST_ASSERT_EQUAL_STRING("test", result1);
    TEST_ASSERT_EQUAL_STRING("different", result2);

    // Memory should be properly allocated (different addresses)
    TEST_ASSERT_TRUE(result1 != result2);

    free(result1);
    free(result2);
}

void test_parse_parameter_string_real_gcode_examples(void) {
    // Test with real G-code examples

    // M117 status messages
    char* result = parse_parameter_string("M117 Printing layer 1 of 100", "M117");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Printing layer 1 of 100", result);
    free(result);

    // Print stats info
    result = parse_parameter_string("SET_PRINT_STATS_INFO TOTAL_LAYER=100", "SET_PRINT_STATS_INFO");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("TOTAL_LAYER=100", result);
    free(result);

    // Object definitions
    result = parse_parameter_string("EXCLUDE_OBJECT_DEFINE NAME=cube_part", "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);
    free(result);

    // Filament type
    result = parse_parameter_string("filament_type=PLA", "filament_type");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("PLA", result);
    free(result);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic extraction tests
    RUN_TEST(test_parse_parameter_string_basic_extraction);
    RUN_TEST(test_parse_parameter_string_parameter_not_found);
    RUN_TEST(test_parse_parameter_string_empty_parameter);
    RUN_TEST(test_parse_parameter_string_null_line);
    RUN_TEST(test_parse_parameter_string_null_parameter);

    // Parameter format tests
    RUN_TEST(test_parse_parameter_string_no_equals_sign);
    RUN_TEST(test_parse_parameter_string_empty_value);
    RUN_TEST(test_parse_parameter_string_whitespace_handling);
    RUN_TEST(test_parse_parameter_string_special_characters);

    // Complex scenario tests
    RUN_TEST(test_parse_parameter_string_multiple_parameters);
    RUN_TEST(test_parse_parameter_string_long_values);
    RUN_TEST(test_parse_parameter_string_memory_allocation);

    // Real-world usage tests
    RUN_TEST(test_parse_parameter_string_real_gcode_examples);

    return UNITY_END();
}
