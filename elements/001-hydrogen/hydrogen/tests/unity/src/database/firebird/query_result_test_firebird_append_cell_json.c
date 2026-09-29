/*
 * Unity Test File: Firebird Append Cell JSON
 * Tests firebird_append_cell_json() — formats a single SQL cell value as JSON,
 * handling all supported Firebird SQL types and null indicators.
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

bool firebird_append_cell_json(char** buf, size_t* size, size_t* cap, FirebirdConnection* fb_conn, const fb_xsqlvar_min* var);

static FirebirdConnection make_conn(void) {
    FirebirdConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.db_handle = (void*)0x01;
    conn.tr_handle = (void*)0x02;
    return conn;
}

static fb_xsqlvar_min make_var(short sqltype, short sqllen, char* sqldata, short* sqlind) {
    fb_xsqlvar_min var;
    memset(&var, 0, sizeof(var));
    var.sqltype = sqltype;
    var.sqllen = sqllen;
    var.sqldata = sqldata;
    var.sqlind = sqlind;
    return var;
}

void test_cell_null_indicator(void);
void test_cell_long(void);
void test_cell_short(void);
void test_cell_int64(void);
void test_cell_double(void);
void test_cell_float(void);
void test_cell_varying(void);
void test_cell_text(void);
void test_cell_text_trailing_spaces(void);
void test_cell_text_null_sqldata(void);
void test_cell_varying_null_sqldata(void);
void test_cell_date(void);
void test_cell_time(void);
void test_cell_timestamp(void);
void test_cell_blob_success(void);
void test_cell_blob_null_text(void);
void test_cell_blob_null_sqldata(void);
void test_cell_fallback_unknown_type(void);
void test_cell_null_sqlind_pointer(void);

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

void test_cell_null_indicator(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    short sqlind = -1;
    fb_xsqlvar_min var = make_var(FB_SQL_LONG, 4, NULL, &sqlind);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_null_sqlind_pointer(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    /* sqlind is NULL → no null check → proceeds to type dispatch */
    fb_xsqlvar_min var = make_var(FB_SQL_LONG, 4, NULL, NULL);
    /* With sqlind NULL, the null check is skipped; sqldata is NULL so
     * the LONG branch is skipped; falls through to "null" */
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_long(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    int val = 42;
    fb_xsqlvar_min var = make_var(FB_SQL_LONG, 4, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("42", buf);
    free(buf);
}

void test_cell_short(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    short val = -7;
    fb_xsqlvar_min var = make_var(FB_SQL_SHORT, 2, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("-7", buf);
    free(buf);
}

void test_cell_int64(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    long long val = 9223372036854775807LL;
    fb_xsqlvar_min var = make_var(FB_SQL_INT64, 8, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("9223372036854775807", buf);
    free(buf);
}

void test_cell_double(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    double val = 3.14159;
    fb_xsqlvar_min var = make_var(FB_SQL_DOUBLE, 8, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    /* DOUBLE uses %.17g so 3.14159 matches the other engines' JSON number */
    TEST_ASSERT_EQUAL_STRING("3.1415899999999999", buf);
    free(buf);
}

void test_cell_float(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    float val = 1.5f;
    fb_xsqlvar_min var = make_var(FB_SQL_FLOAT, 4, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("1.5", buf);
    free(buf);
}

void test_cell_varying(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    const char* text = "varying data";
    unsigned short vlen = (unsigned short)strlen(text);
    /* VARYING format: 2-byte length prefix + data */
    size_t total = 2 + vlen;
    char* data = malloc(total);
    TEST_ASSERT_NOT_NULL(data);
    memcpy(data, &vlen, sizeof(unsigned short));
    memcpy(data + 2, text, vlen);
    fb_xsqlvar_min var = make_var(FB_SQL_VARYING, (short)total, data, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"varying data\"", buf);
    free(data);
    free(buf);
}

void test_cell_text(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    const char* text = "text data";
    fb_xsqlvar_min var = make_var(FB_SQL_TEXT, (short)strlen(text), (char*)text, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"text data\"", buf);
    free(buf);
}

void test_cell_text_trailing_spaces(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    char text[] = "value   ";
    fb_xsqlvar_min var = make_var(FB_SQL_TEXT, 8, text, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"value\"", buf);
    free(buf);
}

void test_cell_text_null_sqldata(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    fb_xsqlvar_min var = make_var(FB_SQL_TEXT, 10, NULL, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_varying_null_sqldata(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    fb_xsqlvar_min var = make_var(FB_SQL_VARYING, 10, NULL, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_date(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    int val = 0;
    fb_xsqlvar_min var = make_var(FB_SQL_TYPE_DATE, 4, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"1900-01-00\"", buf);
    free(buf);
}

void test_cell_time(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    int val = 0;
    fb_xsqlvar_min var = make_var(FB_SQL_TYPE_TIME, 4, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"00:00:00\"", buf);
    free(buf);
}

void test_cell_timestamp(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    char ts[8] = {0};
    fb_xsqlvar_min var = make_var(FB_SQL_TIMESTAMP, 8, ts, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"1900-01-00 00:00:00\"", buf);
    free(buf);
}

void test_cell_blob_success(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    const char* blob_data = "blob text content";
    mock_isc_set_get_segment_data((const unsigned char*)blob_data, (unsigned short)strlen(blob_data));
    fb_quad_t blob_id = {1, 2};
    fb_xsqlvar_min var = make_var(FB_SQL_BLOB, 8, (char*)&blob_id, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("\"blob text content\"", buf);
    free(buf);
}

void test_cell_blob_null_text(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    /* Make open_blob2 fail so firebird_read_blob_text returns NULL */
    mock_isc_set_isc_open_blob2_result(1);
    fb_quad_t blob_id = {1, 2};
    fb_xsqlvar_min var = make_var(FB_SQL_BLOB, 8, (char*)&blob_id, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_blob_null_sqldata(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    fb_xsqlvar_min var = make_var(FB_SQL_BLOB, 8, NULL, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

void test_cell_fallback_unknown_type(void) {
    char* buf = init_buf(256);
    size_t size = 0;
    size_t cap = 256;
    FirebirdConnection conn = make_conn();
    int val = 0;
    fb_xsqlvar_min var = make_var(9999, 4, (char*)&val, NULL);
    TEST_ASSERT_TRUE(firebird_append_cell_json(&buf, &size, &cap, &conn, &var));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    free(buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cell_null_indicator);
    RUN_TEST(test_cell_null_sqlind_pointer);
    RUN_TEST(test_cell_long);
    RUN_TEST(test_cell_short);
    RUN_TEST(test_cell_int64);
    RUN_TEST(test_cell_double);
    RUN_TEST(test_cell_float);
    RUN_TEST(test_cell_varying);
    RUN_TEST(test_cell_text);
    RUN_TEST(test_cell_text_trailing_spaces);
    RUN_TEST(test_cell_text_null_sqldata);
    RUN_TEST(test_cell_varying_null_sqldata);
    RUN_TEST(test_cell_date);
    RUN_TEST(test_cell_time);
    RUN_TEST(test_cell_timestamp);
    RUN_TEST(test_cell_blob_success);
    RUN_TEST(test_cell_blob_null_text);
    RUN_TEST(test_cell_blob_null_sqldata);
    RUN_TEST(test_cell_fallback_unknown_type);

    return UNITY_END();
}
