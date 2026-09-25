/*
 * Unity Test File: Firebird Rewrite Engine SQL
 * Tests firebird_rewrite_engine_sql() — rewrites engine-specific SQL syntax:
 *   LENGTH() -> CHAR_LENGTH()
 *   AS FLOAT -> AS DOUBLE PRECISION
 *   LIMIT n [OFFSET m] -> OFFSET m ROWS FETCH FIRST n ROWS ONLY
 *   / ? -> / CAST(? AS INTEGER)
 * Also handles comment and string literal state tracking.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>
#include <src/database/firebird/connection.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declaration for function being tested */
char* firebird_rewrite_engine_sql(const char* sql, bool* oom);

/* Test function prototypes */
void test_rewrite_engine_sql_null(void);
void test_rewrite_engine_sql_empty(void);
void test_rewrite_engine_sql_no_change(void);
void test_rewrite_engine_sql_length(void);
void test_rewrite_engine_sql_length_case_variants(void);
void test_rewrite_engine_sql_length_not_replaced_in_identifier(void);
void test_rewrite_engine_sql_as_float(void);
void test_rewrite_engine_sql_as_float_case(void);
void test_rewrite_engine_sql_as_float_not_replaced(void);
void test_rewrite_engine_sql_limit_basic(void);
void test_rewrite_engine_sql_limit_with_offset(void);
void test_rewrite_engine_sql_limit_case(void);
void test_rewrite_engine_sql_limit_large_number(void);
void test_rewrite_engine_sql_limit_no_number(void);
void test_rewrite_engine_sql_division_cast(void);
void test_rewrite_engine_sql_division_no_cast(void);
void test_rewrite_engine_sql_line_comment(void);
void test_rewrite_engine_sql_block_comment(void);
void test_rewrite_engine_sql_string_literal(void);
void test_rewrite_engine_sql_string_literal_with_semicolon(void);
void test_rewrite_engine_sql_string_literal_doubled_quote(void);
void test_rewrite_engine_sql_combined(void);
void test_rewrite_engine_sql_oom_flag_set(void);
void test_rewrite_engine_sql_oom_flag_not_set(void);
void test_rewrite_engine_sql_oom_initial_malloc_failure(void);
void test_rewrite_engine_sql_limit_followed_by_text(void);
void test_rewrite_engine_sql_limit_offset_no_number(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_rewrite_engine_sql_null(void) {
    bool oom = false;
    TEST_ASSERT_NULL(firebird_rewrite_engine_sql(NULL, &oom));
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_empty(void) {
    bool oom = false;
    TEST_ASSERT_NULL(firebird_rewrite_engine_sql("", &oom));
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_no_change(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM table1", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_length(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT LENGTH(col) FROM tbl", &oom);
    TEST_ASSERT_NOT_NULL(result);
    /* Verify the output is the rewritten form with CHAR_LENGTH */
    TEST_ASSERT_NOT_NULL(strstr(result, "CHAR_LENGTH"));
    /* The standalone LENGTH(col) should have been replaced; check that
       any occurrence of "LENGTH(col)" is part of "CHAR_LENGTH(col)" */
    char* p = result;
    while ((p = strstr(p, "LENGTH(col)")) != NULL) {
        /* Must be preceded by "CHAR_" to be part of CHAR_LENGTH */
        TEST_ASSERT_TRUE(p >= result + 5 &&
                         strncmp(p - 5, "CHAR_", 5) == 0);
        p++;
    }
    free(result);
}

void test_rewrite_engine_sql_length_case_variants(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT length(col) FROM tbl", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "CHAR_LENGTH"));
    free(result);

    result = firebird_rewrite_engine_sql("SELECT Length(col) FROM tbl", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "CHAR_LENGTH"));
    free(result);
}

void test_rewrite_engine_sql_length_not_replaced_in_identifier(void) {
    /* LENGTH should not be replaced when preceded by an alphanumeric/_ char */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT myLENGTH(col) FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_as_float(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("CAST(x AS FLOAT)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "AS DOUBLE PRECISION"));
    free(result);
}

void test_rewrite_engine_sql_as_float_case(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("CAST(x As Float)", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "AS DOUBLE PRECISION"));
    free(result);
}

void test_rewrite_engine_sql_as_float_not_replaced(void) {
    /* FLOAT not preceded by AS — but actually AS is part of CAST.
       The check is for "AS FLOAT" specifically. */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT myfloat FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_limit_basic(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM tbl LIMIT 10", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "FETCH FIRST 10 ROWS ONLY"));
    free(result);
}

void test_rewrite_engine_sql_limit_with_offset(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM tbl LIMIT 10 OFFSET 5", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "OFFSET 5 ROWS FETCH FIRST 10 ROWS ONLY"));
    free(result);
}

void test_rewrite_engine_sql_limit_case(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM tbl limit 10", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "FETCH FIRST 10 ROWS ONLY"));
    free(result);
}

void test_rewrite_engine_sql_limit_large_number(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM tbl LIMIT 999999999", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "FETCH FIRST 999999999 ROWS ONLY"));
    free(result);
}

void test_rewrite_engine_sql_limit_no_number(void) {
    /* LIMIT followed by non-digit — should not rewrite */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT * FROM tbl LIMIT abc", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_division_cast(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT a / ? FROM tbl", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "CAST(? AS INTEGER)"));
    free(result);
}

void test_rewrite_engine_sql_division_no_cast(void) {
    /* Division not followed by ? — should not rewrite */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT a / b FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_line_comment(void) {
    /* LENGTH inside a line comment should not be rewritten;
       SELECT 1 has no changes either, so result is NULL (no rewrite) */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "-- SELECT LENGTH(x)\nSELECT 1", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_block_comment(void) {
    /* LENGTH inside a block comment should not be rewritten;
       SELECT 1 has no changes either, so result is NULL (no rewrite) */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "/* LENGTH(x) */ SELECT 1", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_string_literal(void) {
    /* LENGTH inside a string literal should not be rewritten */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT 'LENGTH(x)' FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_string_literal_with_semicolon(void) {
    /* String literal ending with semicolon should not start a line comment */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT 'hello;world' FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_string_literal_doubled_quote(void) {
    /* Doubled single quotes inside string should be preserved */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT 'it''s a test' FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_FALSE(oom);
}

void test_rewrite_engine_sql_combined(void) {
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT LENGTH(col), CAST(x AS FLOAT), a / ?, b / y "
        "FROM tbl LIMIT 5", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "CHAR_LENGTH"));
    TEST_ASSERT_NOT_NULL(strstr(result, "AS DOUBLE PRECISION"));
    TEST_ASSERT_NOT_NULL(strstr(result, "CAST(? AS INTEGER)"));
    TEST_ASSERT_NOT_NULL(strstr(result, "FETCH FIRST 5 ROWS ONLY"));
    TEST_ASSERT_NOT_NULL(strstr(result, "b / y"));
    free(result);
}

void test_rewrite_engine_sql_oom_flag_set(void) {
    /* When oom pointer is provided, it should be initialized to false */
    bool oom = true;  /* Pre-set to true to verify it gets reset */
    char* result = firebird_rewrite_engine_sql("SELECT 1", &oom);
    TEST_ASSERT_FALSE(oom);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_engine_sql_oom_flag_not_set(void) {
    /* When oom pointer is NULL, oom output should be handled gracefully */
    char* result = firebird_rewrite_engine_sql("SELECT 1", NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_engine_sql_oom_initial_malloc_failure(void) {
    /* Force the initial malloc to fail (line 654-659). */
    mock_system_set_malloc_failure(1);
    bool oom = false;
    char* result = firebird_rewrite_engine_sql("SELECT LENGTH(col) FROM tbl", &oom);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(oom);
    mock_system_reset_all();
}

void test_rewrite_engine_sql_limit_followed_by_text(void) {
    /* LIMIT 10 followed by text after the number (line 751: j++ skips
       trailing whitespace between the number and subsequent text) */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT * FROM tbl LIMIT 10 OFFSET 5 ROWS", &oom);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "OFFSET 5 ROWS FETCH FIRST 10 ROWS ONLY"));
    TEST_ASSERT_NOT_NULL(strstr(result, "ROWS"));
    free(result);
}

void test_rewrite_engine_sql_limit_offset_no_number(void) {
    /* OFFSET followed by no number (line 823: off_len = 0) —
       the count is used but offset is empty */
    bool oom = false;
    char* result = firebird_rewrite_engine_sql(
        "SELECT * FROM tbl LIMIT 10 OFFSET", &oom);
    /* OFFSET keyword is found but no number after it, so off_len=0,
       and the rewrite proceeds with FETCH FIRST 10 ROWS ONLY */
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "FETCH FIRST 10 ROWS ONLY"));
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rewrite_engine_sql_null);
    RUN_TEST(test_rewrite_engine_sql_empty);
    RUN_TEST(test_rewrite_engine_sql_no_change);
    RUN_TEST(test_rewrite_engine_sql_length);
    RUN_TEST(test_rewrite_engine_sql_length_case_variants);
    RUN_TEST(test_rewrite_engine_sql_length_not_replaced_in_identifier);
    RUN_TEST(test_rewrite_engine_sql_as_float);
    RUN_TEST(test_rewrite_engine_sql_as_float_case);
    RUN_TEST(test_rewrite_engine_sql_as_float_not_replaced);
    RUN_TEST(test_rewrite_engine_sql_limit_basic);
    RUN_TEST(test_rewrite_engine_sql_limit_with_offset);
    RUN_TEST(test_rewrite_engine_sql_limit_case);
    RUN_TEST(test_rewrite_engine_sql_limit_large_number);
    RUN_TEST(test_rewrite_engine_sql_limit_no_number);
    RUN_TEST(test_rewrite_engine_sql_division_cast);
    RUN_TEST(test_rewrite_engine_sql_division_no_cast);
    RUN_TEST(test_rewrite_engine_sql_line_comment);
    RUN_TEST(test_rewrite_engine_sql_block_comment);
    RUN_TEST(test_rewrite_engine_sql_string_literal);
    RUN_TEST(test_rewrite_engine_sql_string_literal_with_semicolon);
    RUN_TEST(test_rewrite_engine_sql_string_literal_doubled_quote);
    RUN_TEST(test_rewrite_engine_sql_combined);
    RUN_TEST(test_rewrite_engine_sql_oom_flag_set);
    RUN_TEST(test_rewrite_engine_sql_oom_flag_not_set);
    RUN_TEST(test_rewrite_engine_sql_oom_initial_malloc_failure);
    RUN_TEST(test_rewrite_engine_sql_limit_followed_by_text);
    RUN_TEST(test_rewrite_engine_sql_limit_offset_no_number);

    return UNITY_END();
}
