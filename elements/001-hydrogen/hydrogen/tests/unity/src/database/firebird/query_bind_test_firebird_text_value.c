/*
 * Unity Test File: Firebird Parameter Text Value
 * Tests firebird_param_text_value() — returns the text representation of a
 * TypedParameter for text-ish types, NULL for numeric types and null params.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

/* Forward declarations for functions being tested */
const char* firebird_param_text_value(const TypedParameter* param);

/* Test function prototypes */
void test_firebird_param_text_value_null_param(void);
void test_firebird_param_text_value_is_null(void);
void test_firebird_param_text_value_string(void);
void test_firebird_param_text_value_string_null_value(void);
void test_firebird_param_text_value_text(void);
void test_firebird_param_text_value_text_null_value(void);
void test_firebird_param_text_value_date(void);
void test_firebird_param_text_value_time(void);
void test_firebird_param_text_value_datetime(void);
void test_firebird_param_text_value_timestamp(void);
void test_firebird_param_text_value_integer(void);
void test_firebird_param_text_value_boolean(void);
void test_firebird_param_text_value_float(void);
void test_firebird_param_text_value_invalid_type(void);

void setUp(void) {
    /* No fixtures needed for text value tests */
}

void tearDown(void) {
    /* No cleanup needed */
}

void test_firebird_param_text_value_null_param(void) {
    TEST_ASSERT_NULL(firebird_param_text_value(NULL));
}

void test_firebird_param_text_value_is_null(void) {
    TypedParameter param = {0};
    param.is_null = true;
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = strdup("should_be_ignored");
    TEST_ASSERT_NULL(firebird_param_text_value(&param));
    free(param.value.string_value);
}

void test_firebird_param_text_value_string(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = strdup("hello");
    TEST_ASSERT_EQUAL_STRING("hello", firebird_param_text_value(&param));
    free(param.value.string_value);
}

void test_firebird_param_text_value_string_null_value(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = NULL;
    TEST_ASSERT_EQUAL_STRING("", firebird_param_text_value(&param));
}

void test_firebird_param_text_value_text(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = strdup("world");
    TEST_ASSERT_EQUAL_STRING("world", firebird_param_text_value(&param));
    free(param.value.text_value);
}

void test_firebird_param_text_value_text_null_value(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = NULL;
    TEST_ASSERT_EQUAL_STRING("", firebird_param_text_value(&param));
}

void test_firebird_param_text_value_date(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = strdup("2024-01-15");
    TEST_ASSERT_EQUAL_STRING("2024-01-15", firebird_param_text_value(&param));
    free(param.value.date_value);
}

void test_firebird_param_text_value_time(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = strdup("12:30:45");
    TEST_ASSERT_EQUAL_STRING("12:30:45", firebird_param_text_value(&param));
    free(param.value.time_value);
}

void test_firebird_param_text_value_datetime(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = strdup("2024-01-15 12:30:45");
    TEST_ASSERT_EQUAL_STRING("2024-01-15 12:30:45", firebird_param_text_value(&param));
    free(param.value.datetime_value);
}

void test_firebird_param_text_value_timestamp(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = strdup("2024-01-15 12:30:45.123");
    TEST_ASSERT_EQUAL_STRING("2024-01-15 12:30:45.123", firebird_param_text_value(&param));
    free(param.value.timestamp_value);
}

void test_firebird_param_text_value_integer(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TEST_ASSERT_NULL(firebird_param_text_value(&param));
}

void test_firebird_param_text_value_boolean(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = true;
    TEST_ASSERT_NULL(firebird_param_text_value(&param));
}

void test_firebird_param_text_value_float(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = 3.14;
    TEST_ASSERT_NULL(firebird_param_text_value(&param));
}

void test_firebird_param_text_value_invalid_type(void) {
    /* Unrecognized param type falls through switch default to return NULL */
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP + 1;  /* Invalid enum value */
    param.value.string_value = strdup("should_not_reach");
    TEST_ASSERT_NULL(firebird_param_text_value(&param));
    free(param.value.string_value);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_param_text_value_null_param);
    RUN_TEST(test_firebird_param_text_value_is_null);
    RUN_TEST(test_firebird_param_text_value_string);
    RUN_TEST(test_firebird_param_text_value_string_null_value);
    RUN_TEST(test_firebird_param_text_value_text);
    RUN_TEST(test_firebird_param_text_value_text_null_value);
    RUN_TEST(test_firebird_param_text_value_date);
    RUN_TEST(test_firebird_param_text_value_time);
    RUN_TEST(test_firebird_param_text_value_datetime);
    RUN_TEST(test_firebird_param_text_value_timestamp);
    RUN_TEST(test_firebird_param_text_value_integer);
    RUN_TEST(test_firebird_param_text_value_boolean);
    RUN_TEST(test_firebird_param_text_value_float);
    RUN_TEST(test_firebird_param_text_value_invalid_type);

    return UNITY_END();
}