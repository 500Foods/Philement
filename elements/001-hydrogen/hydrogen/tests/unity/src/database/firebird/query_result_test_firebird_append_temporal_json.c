/*
 * Unity Test File: Firebird Append Temporal JSON
 * Tests firebird_append_temporal_json() — formats DATE/TIME/TIMESTAMP
 * (and TZ variants) as JSON string values via isc_decode_* functions.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_LIBFBC
#define USE_MOCK_LIBFBC
#endif
#include <unity/mocks/mock_libfbclient.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

bool firebird_append_temporal_json(char** buf, size_t* size, size_t* cap,
                                   short typ, const char* sqldata, short sqllen);

void test_temporal_null_sqldata(void);
void test_temporal_short_sqllen(void);
void test_temporal_date_type(void);
void test_temporal_date_no_decode_ptr(void);
void test_temporal_time_type(void);
void test_temporal_time_no_decode_ptr(void);
void test_temporal_timestamp_type(void);
void test_temporal_timestamp_no_decode_ptr(void);
void test_temporal_unknown_type(void);
void test_temporal_time_tz_type(void);
void test_temporal_timestamp_tz_type(void);
void test_temporal_timestamp_tz_short_sqllen(void);

void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
    load_libfbclient_functions("test");
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

static char* init_buf(size_t cap) {
    char* buf = calloc(1, cap);
    TEST_ASSERT_NOT_NULL(buf);
    return buf;
}

void test_temporal_null_sqldata(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_DATE, NULL, 10));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    TEST_ASSERT_EQUAL(4, size);
    free(buf);
}

void test_temporal_short_sqllen(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    const char data = 'X';
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_DATE, &data, 3));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    TEST_ASSERT_EQUAL(4, size);
    free(buf);
}

void test_temporal_date_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_date = 0;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_DATE, (char*)&fake_date, 4));
    /* Mock isc_decode_sql_date zero-fills struct tm, so tm_year=0 → 1900, tm_mon=0 → 01, tm_mday=0 → 00 */
    TEST_ASSERT_EQUAL_STRING("\"1900-01-00\"", buf);
    free(buf);
}

void test_temporal_date_no_decode_ptr(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_date = 0;
    /* Set the decode pointer to NULL to test the fallback */
    isc_decode_sql_date_ptr = NULL;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_DATE, (char*)&fake_date, 4));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_temporal_time_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_time = 0;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_TIME, (char*)&fake_time, 4));
    /* Mock isc_decode_sql_time zero-fills struct tm, so we get 00:00:00 */
    TEST_ASSERT_EQUAL_STRING("\"00:00:00\"", buf);
    free(buf);
}

void test_temporal_time_no_decode_ptr(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_time = 0;
    isc_decode_sql_time_ptr = NULL;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TYPE_TIME, (char*)&fake_time, 4));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_temporal_timestamp_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    const char fake_ts[8] = {0};
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TIMESTAMP, fake_ts, 8));
    /* Mock isc_decode_timestamp zero-fills struct tm, ms=0 since time_ticks=0 */
    TEST_ASSERT_EQUAL_STRING("\"1900-01-00 00:00:00\"", buf);
    free(buf);
}

void test_temporal_timestamp_no_decode_ptr(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    const char fake_ts[8] = {0};
    isc_decode_timestamp_ptr = NULL;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TIMESTAMP, fake_ts, 8));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_temporal_unknown_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_data = 0;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, 9999, (char*)&fake_data, 4));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_temporal_time_tz_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    int fake_time = 0;
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TIME_TZ, (char*)&fake_time, 4));
    /* TIME_TZ goes to the time branch (uses isc_decode_sql_time_ptr) */
    TEST_ASSERT_EQUAL_STRING("\"00:00:00\"", buf);
    free(buf);
}

void test_temporal_timestamp_tz_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    const char fake_ts[8] = {0};
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TIMESTAMP_TZ, fake_ts, 8));
    /* TIMESTAMP_TZ goes to the timestamp branch (not TIME_TZ_EX which matches time first) */
    TEST_ASSERT_EQUAL_STRING("\"1900-01-00 00:00:00\"", buf);
    free(buf);
}

void test_temporal_timestamp_tz_short_sqllen(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    const char fake_ts[4] = {0};
    /* sqllen < sizeof(fb_isc_timestamp) (8 bytes) → null fallback */
    TEST_ASSERT_TRUE(firebird_append_temporal_json(&buf, &size, &cap, FB_SQL_TIMESTAMP, fake_ts, 4));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_temporal_null_sqldata);
    RUN_TEST(test_temporal_short_sqllen);
    RUN_TEST(test_temporal_date_type);
    RUN_TEST(test_temporal_date_no_decode_ptr);
    RUN_TEST(test_temporal_time_type);
    RUN_TEST(test_temporal_time_no_decode_ptr);
    RUN_TEST(test_temporal_timestamp_type);
    RUN_TEST(test_temporal_timestamp_no_decode_ptr);
    RUN_TEST(test_temporal_unknown_type);
    RUN_TEST(test_temporal_time_tz_type);
    RUN_TEST(test_temporal_timestamp_tz_type);
    RUN_TEST(test_temporal_timestamp_tz_short_sqllen);

    return UNITY_END();
}
