/*
 * Unity Test File: Firebird SQL Expects Rows
 * Tests firebird_sql_expects_rows() — determines if SQL will produce a result set.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

/* Forward declaration for function being tested */
bool firebird_sql_expects_rows(const char* sql);

/* Test function prototypes */
void test_sql_expects_rows_null(void);
void test_sql_expects_rows_empty(void);
void test_sql_expects_rows_select(void);
void test_sql_expects_rows_select_lowercase(void);
void test_sql_expects_rows_select_partial_keyword(void);
void test_sql_expects_rows_with(void);
void test_sql_expects_rows_with_lowercase(void);
void test_sql_expects_rows_with_partial_keyword(void);
void test_sql_expects_rows_returning(void);
void test_sql_expects_rows_returning_lowercase(void);
void test_sql_expects_rows_returning_partial_keyword(void);
void test_sql_expects_rows_insert_no_returning(void);
void test_sql_expects_rows_update_no_returning(void);
void test_sql_expects_rows_delete_no_returning(void);
void test_sql_expects_rows_line_comment_before_select(void);
void test_sql_expects_rows_line_comment_before_select_eof(void);
void test_sql_expects_rows_block_comment_before_select(void);
void test_sql_expects_rows_block_comment_unterminated(void);
void test_sql_expects_rows_block_comment_eof(void);
void test_sql_expects_rows_leading_whitespace(void);
void test_sql_expects_rows_select_with_suffix(void);
void test_sql_expects_rows_with_embedded_returning(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_sql_expects_rows_null(void) {
    TEST_ASSERT_FALSE(firebird_sql_expects_rows(NULL));
}

void test_sql_expects_rows_empty(void) {
    TEST_ASSERT_FALSE(firebird_sql_expects_rows(""));
}

void test_sql_expects_rows_select(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("SELECT * FROM tbl"));
}

void test_sql_expects_rows_select_lowercase(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("select * from tbl"));
}

void test_sql_expects_rows_select_partial_keyword(void) {
    /* SELECT followed by alphanumeric is not a match */
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("SELECTX FROM tbl"));
}

void test_sql_expects_rows_with(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("WITH cte AS (SELECT 1) SELECT * FROM cte"));
}

void test_sql_expects_rows_with_lowercase(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("with cte as (select 1) select * from cte"));
}

void test_sql_expects_rows_with_partial_keyword(void) {
    /* WITH followed by alphanumeric is not a match */
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("WITHX AS (SELECT 1)"));
}

void test_sql_expects_rows_returning(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("INSERT INTO t VALUES (1) RETURNING id"));
}

void test_sql_expects_rows_returning_lowercase(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("insert into t values (1) returning id"));
}

void test_sql_expects_rows_returning_partial_keyword(void) {
    /* RETURNING followed by alphanumeric is not a match */
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("INSERT INTO t VALUES (1) RETURNINGX id"));
}

void test_sql_expects_rows_insert_no_returning(void) {
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("INSERT INTO t VALUES (1)"));
}

void test_sql_expects_rows_update_no_returning(void) {
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("UPDATE t SET x = 1"));
}

void test_sql_expects_rows_delete_no_returning(void) {
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("DELETE FROM t WHERE x = 1"));
}

void test_sql_expects_rows_line_comment_before_select(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("-- comment\nSELECT 1"));
}

void test_sql_expects_rows_line_comment_before_select_eof(void) {
    /* Single line comment at end of input, no keyword after */
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("-- just a comment"));
}

void test_sql_expects_rows_block_comment_before_select(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("/* comment */ SELECT 1"));
}

void test_sql_expects_rows_block_comment_unterminated(void) {
    /* Unterminated block comment — reaches \0, continues loop, then checks SELECT/WITH */
    TEST_ASSERT_FALSE(firebird_sql_expects_rows("/* unterminated comment"));
}

void test_sql_expects_rows_block_comment_eof(void) {
    /* Block comment at end — SELECT was already found before it, so returns true */
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("SELECT 1 /* comment */"));
}

void test_sql_expects_rows_leading_whitespace(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("   \t\n\t  SELECT 1"));
}

void test_sql_expects_rows_select_with_suffix(void) {
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("SELECT id, name FROM users WHERE active = 1"));
}

void test_sql_expects_rows_with_embedded_returning(void) {
    /* RETURNING appears in the middle of the SQL, not at the start */
    TEST_ASSERT_TRUE(firebird_sql_expects_rows("UPDATE t SET x = 1 WHERE id = 5 RETURNING x"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sql_expects_rows_null);
    RUN_TEST(test_sql_expects_rows_empty);
    RUN_TEST(test_sql_expects_rows_select);
    RUN_TEST(test_sql_expects_rows_select_lowercase);
    RUN_TEST(test_sql_expects_rows_select_partial_keyword);
    RUN_TEST(test_sql_expects_rows_with);
    RUN_TEST(test_sql_expects_rows_with_lowercase);
    RUN_TEST(test_sql_expects_rows_with_partial_keyword);
    RUN_TEST(test_sql_expects_rows_returning);
    RUN_TEST(test_sql_expects_rows_returning_lowercase);
    RUN_TEST(test_sql_expects_rows_returning_partial_keyword);
    RUN_TEST(test_sql_expects_rows_insert_no_returning);
    RUN_TEST(test_sql_expects_rows_update_no_returning);
    RUN_TEST(test_sql_expects_rows_delete_no_returning);
    RUN_TEST(test_sql_expects_rows_line_comment_before_select);
    RUN_TEST(test_sql_expects_rows_line_comment_before_select_eof);
    RUN_TEST(test_sql_expects_rows_block_comment_before_select);
    RUN_TEST(test_sql_expects_rows_block_comment_unterminated);
    RUN_TEST(test_sql_expects_rows_block_comment_eof);
    RUN_TEST(test_sql_expects_rows_leading_whitespace);
    RUN_TEST(test_sql_expects_rows_select_with_suffix);
    RUN_TEST(test_sql_expects_rows_with_embedded_returning);

    return UNITY_END();
}
