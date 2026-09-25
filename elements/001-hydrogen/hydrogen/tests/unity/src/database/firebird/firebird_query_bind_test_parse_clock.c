/*
 * Unity Test File: Firebird Parse Clock
 * Tests firebird_parse_clock() — parses date/time/timestamp strings into a
 * struct tm and optional fractional tick output.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#include <time.h>

/* Forward declaration for function being tested */
int firebird_parse_clock(const char* text, void* tm_out, unsigned int* frac_ticks);

/* Test function prototypes */
void test_firebird_parse_clock_null_text(void);
void test_firebird_parse_clock_null_tm(void);
void test_firebird_parse_clock_null_frac(void);
void test_firebird_parse_clock_datetime_with_frac(void);
void test_firebird_parse_clock_datetime_no_frac(void);
void test_firebird_parse_clock_datetime_T_separator(void);
void test_firebird_parse_clock_invalid_datetime(void);
void test_firebird_parse_clock_date_only(void);
void test_firebird_parse_clock_date_invalid(void);
void test_firebird_parse_clock_time_with_frac(void);
void test_firebird_parse_clock_time_no_frac(void);
void test_firebird_parse_clock_time_invalid(void);
void test_firebird_parse_clock_unparseable(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebird_parse_clock_null_text(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock(NULL, &tm_out, &frac));
}

void test_firebird_parse_clock_null_tm(void) {
    unsigned int frac = 0;
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-01-15", NULL, &frac));
}

void test_firebird_parse_clock_null_frac(void) {
    struct tm tm_out;
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-01-15", &tm_out, NULL));
}

void test_firebird_parse_clock_datetime_with_frac(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    int result = firebird_parse_clock("2024-01-15 12:30:45.123456", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(3, result);
    TEST_ASSERT_EQUAL_INT(2024 - 1900, tm_out.tm_year);
    TEST_ASSERT_EQUAL_INT(0, tm_out.tm_mon);   /* January = 0 */
    TEST_ASSERT_EQUAL_INT(15, tm_out.tm_mday);
    TEST_ASSERT_EQUAL_INT(12, tm_out.tm_hour);
    TEST_ASSERT_EQUAL_INT(30, tm_out.tm_min);
    TEST_ASSERT_EQUAL_INT(45, tm_out.tm_sec);
    TEST_ASSERT_EQUAL_UINT(1234, frac);
}

void test_firebird_parse_clock_datetime_no_frac(void) {
    struct tm tm_out;
    unsigned int frac = 999;
    int result = firebird_parse_clock("2024-01-15 12:30:45", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(3, result);
    TEST_ASSERT_EQUAL_INT(0, tm_out.tm_year + 1900 - 2024);
    TEST_ASSERT_EQUAL_INT(15, tm_out.tm_mday);
    TEST_ASSERT_EQUAL_INT(12, tm_out.tm_hour);
    TEST_ASSERT_EQUAL_INT(45, tm_out.tm_sec);
    TEST_ASSERT_EQUAL_UINT(0, frac);
}

void test_firebird_parse_clock_datetime_T_separator(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    int result = firebird_parse_clock("2024-01-15T12:30:45", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(3, result);
    TEST_ASSERT_EQUAL_INT(15, tm_out.tm_mday);
    TEST_ASSERT_EQUAL_INT(12, tm_out.tm_hour);
    TEST_ASSERT_EQUAL_INT(30, tm_out.tm_min);
    TEST_ASSERT_EQUAL_INT(45, tm_out.tm_sec);
}

void test_firebird_parse_clock_invalid_datetime(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    /* Invalid month 13 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-13-15 12:30:45", &tm_out, &frac));
    /* Invalid hour 25 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-01-15 25:30:45", &tm_out, &frac));
    /* Invalid minute 61 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-01-15 12:61:45", &tm_out, &frac));
}

void test_firebird_parse_clock_date_only(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    int result = firebird_parse_clock("2024-01-15", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_INT(2024 - 1900, tm_out.tm_year);
    TEST_ASSERT_EQUAL_INT(0, tm_out.tm_mon);   /* January = 0 */
    TEST_ASSERT_EQUAL_INT(15, tm_out.tm_mday);
    TEST_ASSERT_EQUAL_UINT(0, frac);
}

void test_firebird_parse_clock_date_invalid(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    /* Invalid month 0, day 0 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-0-0", &tm_out, &frac));
    /* Invalid day 32 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("2024-01-32", &tm_out, &frac));
}

void test_firebird_parse_clock_time_with_frac(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    int result = firebird_parse_clock("12:30:45.500", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(12, tm_out.tm_hour);
    TEST_ASSERT_EQUAL_INT(30, tm_out.tm_min);
    TEST_ASSERT_EQUAL_INT(45, tm_out.tm_sec);
    TEST_ASSERT_EQUAL_UINT(5000, frac);
}

void test_firebird_parse_clock_time_no_frac(void) {
    struct tm tm_out;
    unsigned int frac = 999;
    int result = firebird_parse_clock("12:30:45", &tm_out, &frac);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(12, tm_out.tm_hour);
    TEST_ASSERT_EQUAL_INT(30, tm_out.tm_min);
    TEST_ASSERT_EQUAL_INT(45, tm_out.tm_sec);
    TEST_ASSERT_EQUAL_UINT(0, frac);
}

void test_firebird_parse_clock_time_invalid(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    /* Invalid hour 24 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("24:30:45", &tm_out, &frac));
    /* Invalid second 61 */
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("12:30:61", &tm_out, &frac));
}

void test_firebird_parse_clock_unparseable(void) {
    struct tm tm_out;
    unsigned int frac = 0;
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("not a date", &tm_out, &frac));
    TEST_ASSERT_EQUAL_INT(0, firebird_parse_clock("", &tm_out, &frac));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_parse_clock_null_text);
    RUN_TEST(test_firebird_parse_clock_null_tm);
    RUN_TEST(test_firebird_parse_clock_null_frac);
    RUN_TEST(test_firebird_parse_clock_datetime_with_frac);
    RUN_TEST(test_firebird_parse_clock_datetime_no_frac);
    RUN_TEST(test_firebird_parse_clock_datetime_T_separator);
    RUN_TEST(test_firebird_parse_clock_invalid_datetime);
    RUN_TEST(test_firebird_parse_clock_date_only);
    RUN_TEST(test_firebird_parse_clock_date_invalid);
    RUN_TEST(test_firebird_parse_clock_time_with_frac);
    RUN_TEST(test_firebird_parse_clock_time_no_frac);
    RUN_TEST(test_firebird_parse_clock_time_invalid);
    RUN_TEST(test_firebird_parse_clock_unparseable);

    return UNITY_END();
}
