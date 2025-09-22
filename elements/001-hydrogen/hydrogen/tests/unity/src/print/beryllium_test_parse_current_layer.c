/*
 * Unity Test File: Beryllium parse_current_layer Function Tests
 * This file contains unit tests for the parse_current_layer() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test SET_PRINT_STATS_INFO format parsing
 * - Test LAYER_CHANGE format parsing
 * - Test standalone ;LAYER: format parsing
 * - Test edge cases and invalid inputs
 * - Test Z height vs layer number detection
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

void setUp(void) {
    // No setup needed for layer parsing tests
}

void tearDown(void) {
    // No teardown needed for layer parsing tests
}

// Forward declaration for the function being tested
int parse_current_layer(const char *line);

// Test functions
void test_parse_current_layer_null_input(void);
void test_parse_current_layer_empty_string(void);
void test_parse_current_layer_no_layer_info(void);
void test_parse_current_layer_set_print_stats_info(void);
void test_parse_current_layer_layer_change_with_layer_number(void);
void test_parse_current_layer_layer_change_with_z_height(void);
void test_parse_current_layer_standalone_layer_comment(void);
void test_parse_current_layer_malformed_layer_change(void);
void test_parse_current_layer_negative_layer_number(void);
void test_parse_current_layer_large_layer_number(void);

void test_parse_current_layer_null_input(void) {
    int result = parse_current_layer(NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_empty_string(void) {
    int result = parse_current_layer("");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_no_layer_info(void) {
    int result = parse_current_layer("G1 X10 Y10 Z0.5");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_set_print_stats_info(void) {
    int result = parse_current_layer("SET_PRINT_STATS_INFO CURRENT_LAYER=5");
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_parse_current_layer_layer_change_with_layer_number(void) {
    int result = parse_current_layer(";LAYER_CHANGE\n;LAYER:3");
    TEST_ASSERT_EQUAL_INT(3, result);
}

void test_parse_current_layer_layer_change_with_z_height(void) {
    int result = parse_current_layer(";LAYER_CHANGE\n;Z:0.4");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_standalone_layer_comment(void) {
    int result = parse_current_layer(";LAYER:10");
    TEST_ASSERT_EQUAL_INT(10, result);
}

void test_parse_current_layer_malformed_layer_change(void) {
    int result = parse_current_layer(";LAYER_CHANGE\n;INVALID:DATA");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_negative_layer_number(void) {
    int result = parse_current_layer("SET_PRINT_STATS_INFO CURRENT_LAYER=-1");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_current_layer_large_layer_number(void) {
    int result = parse_current_layer("SET_PRINT_STATS_INFO CURRENT_LAYER=999");
    TEST_ASSERT_EQUAL_INT(999, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_current_layer_null_input);
    RUN_TEST(test_parse_current_layer_empty_string);
    RUN_TEST(test_parse_current_layer_no_layer_info);
    RUN_TEST(test_parse_current_layer_set_print_stats_info);
    RUN_TEST(test_parse_current_layer_layer_change_with_layer_number);
    RUN_TEST(test_parse_current_layer_layer_change_with_z_height);
    RUN_TEST(test_parse_current_layer_standalone_layer_comment);
    RUN_TEST(test_parse_current_layer_malformed_layer_change);
    RUN_TEST(test_parse_current_layer_negative_layer_number);
    RUN_TEST(test_parse_current_layer_large_layer_number);

    return UNITY_END();
}