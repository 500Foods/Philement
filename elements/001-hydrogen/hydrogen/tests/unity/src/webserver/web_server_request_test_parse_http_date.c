/*
 * Unity Test File: Web Server Request - Parse HTTP Date Test
 * This file contains unit tests for parse_http_date() function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>

// Standard library includes
#include <string.h>
#include <time.h>

// Forward declarations for functions being tested
bool format_http_date(time_t timestamp, char *buffer, size_t buffer_size);
time_t parse_http_date(const char *http_date);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// NULL parameter
static void test_parse_http_date_null(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date(NULL));
}

// Empty string
static void test_parse_http_date_empty(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date(""));
}

// Garbage input
static void test_parse_http_date_garbage(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("not a date"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("12345"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("hello world"));
}

// Missing "GMT" suffix
static void test_parse_http_date_missing_gmt(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:37"));
}

// Wrong timezone suffix
static void test_parse_http_date_wrong_timezone(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:37 UTC"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:37 EST"));
}

// Trailing content after valid date
static void test_parse_http_date_trailing_content(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:37 GMT extra"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:37 GMT "));
}

// Lowercase day/month names (strptime is case-sensitive in many locales)
static void test_parse_http_date_lowercase(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("sun, 06 nov 1994 08:49:37 gmt"));
}

// Valid RFC 7231 dates - well-known reference values
static void test_parse_http_date_known_value_1(void) {
    // "Sun, 06 Nov 1994 08:49:37 GMT" -> epoch 784111777
    time_t result = parse_http_date("Sun, 06 Nov 1994 08:49:37 GMT");
    TEST_ASSERT_EQUAL_INT64(784111777, result);
}

static void test_parse_http_date_known_value_2(void) {
    // "Wed, 15 Jan 2025 12:30:45 GMT" -> epoch 1736944245
    time_t result = parse_http_date("Wed, 15 Jan 2025 12:30:45 GMT");
    TEST_ASSERT_EQUAL_INT64(1736944245, result);
}

static void test_parse_http_date_known_value_3(void) {
    // "Fri, 31 Dec 1999 23:59:59 GMT" -> epoch 946684799
    time_t result = parse_http_date("Fri, 31 Dec 1999 23:59:59 GMT");
    TEST_ASSERT_EQUAL_INT64(946684799, result);
}

// Epoch boundary: "Thu, 01 Jan 1970 00:00:00 GMT" -> epoch 0
static void test_parse_http_date_epoch_zero(void) {
    time_t result = parse_http_date("Thu, 01 Jan 1970 00:00:00 GMT");
    TEST_ASSERT_EQUAL_INT64(0, result);
}

// Invalid day
static void test_parse_http_date_invalid_day(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 99 Nov 1994 08:49:37 GMT"));
}

// Invalid month
static void test_parse_http_date_invalid_month(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Xxx 1994 08:49:37 GMT"));
}

// Invalid year (too many digits / non-numeric)
static void test_parse_http_date_invalid_year(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 19xy 08:49:37 GMT"));
}

// Invalid time fields - non-numeric characters where numbers are expected
static void test_parse_http_date_invalid_time(void) {
    // strptime rejects non-numeric characters in time fields
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 ab:49:37 GMT"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:xy:37 GMT"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("Sun, 06 Nov 1994 08:49:zz GMT"));
}

// Missing day-of-week
static void test_parse_http_date_missing_weekday(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("06 Nov 1994 08:49:37 GMT"));
}

// Wrong format entirely
static void test_parse_http_date_wrong_format(void) {
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("1994-11-06T08:49:37Z"));
    TEST_ASSERT_EQUAL_INT64((time_t)-1, parse_http_date("06/11/1994 08:49:37"));
}

// Round-trip: format_http_date then parse_http_date should return original
static void test_parse_http_date_round_trip(void) {
    time_t original = 1609459200; // 2021-01-01 00:00:00 UTC
    char date_buf[64];

    bool formatted = format_http_date(original, date_buf, sizeof(date_buf));
    TEST_ASSERT_TRUE(formatted);

    time_t parsed = parse_http_date(date_buf);
    TEST_ASSERT_EQUAL_INT64(original, parsed);
}

// Round-trip with another value
static void test_parse_http_date_round_trip_2(void) {
    time_t original = 1234567890; // 2009-02-13 23:31:30 UTC
    char date_buf[64];

    bool formatted = format_http_date(original, date_buf, sizeof(date_buf));
    TEST_ASSERT_TRUE(formatted);

    time_t parsed = parse_http_date(date_buf);
    TEST_ASSERT_EQUAL_INT64(original, parsed);
}

// Round-trip for epoch zero
static void test_parse_http_date_round_trip_zero(void) {
    time_t original = 0;
    char date_buf[64];

    bool formatted = format_http_date(original, date_buf, sizeof(date_buf));
    TEST_ASSERT_TRUE(formatted);

    time_t parsed = parse_http_date(date_buf);
    TEST_ASSERT_EQUAL_INT64(original, parsed);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_http_date_null);
    RUN_TEST(test_parse_http_date_empty);
    RUN_TEST(test_parse_http_date_garbage);
    RUN_TEST(test_parse_http_date_missing_gmt);
    RUN_TEST(test_parse_http_date_wrong_timezone);
    RUN_TEST(test_parse_http_date_trailing_content);
    RUN_TEST(test_parse_http_date_lowercase);
    RUN_TEST(test_parse_http_date_known_value_1);
    RUN_TEST(test_parse_http_date_known_value_2);
    RUN_TEST(test_parse_http_date_known_value_3);
    RUN_TEST(test_parse_http_date_epoch_zero);
    RUN_TEST(test_parse_http_date_invalid_day);
    RUN_TEST(test_parse_http_date_invalid_month);
    RUN_TEST(test_parse_http_date_invalid_year);
    RUN_TEST(test_parse_http_date_invalid_time);
    RUN_TEST(test_parse_http_date_missing_weekday);
    RUN_TEST(test_parse_http_date_wrong_format);
    RUN_TEST(test_parse_http_date_round_trip);
    RUN_TEST(test_parse_http_date_round_trip_2);
    RUN_TEST(test_parse_http_date_round_trip_zero);

    return UNITY_END();
}
