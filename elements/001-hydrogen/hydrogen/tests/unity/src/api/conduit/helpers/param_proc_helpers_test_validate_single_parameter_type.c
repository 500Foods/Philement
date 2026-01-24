/*
 * Unity Test File: Validate Single Parameter Type
 * This file contains unit tests for validate_single_parameter_type() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/conduit/helpers/param_proc_helpers.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_validate_single_parameter_type_integer(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_integer(123), 0));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_string("123"), 0));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(123.45), 0));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 0));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 0));
}

static void test_validate_single_parameter_type_string(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("test"), 1));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(123), 1));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(123.45), 1));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 1));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 1));
}

static void test_validate_single_parameter_type_boolean(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_true(), 2));
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_false(), 2));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(1), 2));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_string("true"), 2));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 2));
}

static void test_validate_single_parameter_type_float(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_real(123.45), 3));
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_integer(123), 3));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_string("123.45"), 3));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 3));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 3));
}

static void test_validate_single_parameter_type_text(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("text"), 4));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(123), 4));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(123.45), 4));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 4));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 4));
}

static void test_validate_single_parameter_type_date(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("2023-12-25"), 5));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(20231225), 5));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(2023.12), 5));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 5));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 5));
}

static void test_validate_single_parameter_type_time(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("14:30:00"), 6));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(143000), 6));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(14.5), 6));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 6));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 6));
}

static void test_validate_single_parameter_type_datetime(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("2023-12-25 14:30:00"), 7));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(20231225143000), 7));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(20231225.143000), 7));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 7));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 7));
}

static void test_validate_single_parameter_type_timestamp(void) {
    TEST_ASSERT_TRUE(validate_single_parameter_type(json_string("2023-12-25 14:30:00.123"), 8));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(20231225143000123), 8));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_real(20231225.143000123), 8));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_true(), 8));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_null(), 8));
}

static void test_validate_single_parameter_type_invalid_type(void) {
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_integer(123), 9));
    TEST_ASSERT_FALSE(validate_single_parameter_type(json_string("test"), -1));
}

static void test_validate_single_parameter_type_null(void) {
    TEST_ASSERT_FALSE(validate_single_parameter_type(NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_single_parameter_type_integer);
    RUN_TEST(test_validate_single_parameter_type_string);
    RUN_TEST(test_validate_single_parameter_type_boolean);
    RUN_TEST(test_validate_single_parameter_type_float);
    RUN_TEST(test_validate_single_parameter_type_text);
    RUN_TEST(test_validate_single_parameter_type_date);
    RUN_TEST(test_validate_single_parameter_type_time);
    RUN_TEST(test_validate_single_parameter_type_datetime);
    RUN_TEST(test_validate_single_parameter_type_timestamp);
    RUN_TEST(test_validate_single_parameter_type_invalid_type);
    RUN_TEST(test_validate_single_parameter_type_null);
    
    return UNITY_END();
}
