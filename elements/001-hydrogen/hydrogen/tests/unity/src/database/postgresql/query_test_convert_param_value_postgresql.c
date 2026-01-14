/*
 * Unity Test File: PostgreSQL Parameter Value Conversion
 * This file contains unit tests for postgresql_convert_param_value function
 * Tests conversion of TypedParameter values to PostgreSQL string format
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/postgresql/query.h>

// Test function declarations
void test_postgresql_convert_param_value_null_parameter(void);
void test_postgresql_convert_param_value_integer_type(void);
void test_postgresql_convert_param_value_string_type(void);
void test_postgresql_convert_param_value_boolean_true(void);
void test_postgresql_convert_param_value_boolean_false(void);
void test_postgresql_convert_param_value_float_type(void);
void test_postgresql_convert_param_value_text_type(void);
void test_postgresql_convert_param_value_text_null(void);
void test_postgresql_convert_param_value_date_type(void);
void test_postgresql_convert_param_value_date_null(void);
void test_postgresql_convert_param_value_time_type(void);
void test_postgresql_convert_param_value_time_null(void);
void test_postgresql_convert_param_value_datetime_type(void);
void test_postgresql_convert_param_value_datetime_null(void);
void test_postgresql_convert_param_value_timestamp_type(void);
void test_postgresql_convert_param_value_timestamp_null(void);
void test_postgresql_convert_param_value_unknown_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test NULL parameter input
void test_postgresql_convert_param_value_null_parameter(void) {
    char* result = postgresql_convert_param_value(NULL, "TEST");
    TEST_ASSERT_NULL(result);
}

// Test INTEGER parameter type
void test_postgresql_convert_param_value_integer_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 12345;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("12345", result);
    free(result);
}

// Test STRING parameter type
void test_postgresql_convert_param_value_string_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = (char*)"test_string";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_string", result);
    free(result);
}

// Test BOOLEAN parameter type - true
void test_postgresql_convert_param_value_boolean_true(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = true;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);
    free(result);
}

// Test BOOLEAN parameter type - false
void test_postgresql_convert_param_value_boolean_false(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = false;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("false", result);
    free(result);
}

// Test FLOAT parameter type
void test_postgresql_convert_param_value_float_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = 99.99;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("99.99", result);
    free(result);
}

// Test TEXT parameter type with value
void test_postgresql_convert_param_value_text_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = (char*)"large text content";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("large text content", result);
    free(result);
}

// Test TEXT parameter type with NULL value
void test_postgresql_convert_param_value_text_null(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = NULL;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

// Test DATE parameter type with value
void test_postgresql_convert_param_value_date_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = (char*)"2025-01-15";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2025-01-15", result);
    free(result);
}

// Test DATE parameter type with NULL value
void test_postgresql_convert_param_value_date_null(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = NULL;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

// Test TIME parameter type with value
void test_postgresql_convert_param_value_time_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = (char*)"14:30:00";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("14:30:00", result);
    free(result);
}

// Test TIME parameter type with NULL value
void test_postgresql_convert_param_value_time_null(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = NULL;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

// Test DATETIME parameter type with value
void test_postgresql_convert_param_value_datetime_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = (char*)"2025-01-15 14:30:00";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2025-01-15 14:30:00", result);
    free(result);
}

// Test DATETIME parameter type with NULL value
void test_postgresql_convert_param_value_datetime_null(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = NULL;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

// Test TIMESTAMP parameter type with value
void test_postgresql_convert_param_value_timestamp_type(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = (char*)"2025-01-15 14:30:00.123";

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2025-01-15 14:30:00.123", result);
    free(result);
}

// Test TIMESTAMP parameter type with NULL value
void test_postgresql_convert_param_value_timestamp_null(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = NULL;

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

// Test unknown parameter type
void test_postgresql_convert_param_value_unknown_type(void) {
    TypedParameter param = {0};
    param.type = 999; // Unknown type

    char* result = postgresql_convert_param_value(&param, "TEST");
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_convert_param_value_null_parameter);
    RUN_TEST(test_postgresql_convert_param_value_integer_type);
    RUN_TEST(test_postgresql_convert_param_value_string_type);
    RUN_TEST(test_postgresql_convert_param_value_boolean_true);
    RUN_TEST(test_postgresql_convert_param_value_boolean_false);
    RUN_TEST(test_postgresql_convert_param_value_float_type);
    RUN_TEST(test_postgresql_convert_param_value_text_type);
    RUN_TEST(test_postgresql_convert_param_value_text_null);
    RUN_TEST(test_postgresql_convert_param_value_date_type);
    RUN_TEST(test_postgresql_convert_param_value_date_null);
    RUN_TEST(test_postgresql_convert_param_value_time_type);
    RUN_TEST(test_postgresql_convert_param_value_time_null);
    RUN_TEST(test_postgresql_convert_param_value_datetime_type);
    RUN_TEST(test_postgresql_convert_param_value_datetime_null);
    RUN_TEST(test_postgresql_convert_param_value_timestamp_type);
    RUN_TEST(test_postgresql_convert_param_value_timestamp_null);
    RUN_TEST(test_postgresql_convert_param_value_unknown_type);

    return UNITY_END();
}