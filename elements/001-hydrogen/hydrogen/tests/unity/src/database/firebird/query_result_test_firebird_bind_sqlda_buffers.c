/*
 * Unity Test File: Firebird Bind SSQLDA Buffers
 * Tests firebird_bind_sqlda_buffers() — allocates sqldata/sqlind for each column
 * in an XSQLDA based on the SQL type.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

bool firebird_bind_sqlda_buffers(fb_xsqlda_min* sqlda);

void test_bind_sqlda_buffers_null(void);
void test_bind_sqlda_buffers_sqld_zero(void);
void test_bind_sqlda_buffers_text_type(void);
void test_bind_sqlda_buffers_varying_type(void);
void test_bind_sqlda_buffers_blob_type(void);
void test_bind_sqlda_buffers_long_type(void);
void test_bind_sqlda_buffers_short_type(void);
void test_bind_sqlda_buffers_zero_length(void);
void test_bind_sqlda_buffers_sqldata_calloc_failure(void);
void test_bind_sqlda_buffers_sqlind_calloc_failure(void);
void test_bind_sqlda_buffers_negative_sqllen(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_bind_sqlda_buffers_null(void) {
    TEST_ASSERT_FALSE(firebird_bind_sqlda_buffers(NULL));
}

void test_bind_sqlda_buffers_sqld_zero(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + 10 * sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 10;
    sqlda->sqld = 0;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));

    for (short i = 0; i < 10; i++) {
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqlind);
    }

    free(sqlda);
}

void test_bind_sqlda_buffers_text_type(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_TEXT;
    sqlda->sqlvar[0].sqllen = 10;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_varying_type(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_VARYING;
    sqlda->sqlvar[0].sqllen = 20;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_blob_type(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_BLOB;
    sqlda->sqlvar[0].sqllen = 0;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_long_type(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_LONG;
    sqlda->sqlvar[0].sqllen = 4;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_short_type(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_SHORT;
    sqlda->sqlvar[0].sqllen = 2;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_zero_length(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_TEXT;
    sqlda->sqlvar[0].sqllen = 0;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

void test_bind_sqlda_buffers_sqldata_calloc_failure(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_LONG;
    sqlda->sqlvar[0].sqllen = 4;

    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(firebird_bind_sqlda_buffers(sqlda));
    mock_system_reset_all();

    free(sqlda);
}

void test_bind_sqlda_buffers_sqlind_calloc_failure(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_LONG;
    sqlda->sqlvar[0].sqllen = 4;

    mock_system_set_calloc_failure(2);
    TEST_ASSERT_FALSE(firebird_bind_sqlda_buffers(sqlda));
    mock_system_reset_all();

    free(sqlda);
}

void test_bind_sqlda_buffers_negative_sqllen(void) {
    fb_xsqlda_min* sqlda = calloc(1, sizeof(fb_xsqlda_min) + sizeof(fb_xsqlvar_min));
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->sqlvar[0].sqltype = FB_SQL_TEXT;
    sqlda->sqlvar[0].sqllen = -5;

    TEST_ASSERT_TRUE(firebird_bind_sqlda_buffers(sqlda));
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[0].sqlind);

    firebird_free_sqlda_buffers(sqlda);
    free(sqlda);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bind_sqlda_buffers_null);
    RUN_TEST(test_bind_sqlda_buffers_sqld_zero);
    RUN_TEST(test_bind_sqlda_buffers_text_type);
    RUN_TEST(test_bind_sqlda_buffers_varying_type);
    RUN_TEST(test_bind_sqlda_buffers_blob_type);
    RUN_TEST(test_bind_sqlda_buffers_long_type);
    RUN_TEST(test_bind_sqlda_buffers_short_type);
    RUN_TEST(test_bind_sqlda_buffers_zero_length);
    RUN_TEST(test_bind_sqlda_buffers_sqldata_calloc_failure);
    RUN_TEST(test_bind_sqlda_buffers_sqlind_calloc_failure);
    RUN_TEST(test_bind_sqlda_buffers_negative_sqllen);

    return UNITY_END();
}
