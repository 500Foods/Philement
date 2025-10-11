/*
 * Unity Test File: Beryllium parse_name_parameter Function Tests
 * This file contains comprehensive unit tests for the parse_name_parameter() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test NAME parameter extraction from G-code commands
 * - Test various NAME parameter formats and edge cases
 * - Test memory allocation and error handling
 * - Test real-world G-code examples with object names
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Forward declaration for the function being tested
char* parse_name_parameter(const char *line);

void setUp(void) {
    // No setup needed for name parameter parsing tests
}

void tearDown(void) {
    // No teardown needed for name parameter parsing tests
}

// Function prototypes for test functions
void test_parse_name_parameter_basic_extraction(void);
void test_parse_name_parameter_no_name_parameter(void);
void test_parse_name_parameter_null_line(void);
void test_parse_name_parameter_empty_line(void);
void test_parse_name_parameter_name_without_value(void);
void test_parse_name_parameter_whitespace_handling(void);
void test_parse_name_parameter_special_characters(void);
void test_parse_name_parameter_long_names(void);
void test_parse_name_parameter_multiple_spaces(void);
void test_parse_name_parameter_case_sensitivity(void);
void test_parse_name_parameter_real_gcode_examples(void);
void test_parse_name_parameter_mixed_parameters(void);

//=============================================================================
// Basic NAME Parameter Extraction Tests
//=============================================================================

void test_parse_name_parameter_basic_extraction(void) {
    // Test basic NAME parameter extraction
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=cube_part");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);
    free(result);

    // Test with different object names
    result = parse_name_parameter("EXCLUDE_OBJECT_START NAME=sphere");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("sphere", result);
    free(result);
}

void test_parse_name_parameter_no_name_parameter(void) {
    // Test when NAME parameter is not present
    char* result = parse_name_parameter("G1 X10 Y20 Z30");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);

    // Test with other parameters but no NAME
    result = parse_name_parameter("M117 Printing object");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_parse_name_parameter_null_line(void) {
    // Test with NULL line
    char* result = parse_name_parameter(NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_parse_name_parameter_empty_line(void) {
    // Test with empty line
    char* result = parse_name_parameter("");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_parse_name_parameter_name_without_value(void) {
    // Test NAME parameter without value
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_parse_name_parameter_whitespace_handling(void) {
    // Test with whitespace around NAME parameter
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME= cube_part ");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);  // Spaces are trimmed by current implementation
    free(result);

    // Test with tabs
    result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=\tcube_part\t");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);  // Tabs are trimmed by current implementation
    free(result);
}

void test_parse_name_parameter_special_characters(void) {
    // Test with special characters in object names
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=cube_part_123");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part_123", result);
    free(result);

    // Test with complex object names
    result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=object-with@special#chars");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("object-with@special#chars", result);
    free(result);
}

void test_parse_name_parameter_long_names(void) {
    // Test with very long object names
    const char* long_name = "very_long_object_name_that_might_be_used_in_complex_3d_models_with_detailed_descriptions";
    char line[256];
    snprintf(line, sizeof(line), "EXCLUDE_OBJECT_DEFINE NAME=%s", long_name);

    char* result = parse_name_parameter(line);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(long_name, result);
    free(result);
}

void test_parse_name_parameter_multiple_spaces(void) {
    // Test with multiple spaces in the line
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE  NAME  =  cube_part  ");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);  // Leading/trailing spaces are trimmed
    free(result);
}

void test_parse_name_parameter_case_sensitivity(void) {
    // Test case sensitivity (should be case sensitive)
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE name=cube_part");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result); // Should not find "name" (lowercase)
    free(result);

    // Test with correct case
    result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=cube_part");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);
    free(result);
}

void test_parse_name_parameter_real_gcode_examples(void) {
    // Test with real G-code examples from 3D printing

    // Object definition
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=cube_body");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_body", result);
    free(result);

    // Object start
    result = parse_name_parameter("EXCLUDE_OBJECT_START NAME=sphere_lid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("sphere_lid", result);
    free(result);

    // Object end (should not have NAME parameter)
    result = parse_name_parameter("EXCLUDE_OBJECT_END");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);

    // Complex object names
    result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE NAME=support_structure_bottom");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("support_structure_bottom", result);
    free(result);
}

void test_parse_name_parameter_mixed_parameters(void) {
    // Test with NAME parameter mixed with other parameters
    char* result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE CENTER=10,20 NAME=cube_part COLOR=red");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);
    free(result);

    // Test NAME parameter at the end
    result = parse_name_parameter("EXCLUDE_OBJECT_DEFINE CENTER=10,20 COLOR=red NAME=cube_part");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cube_part", result);
    free(result);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic extraction tests
    RUN_TEST(test_parse_name_parameter_basic_extraction);
    RUN_TEST(test_parse_name_parameter_no_name_parameter);
    RUN_TEST(test_parse_name_parameter_null_line);
    RUN_TEST(test_parse_name_parameter_empty_line);

    // Parameter format tests
    RUN_TEST(test_parse_name_parameter_name_without_value);
    RUN_TEST(test_parse_name_parameter_whitespace_handling);
    RUN_TEST(test_parse_name_parameter_special_characters);
    RUN_TEST(test_parse_name_parameter_long_names);
    RUN_TEST(test_parse_name_parameter_multiple_spaces);
    RUN_TEST(test_parse_name_parameter_case_sensitivity);

    // Real-world usage tests
    RUN_TEST(test_parse_name_parameter_real_gcode_examples);
    RUN_TEST(test_parse_name_parameter_mixed_parameters);

    return UNITY_END();
}
