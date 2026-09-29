/*
 * Unity Test File: Firebird Free SSQLDA Buffers
 * Tests firebird_free_sqlda_buffers() — frees sqldata/sqlind pointers in an XSQLDA.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

void firebird_free_sqlda_buffers(fb_xsqlda_min* sqlda);

void test_free_sqlda_buffers_null(void);
void test_free_sqlda_buffers_no_columns(void);
void test_free_sqlda_buffers_with_buffers(void);
void test_free_sqlda_buffers_null_buffers(void);
void test_free_sqlda_buffers_sqln_greater_than_sqld(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_free_sqlda_buffers_null(void) {
    firebird_free_sqlda_buffers(NULL);
}

void test_free_sqlda_buffers_no_columns(void) {
    fb_xsqlda_min sqlda = {0};
    sqlda.version = 1;
    sqlda.sqln = 0;
    sqlda.sqld = 0;
    firebird_free_sqlda_buffers(&sqlda);
}

void test_free_sqlda_buffers_with_buffers(void) {
    short cols = 3;
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(cols);
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = cols;
    sqlda->sqld = cols;

    for (short i = 0; i < cols; i++) {
        sqlda->sqlvar[i].sqldata = malloc(10);
        sqlda->sqlvar[i].sqlind = malloc(sizeof(short));
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqlind);
    }

    firebird_free_sqlda_buffers(sqlda);

    for (short i = 0; i < cols; i++) {
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqlind);
    }

    free(sqlda);
}

void test_free_sqlda_buffers_null_buffers(void) {
    short cols = 2;
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(cols);
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = cols;
    sqlda->sqld = cols;

    sqlda->sqlvar[0].sqldata = NULL;
    sqlda->sqlvar[0].sqlind = NULL;
    sqlda->sqlvar[1].sqldata = malloc(10);
    sqlda->sqlvar[1].sqlind = NULL;
    TEST_ASSERT_NOT_NULL(sqlda->sqlvar[1].sqldata);

    firebird_free_sqlda_buffers(sqlda);

    TEST_ASSERT_NULL(sqlda->sqlvar[0].sqldata);
    TEST_ASSERT_NULL(sqlda->sqlvar[0].sqlind);
    TEST_ASSERT_NULL(sqlda->sqlvar[1].sqldata);
    TEST_ASSERT_NULL(sqlda->sqlvar[1].sqlind);

    free(sqlda);
}

void test_free_sqlda_buffers_sqln_greater_than_sqld(void) {
    short alloc_cols = 5;
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(alloc_cols);
    TEST_ASSERT_NOT_NULL(sqlda);
    sqlda->version = 1;
    sqlda->sqln = 5;
    sqlda->sqld = 2;

    for (short i = 0; i < alloc_cols; i++) {
        sqlda->sqlvar[i].sqldata = malloc(10);
        sqlda->sqlvar[i].sqlind = malloc(sizeof(short));
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqlind);
    }

    firebird_free_sqlda_buffers(sqlda);

    for (short i = 0; i < 2; i++) {
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqlind);
    }

    for (short i = 2; i < alloc_cols; i++) {
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NOT_NULL(sqlda->sqlvar[i].sqlind);
        free(sqlda->sqlvar[i].sqldata);
        free(sqlda->sqlvar[i].sqlind);
    }

    free(sqlda);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_sqlda_buffers_null);
    RUN_TEST(test_free_sqlda_buffers_no_columns);
    RUN_TEST(test_free_sqlda_buffers_with_buffers);
    RUN_TEST(test_free_sqlda_buffers_null_buffers);
    RUN_TEST(test_free_sqlda_buffers_sqln_greater_than_sqld);

    return UNITY_END();
}
