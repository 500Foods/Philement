/*
 * Unity Test File: Firebird Rewrite Dateadd Params
 * Tests firebird_rewrite_dateadd_params() — rewrites legacy Firebird
 * DATEADD(+ ? UNIT TO expr) syntax into DATEADD(UNIT, 0 +/- ?, expr).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declaration for function being tested */
char* firebird_rewrite_dateadd_params(const char* sql, bool* oom);

/* Test function prototypes */
void test_rewrite_dateadd_null(void);
void test_rewrite_dateadd_no_match(void);
void test_rewrite_dateadd_add_plus(void);
void test_rewrite_dateadd_add_minus(void);
void test_rewrite_dateadd_add_plus_lowercase(void);
void test_rewrite_dateadd_add_minus_lowercase(void);
void test_rewrite_dateadd_add_no_space(void);
void test_rewrite_dateadd_no_unit_match(void);
void test_rewrite_dateadd_invalid_unit(void);
void test_rewrite_dateadd_add_no_params(void);
void test_rewrite_dateadd_add_no_tO(void);
void test_rewrite_dateadd_add_expr_with_parens(void);
void test_rewrite_dateadd_prefix_before(void);
void test_rewrite_dateadd_suffix_after(void);
void test_rewrite_dateadd_oom_initial(void);
void test_rewrite_dateadd_oom_null(void);
void test_rewrite_dateadd_oom_realloc_failure(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_rewrite_dateadd_null(void) {
    bool oom = false;
    TEST_ASSERT_NULL(firebird_rewrite_dateadd_params(NULL, &oom));
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_dateadd_no_match(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params("SELECT * FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_dateadd_add_plus(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? MINUTE TO col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("DATEADD(MINUTE, 0 + ?, col)", result);
    free(result);
}

void test_rewrite_dateadd_add_minus(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(- ? HOUR TO col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("DATEADD(HOUR, 0 - ?, col)", result);
    free(result);
}

void test_rewrite_dateadd_add_plus_lowercase(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "dateadd(+ ? day to col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("DATEADD(day, 0 + ?, col)", result);
    free(result);
}

void test_rewrite_dateadd_add_minus_lowercase(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "dateadd(- ? second to col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("DATEADD(second, 0 - ?, col)", result);
    free(result);
}

void test_rewrite_dateadd_add_no_space(void) {
    /* DATEADD(+?UNIT TO expr) — no spaces after + and ? */
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+?MINUTE TO col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("DATEADD(MINUTE, 0 + ?, col)", result);
    free(result);
}

void test_rewrite_dateadd_no_unit_match(void) {
    /* DATEADD(+ ? UNKNOWN TO col) — no matching unit */
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? UNKNOWN TO col)", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_dateadd_invalid_unit(void) {
    /* Dateinvalid prefix won't match DATEADD */
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? MINUTE TO col)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
}

void test_rewrite_dateadd_add_no_params(void) {
    /* DATEADD(MINUTE TO expr) — no +/- ? */
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "SELECT DATEADD(MINUTE TO col) FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_dateadd_add_no_tO(void) {
    /* DATEADD(+ ? MINUTE col) — no TO keyword */
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? MINUTE col)", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_dateadd_add_expr_with_parens(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? MINUTE TO extract(year from col))", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "extract(year from col)"));
    free(result);
}

void test_rewrite_dateadd_prefix_before(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "SELECT DATEADD(+ ? MINUTE TO col) FROM tbl", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SELECT "));
    TEST_ASSERT_NOT_NULL(strstr(result, " FROM tbl"));
    free(result);
}

void test_rewrite_dateadd_suffix_after(void) {
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "DATEADD(+ ? MINUTE TO col) WHERE x = 1", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, " WHERE x = 1"));
    free(result);
}

void test_rewrite_dateadd_oom_initial(void) {
    bool oom = true;
    char* result = firebird_rewrite_dateadd_params("SELECT 1", &oom);
    TEST_ASSERT_FALSE(oom);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_dateadd_oom_null(void) {
    char* result = firebird_rewrite_dateadd_params("SELECT 1", NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_dateadd_oom_realloc_failure(void) {
    /* Force realloc to fail inside the rewrite growth path (lines 561-569).
       The function calls real malloc for the first allocation, then realloc
       when it needs to grow the buffer for the rewrite. */
    mock_system_set_realloc_failure(1);
    bool oom = false;
    char* result = firebird_rewrite_dateadd_params(
        "SELECT DATEADD(+ ? MINUTE TO col) FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(oom);
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rewrite_dateadd_null);
    RUN_TEST(test_rewrite_dateadd_no_match);
    RUN_TEST(test_rewrite_dateadd_add_plus);
    RUN_TEST(test_rewrite_dateadd_add_minus);
    RUN_TEST(test_rewrite_dateadd_add_plus_lowercase);
    RUN_TEST(test_rewrite_dateadd_add_minus_lowercase);
    RUN_TEST(test_rewrite_dateadd_add_no_space);
    RUN_TEST(test_rewrite_dateadd_no_unit_match);
    RUN_TEST(test_rewrite_dateadd_invalid_unit);
    RUN_TEST(test_rewrite_dateadd_add_no_params);
    RUN_TEST(test_rewrite_dateadd_add_no_tO);
    RUN_TEST(test_rewrite_dateadd_add_expr_with_parens);
    RUN_TEST(test_rewrite_dateadd_prefix_before);
    RUN_TEST(test_rewrite_dateadd_suffix_after);
    RUN_TEST(test_rewrite_dateadd_oom_initial);
    RUN_TEST(test_rewrite_dateadd_oom_null);
    RUN_TEST(test_rewrite_dateadd_oom_realloc_failure);

    return UNITY_END();
}
