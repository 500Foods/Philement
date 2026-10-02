/*
 * Unity Test File: MSSQL query helper functions
 * Tests mssql_trim_trailing_whitespace, mssql_format_datetime_string,
 * mssql_format_timestamp_string, mssql_normalize_iso8601_timestamp,
 * and mssql_cleanup_bound_values.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>

void setUp(void);
void tearDown(void);
void test_mssql_trim_trailing_whitespace_null(void);
void test_mssql_trim_trailing_whitespace_no_trailing(void);
void test_mssql_trim_trailing_whitespace_trailing_spaces(void);
void test_mssql_trim_trailing_whitespace_mixed_whitespace(void);
void test_mssql_trim_trailing_whitespace_all_whitespace(void);
void test_mssql_trim_trailing_whitespace_empty(void);
void test_mssql_format_datetime_string_null(void);
void test_mssql_format_datetime_string_short_string(void);
void test_mssql_format_datetime_string_no_decimal(void);
void test_mssql_format_datetime_string_with_fractional(void);
void test_mssql_format_datetime_string_no_space_at_10(void);
void test_mssql_format_timestamp_string_null(void);
void test_mssql_format_timestamp_string_short_string(void);
void test_mssql_format_timestamp_string_no_decimal(void);
void test_mssql_format_timestamp_string_truncate(void);
void test_mssql_format_timestamp_string_exact_length(void);
void test_mssql_format_timestamp_string_no_space_at_10(void);
void test_mssql_normalize_iso8601_null_input(void);
void test_mssql_normalize_iso8601_null_output(void);
void test_mssql_normalize_iso8601_small_buffer(void);
void test_mssql_normalize_iso8601_valid(void);
void test_mssql_normalize_iso8601_invalid_month(void);
void test_mssql_normalize_iso8601_invalid_day(void);
void test_mssql_normalize_iso8601_invalid_hour(void);
void test_mssql_normalize_iso8601_invalid_minute(void);
void test_mssql_normalize_iso8601_invalid_second(void);
void test_mssql_normalize_iso8601_invalid_format(void);
void test_mssql_normalize_iso8601_leap_second(void);
void test_mssql_cleanup_bound_values_null(void);
void test_mssql_cleanup_bound_values_empty(void);
void test_mssql_cleanup_bound_values_with_strings(void);
void test_mssql_cleanup_bound_values_with_null_entries(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mssql_trim_trailing_whitespace_null(void) {
    TEST_ASSERT_NULL(mssql_trim_trailing_whitespace(NULL));
}

void test_mssql_trim_trailing_whitespace_no_trailing(void) {
    char str[] = "hello";
    char* result = mssql_trim_trailing_whitespace(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
}

void test_mssql_trim_trailing_whitespace_trailing_spaces(void) {
    char str[] = "hello   ";
    char* result = mssql_trim_trailing_whitespace(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
}

void test_mssql_trim_trailing_whitespace_mixed_whitespace(void) {
    char str[] = "hello\t\n\r ";
    char* result = mssql_trim_trailing_whitespace(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
}

void test_mssql_trim_trailing_whitespace_all_whitespace(void) {
    char str[] = "   \t\n\r  ";
    char* result = mssql_trim_trailing_whitespace(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
}

void test_mssql_trim_trailing_whitespace_empty(void) {
    char str[] = "";
    char* result = mssql_trim_trailing_whitespace(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
}

void test_mssql_format_datetime_string_null(void) {
    TEST_ASSERT_NULL(mssql_format_datetime_string(NULL));
}

void test_mssql_format_datetime_string_short_string(void) {
    char str[] = "short";
    char* result = mssql_format_datetime_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("short", result);
}

void test_mssql_format_datetime_string_no_decimal(void) {
    char str[] = "2023-12-25 14:30:00";
    char* result = mssql_format_datetime_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", result);
}

void test_mssql_format_datetime_string_with_fractional(void) {
    char str[] = "2023-12-25 14:30:00.000000";
    char* result = mssql_format_datetime_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", result);
}

void test_mssql_format_datetime_string_no_space_at_10(void) {
    char str[] = "2023-12-25T14:30:00.000000";
    char* result = mssql_format_datetime_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25T14:30:00.000000", result);
}

void test_mssql_format_timestamp_string_null(void) {
    TEST_ASSERT_NULL(mssql_format_timestamp_string(NULL));
}

void test_mssql_format_timestamp_string_short_string(void) {
    char str[] = "short";
    char* result = mssql_format_timestamp_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("short", result);
}

void test_mssql_format_timestamp_string_no_decimal(void) {
    char str[] = "2023-12-25 14:30:00";
    char* result = mssql_format_timestamp_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", result);
}

void test_mssql_format_timestamp_string_truncate(void) {
    char str[] = "2023-12-25 14:30:00.000032";
    char* result = mssql_format_timestamp_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00.000", result);
}

void test_mssql_format_timestamp_string_exact_length(void) {
    char str[] = "2023-12-25 14:30:00.000";
    char* result = mssql_format_timestamp_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00.000", result);
}

void test_mssql_format_timestamp_string_no_space_at_10(void) {
    char str[] = "2023-12-25T14:30:00.000032";
    char* result = mssql_format_timestamp_string(str);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2023-12-25T14:30:00.000032", result);
}

void test_mssql_normalize_iso8601_null_input(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp(NULL, output, sizeof(output)));
}

void test_mssql_normalize_iso8601_null_output(void) {
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-25T14:30:00", NULL, 64));
}

void test_mssql_normalize_iso8601_small_buffer(void) {
    char output[10];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-25T14:30:00", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_valid(void) {
    char output[64];
    TEST_ASSERT_TRUE(mssql_normalize_iso8601_timestamp("2023-12-25T14:30:00", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", output);
}

void test_mssql_normalize_iso8601_invalid_month(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-13-25T14:30:00", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_invalid_day(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-32T14:30:00", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_invalid_hour(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-25T25:30:00", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_invalid_minute(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-25T14:60:00", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_invalid_second(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("2023-12-25T14:30:61", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_invalid_format(void) {
    char output[64];
    TEST_ASSERT_FALSE(mssql_normalize_iso8601_timestamp("not-a-timestamp", output, sizeof(output)));
}

void test_mssql_normalize_iso8601_leap_second(void) {
    char output[64];
    TEST_ASSERT_TRUE(mssql_normalize_iso8601_timestamp("2023-06-15T12:00:60", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("2023-06-15 12:00:60", output);
}

void test_mssql_cleanup_bound_values_null(void) {
    mssql_cleanup_bound_values(NULL, 0);
    TEST_PASS();
}

void test_mssql_cleanup_bound_values_empty(void) {
    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    mssql_cleanup_bound_values(bound_values, 0);
    TEST_PASS();
}

void test_mssql_cleanup_bound_values_with_strings(void) {
    void** bound_values = calloc(3, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    bound_values[0] = strdup("value1");
    bound_values[1] = strdup("value2");
    bound_values[2] = strdup("value3");
    mssql_cleanup_bound_values(bound_values, 3);
    TEST_PASS();
}

void test_mssql_cleanup_bound_values_with_null_entries(void) {
    void** bound_values = calloc(3, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    bound_values[0] = strdup("value1");
    bound_values[1] = NULL;
    bound_values[2] = strdup("value3");
    mssql_cleanup_bound_values(bound_values, 3);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_trim_trailing_whitespace_null);
    RUN_TEST(test_mssql_trim_trailing_whitespace_no_trailing);
    RUN_TEST(test_mssql_trim_trailing_whitespace_trailing_spaces);
    RUN_TEST(test_mssql_trim_trailing_whitespace_mixed_whitespace);
    RUN_TEST(test_mssql_trim_trailing_whitespace_all_whitespace);
    RUN_TEST(test_mssql_trim_trailing_whitespace_empty);

    RUN_TEST(test_mssql_format_datetime_string_null);
    RUN_TEST(test_mssql_format_datetime_string_short_string);
    RUN_TEST(test_mssql_format_datetime_string_no_decimal);
    RUN_TEST(test_mssql_format_datetime_string_with_fractional);
    RUN_TEST(test_mssql_format_datetime_string_no_space_at_10);

    RUN_TEST(test_mssql_format_timestamp_string_null);
    RUN_TEST(test_mssql_format_timestamp_string_short_string);
    RUN_TEST(test_mssql_format_timestamp_string_no_decimal);
    RUN_TEST(test_mssql_format_timestamp_string_truncate);
    RUN_TEST(test_mssql_format_timestamp_string_exact_length);
    RUN_TEST(test_mssql_format_timestamp_string_no_space_at_10);

    RUN_TEST(test_mssql_normalize_iso8601_null_input);
    RUN_TEST(test_mssql_normalize_iso8601_null_output);
    RUN_TEST(test_mssql_normalize_iso8601_small_buffer);
    RUN_TEST(test_mssql_normalize_iso8601_valid);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_month);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_day);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_hour);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_minute);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_second);
    RUN_TEST(test_mssql_normalize_iso8601_invalid_format);
    RUN_TEST(test_mssql_normalize_iso8601_leap_second);

    RUN_TEST(test_mssql_cleanup_bound_values_null);
    RUN_TEST(test_mssql_cleanup_bound_values_empty);
    RUN_TEST(test_mssql_cleanup_bound_values_with_strings);
    RUN_TEST(test_mssql_cleanup_bound_values_with_null_entries);

    return UNITY_END();
}
