/*
 * Unity Test File: Firebird Alloc XSQLDA
 * Tests firebird_alloc_sqlda() — allocates an XSQLDA with room for `cols` columns.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

fb_xsqlda_min* firebird_alloc_sqlda(short cols);

void test_alloc_sqlda_zero_cols(void);
void test_alloc_sqlda_one_col(void);
void test_alloc_sqlda_twenty_cols(void);
void test_alloc_sqlda_sets_version_and_sqld(void);
void test_alloc_sqlda_null_on_calloc_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_alloc_sqlda_zero_cols(void) {
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(0);
    TEST_ASSERT_NOT_NULL(sqlda);
    TEST_ASSERT_EQUAL_INT(1, sqlda->version);
    TEST_ASSERT_EQUAL_INT(0, sqlda->sqln);
    TEST_ASSERT_EQUAL_INT(0, sqlda->sqld);
    free(sqlda);
}

void test_alloc_sqlda_one_col(void) {
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(1);
    TEST_ASSERT_NOT_NULL(sqlda);
    TEST_ASSERT_EQUAL_INT(1, sqlda->version);
    TEST_ASSERT_EQUAL_INT(1, sqlda->sqln);
    TEST_ASSERT_EQUAL_INT(0, sqlda->sqld);
    free(sqlda);
}

void test_alloc_sqlda_twenty_cols(void) {
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(FB_SQLDA_INIT_COLS);
    TEST_ASSERT_NOT_NULL(sqlda);
    TEST_ASSERT_EQUAL_INT(1, sqlda->version);
    TEST_ASSERT_EQUAL_INT(FB_SQLDA_INIT_COLS, sqlda->sqln);
    TEST_ASSERT_EQUAL_INT(0, sqlda->sqld);
    free(sqlda);
}

void test_alloc_sqlda_sets_version_and_sqld(void) {
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(5);
    TEST_ASSERT_NOT_NULL(sqlda);
    TEST_ASSERT_EQUAL_INT(1, sqlda->version);
    TEST_ASSERT_EQUAL_INT(5, sqlda->sqln);
    TEST_ASSERT_EQUAL_INT(0, sqlda->sqld);
    for (short i = 0; i < 5; i++) {
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqldata);
        TEST_ASSERT_NULL(sqlda->sqlvar[i].sqlind);
    }
    free(sqlda);
}

void test_alloc_sqlda_null_on_calloc_failure(void) {
    mock_system_set_calloc_failure(1);
    fb_xsqlda_min* sqlda = firebird_alloc_sqlda(5);
    TEST_ASSERT_NULL(sqlda);
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alloc_sqlda_zero_cols);
    RUN_TEST(test_alloc_sqlda_one_col);
    RUN_TEST(test_alloc_sqlda_twenty_cols);
    RUN_TEST(test_alloc_sqlda_sets_version_and_sqld);
    RUN_TEST(test_alloc_sqlda_null_on_calloc_failure);

    return UNITY_END();
}
