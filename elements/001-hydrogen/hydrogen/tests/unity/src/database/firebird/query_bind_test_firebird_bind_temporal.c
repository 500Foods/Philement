/*
 * Unity Test File: Firebird Bind Temporal
 * Tests firebird_bind_temporal() — writes a DATE/TIME/TIMESTAMP value
 * from text into an XSQLVAR buffer using Firebird encode functions.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>
#include <src/database/firebird/connection.h>
#include <unity/mocks/mock_libfbclient.h>

/* Forward declaration for function being tested */
bool firebird_bind_temporal(fb_xsqlvar_min* var, const char* text);

/* Test function prototypes */
void test_firebird_bind_temporal_null_var(void);
void test_firebird_bind_temporal_null_sqldata(void);
void test_firebird_bind_temporal_null_text(void);
void test_firebird_bind_temporal_unparseable_text(void);
void test_firebird_bind_temporal_date_kind2(void);
void test_firebird_bind_temporal_date_success(void);
void test_firebird_bind_temporal_time_kind1(void);
void test_firebird_bind_temporal_time_success(void);
void test_firebird_bind_temporal_time_tz_success(void);
void test_firebird_bind_temporal_time_tz_ex_success(void);
void test_firebird_bind_temporal_time_tz_short_buf(void);
void test_firebird_bind_temporal_timestamp_kind2(void);
void test_firebird_bind_temporal_timestamp_success(void);
void test_firebird_bind_temporal_timestamp_tz_success(void);
void test_firebird_bind_temporal_timestamp_tz_ex_short_buf(void);
void test_firebird_bind_temporal_unsupported_type(void);

void setUp(void) {
    mock_libfbc_reset_all();
    /* Assign mock encode function pointers so firebird_bind_temporal works. */
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;
}

void tearDown(void) {
    mock_libfbc_reset_all();
    isc_encode_sql_date_ptr = NULL;
    isc_encode_sql_time_ptr = NULL;
    isc_encode_timestamp_ptr = NULL;
}

void test_firebird_bind_temporal_null_var(void) {
    TEST_ASSERT_FALSE(firebird_bind_temporal(NULL, "2024-01-15"));
}

void test_firebird_bind_temporal_null_sqldata(void) {
    fb_xsqlvar_min var;
    memset(&var, 0, sizeof(var));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = NULL;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "2024-01-15"));
}

void test_firebird_bind_temporal_null_text(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, NULL));
}

void test_firebird_bind_temporal_unparseable_text(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "not a date"));
}

void test_firebird_bind_temporal_date_kind2(void) {
    /* kind == 2 (time), but type is DATE — should return false. */
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "12:30:45"));
}

void test_firebird_bind_temporal_date_success(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "2024-01-15"));
}

void test_firebird_bind_temporal_time_kind1(void) {
    /* kind == 1 (date), but type is TIME — should return false. */
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_TIME;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "2024-01-15"));
}

void test_firebird_bind_temporal_time_success(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TYPE_TIME;
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "12:30:45.500"));
}

void test_firebird_bind_temporal_time_tz_success(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIME_TZ;
    var.sqldata = buf;
    var.sqllen = 6;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "12:30:45"));
}

void test_firebird_bind_temporal_time_tz_ex_success(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIME_TZ_EX;
    var.sqldata = buf;
    var.sqllen = 6;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "12:30:45"));
}

void test_firebird_bind_temporal_time_tz_short_buf(void) {
    /* sqllen < 6 for TIME_TZ — should return false. */
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIME_TZ;
    var.sqldata = buf;
    var.sqllen = 5;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "12:30:45"));
}

void test_firebird_bind_temporal_timestamp_kind2(void) {
    /* kind == 2 (time), but type is TIMESTAMP — should return false. */
    fb_xsqlvar_min var;
    char buf[16];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIMESTAMP;
    var.sqldata = buf;
    var.sqllen = 8;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "12:30:45"));
}

void test_firebird_bind_temporal_timestamp_success(void) {
    fb_xsqlvar_min var;
    char buf[16];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIMESTAMP;
    var.sqldata = buf;
    var.sqllen = 8;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "2024-01-15 12:30:45.500"));
}

void test_firebird_bind_temporal_timestamp_tz_success(void) {
    fb_xsqlvar_min var;
    char buf[16];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIMESTAMP_TZ;
    var.sqldata = buf;
    var.sqllen = 10;
    TEST_ASSERT_TRUE(firebird_bind_temporal(&var, "2024-01-15 12:30:45"));
}

void test_firebird_bind_temporal_timestamp_tz_ex_short_buf(void) {
    /* sqllen < 10 for TIMESTAMP_TZ_EX — should return false. */
    fb_xsqlvar_min var;
    char buf[16];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = FB_SQL_TIMESTAMP_TZ_EX;
    var.sqldata = buf;
    var.sqllen = 9;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "2024-01-15 12:30:45"));
}

void test_firebird_bind_temporal_unsupported_type(void) {
    fb_xsqlvar_min var;
    char buf[8];
    memset(&var, 0, sizeof(var));
    memset(buf, 0, sizeof(buf));
    var.sqltype = 9999;  /* Unsupported type */
    var.sqldata = buf;
    var.sqllen = 4;
    TEST_ASSERT_FALSE(firebird_bind_temporal(&var, "2024-01-15"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_bind_temporal_null_var);
    RUN_TEST(test_firebird_bind_temporal_null_sqldata);
    RUN_TEST(test_firebird_bind_temporal_null_text);
    RUN_TEST(test_firebird_bind_temporal_unparseable_text);
    RUN_TEST(test_firebird_bind_temporal_date_kind2);
    RUN_TEST(test_firebird_bind_temporal_date_success);
    RUN_TEST(test_firebird_bind_temporal_time_kind1);
    RUN_TEST(test_firebird_bind_temporal_time_success);
    RUN_TEST(test_firebird_bind_temporal_time_tz_success);
    RUN_TEST(test_firebird_bind_temporal_time_tz_ex_success);
    RUN_TEST(test_firebird_bind_temporal_time_tz_short_buf);
    RUN_TEST(test_firebird_bind_temporal_timestamp_kind2);
    RUN_TEST(test_firebird_bind_temporal_timestamp_success);
    RUN_TEST(test_firebird_bind_temporal_timestamp_tz_success);
    RUN_TEST(test_firebird_bind_temporal_timestamp_tz_ex_short_buf);
    RUN_TEST(test_firebird_bind_temporal_unsupported_type);

    return UNITY_END();
}
