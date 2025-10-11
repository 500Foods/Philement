/*
 * Unity Test File: Beryllium parse_parameter Function Tests
 * This file contains unit tests for the parse_parameter() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test numeric parameter extraction from G-code
 * - Test missing parameter handling (NaN return)
 * - Test parameter boundary conditions
 * - Test edge cases and malformed input
 * - Test various parameter formats
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <math.h>

// Include necessary headers for the beryllium module
#include <src/print/beryllium.h>

void setUp(void) {
    // No setup needed for parameter parsing tests
}

void tearDown(void) {
    // No teardown needed for parameter parsing tests
}

// Forward declaration for the function being tested
double parse_parameter(const char *line, const char *parameter);

// Test functions
void test_parse_parameter_null_line(void);
void test_parse_parameter_null_parameter(void);
void test_parse_parameter_both_null(void);
void test_parse_parameter_missing_parameter(void);
void test_parse_parameter_simple_x_value(void);
void test_parse_parameter_simple_y_value(void);
void test_parse_parameter_simple_z_value(void);
void test_parse_parameter_simple_e_value(void);
void test_parse_parameter_simple_f_value(void);
void test_parse_parameter_negative_values(void);
void test_parse_parameter_decimal_values(void);
void test_parse_parameter_parameter_at_end(void);
void test_parse_parameter_multiple_parameters(void);
void test_parse_parameter_whitespace_around_parameter(void);
void test_parse_parameter_case_sensitive(void);

void test_parse_parameter_null_line(void) {
    double result = parse_parameter(NULL, "X");
    TEST_ASSERT_TRUE(isnan(result));
}

void test_parse_parameter_null_parameter(void) {
    double result = parse_parameter("G1 X10", NULL);
    TEST_ASSERT_TRUE(isnan(result));
}

void test_parse_parameter_both_null(void) {
    double result = parse_parameter(NULL, NULL);
    TEST_ASSERT_TRUE(isnan(result));
}

void test_parse_parameter_missing_parameter(void) {
    double result = parse_parameter("G1 Y10 Z0.5", "X");
    TEST_ASSERT_TRUE(isnan(result));
}

void test_parse_parameter_simple_x_value(void) {
    double result = parse_parameter("G1 X10 Y20", "X");
    TEST_ASSERT_EQUAL_DOUBLE(10.0, result);
}

void test_parse_parameter_simple_y_value(void) {
    double result = parse_parameter("G1 X10 Y20", "Y");
    TEST_ASSERT_EQUAL_DOUBLE(20.0, result);
}

void test_parse_parameter_simple_z_value(void) {
    double result = parse_parameter("G1 Z0.5", "Z");
    TEST_ASSERT_EQUAL_DOUBLE(0.5, result);
}

void test_parse_parameter_simple_e_value(void) {
    double result = parse_parameter("G1 E2.5", "E");
    TEST_ASSERT_EQUAL_DOUBLE(2.5, result);
}

void test_parse_parameter_simple_f_value(void) {
    double result = parse_parameter("G1 F3000", "F");
    TEST_ASSERT_EQUAL_DOUBLE(3000.0, result);
}

void test_parse_parameter_negative_values(void) {
    double result = parse_parameter("G1 X-10.5", "X");
    TEST_ASSERT_EQUAL_DOUBLE(-10.5, result);
}

void test_parse_parameter_decimal_values(void) {
    double result = parse_parameter("G1 X10.123 Y20.456", "X");
    TEST_ASSERT_EQUAL_DOUBLE(10.123, result);
}

void test_parse_parameter_parameter_at_end(void) {
    double result = parse_parameter("G1 X10", "X");
    TEST_ASSERT_EQUAL_DOUBLE(10.0, result);
}

void test_parse_parameter_multiple_parameters(void) {
    double result1 = parse_parameter("G1 X10 Y20 Z0.5 E2.5 F3000", "X");
    double result2 = parse_parameter("G1 X10 Y20 Z0.5 E2.5 F3000", "Y");
    double result3 = parse_parameter("G1 X10 Y20 Z0.5 E2.5 F3000", "Z");
    double result4 = parse_parameter("G1 X10 Y20 Z0.5 E2.5 F3000", "E");
    double result5 = parse_parameter("G1 X10 Y20 Z0.5 E2.5 F3000", "F");

    TEST_ASSERT_EQUAL_DOUBLE(10.0, result1);
    TEST_ASSERT_EQUAL_DOUBLE(20.0, result2);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, result3);
    TEST_ASSERT_EQUAL_DOUBLE(2.5, result4);
    TEST_ASSERT_EQUAL_DOUBLE(3000.0, result5);
}

void test_parse_parameter_whitespace_around_parameter(void) {
    double result = parse_parameter("G1  X10  Y20  ", "X");
    TEST_ASSERT_EQUAL_DOUBLE(10.0, result);
}

void test_parse_parameter_case_sensitive(void) {
    double result = parse_parameter("G1 x10", "X");
    TEST_ASSERT_TRUE(isnan(result)); // Should be case sensitive
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_parameter_null_line);
    RUN_TEST(test_parse_parameter_null_parameter);
    RUN_TEST(test_parse_parameter_both_null);
    RUN_TEST(test_parse_parameter_missing_parameter);
    RUN_TEST(test_parse_parameter_simple_x_value);
    RUN_TEST(test_parse_parameter_simple_y_value);
    RUN_TEST(test_parse_parameter_simple_z_value);
    RUN_TEST(test_parse_parameter_simple_e_value);
    RUN_TEST(test_parse_parameter_simple_f_value);
    RUN_TEST(test_parse_parameter_negative_values);
    RUN_TEST(test_parse_parameter_decimal_values);
    RUN_TEST(test_parse_parameter_parameter_at_end);
    RUN_TEST(test_parse_parameter_multiple_parameters);
    RUN_TEST(test_parse_parameter_whitespace_around_parameter);
    RUN_TEST(test_parse_parameter_case_sensitive);

    return UNITY_END();
}