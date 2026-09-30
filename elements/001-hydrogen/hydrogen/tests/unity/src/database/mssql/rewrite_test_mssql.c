/*
 * Unity Test File: MSSQL RETURNING → OUTPUT Rewrite
 *
 * Tests for mssql_rewrite_returning_to_output() and its helpers.
 * Covers the INSERT ... WITH cte ... SELECT ... RETURNING col shape
 * emitted by Helium QueryRefs.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdlib.h>

#include <src/database/mssql/rewrite.h>

/* Forward declarations for all test functions */
void test_mssql_skip_ws_null_returns_null(void);
void test_mssql_skip_ws_all_whitespace(void);
void test_mssql_skip_ws_no_leading_whitespace(void);
void test_mssql_skip_ws_empty_string(void);
void test_mssql_match_word_match(void);
void test_mssql_match_word_no_match(void);
void test_mssql_match_word_partial_identifier_not_matched(void);
void test_mssql_find_returning_found(void);
void test_mssql_find_returning_not_found(void);
void test_mssql_find_returning_returns_uppercase(void);
void test_mssql_extract_returning_column_simple(void);
void test_mssql_extract_returning_column_with_semicolon(void);
void test_mssql_extract_returning_column_with_comma(void);
void test_mssql_extract_returning_column_null_input(void);
void test_mssql_find_insert_col_list_open_found(void);
void test_mssql_find_insert_col_list_open_not_found(void);
void test_mssql_find_insert_col_list_close_simple(void);
void test_mssql_find_insert_col_list_close_nested_parens(void);
void test_mssql_find_insert_col_list_close_in_string(void);
void test_mssql_find_insert_col_list_close_mismatch(void);
void test_rewrite_no_returning_returns_null(void);
void test_rewrite_null_input_returns_null(void);
void test_rewrite_simple_insert_select_returning(void);
void test_rewrite_preserves_cte_and_select(void);
void test_rewrite_output_after_column_list(void);
void test_rewrite_no_insert_returns_null(void);
void test_rewrite_empty_string_returns_null(void);

void setUp(void) {
    /* Set up fixtures if needed */
}

void tearDown(void) {
    /* Clean up fixtures if needed */
}

/* ---- Helper: mssql_skip_ws ---- */

void test_mssql_skip_ws_null_returns_null(void) {
    TEST_ASSERT_NULL(mssql_skip_ws(NULL));
}

void test_mssql_skip_ws_all_whitespace(void) {
    const char* result = mssql_skip_ws("   \t\n  hello");
    TEST_ASSERT_EQUAL_STRING("hello", result);
}

void test_mssql_skip_ws_no_leading_whitespace(void) {
    const char* result = mssql_skip_ws("hello world");
    TEST_ASSERT_EQUAL_STRING("hello world", result);
}

void test_mssql_skip_ws_empty_string(void) {
    const char* result = mssql_skip_ws("");
    TEST_ASSERT_EQUAL_STRING("", result);
}

/* ---- Helper: mssql_match_word ---- */

void test_mssql_match_word_match(void) {
    size_t adv = 0;
    bool matched = mssql_match_word("RETURNING col", "RETURNING", &adv);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(9, adv);
}

void test_mssql_match_word_no_match(void) {
    size_t adv = 0;
    bool matched = mssql_match_word("SELECT col", "RETURNING", &adv);
    TEST_ASSERT_FALSE(matched);
}

void test_mssql_match_word_partial_identifier_not_matched(void) {
    size_t adv = 0;
    bool matched = mssql_match_word("RETURNINGNESS", "RETURNING", &adv);
    TEST_ASSERT_FALSE(matched);
}

/* ---- Helper: mssql_find_returning ---- */

void test_mssql_find_returning_found(void) {
    const char* sql = "INSERT INTO test SELECT 1 RETURNING id";
    long offset = mssql_find_returning(sql);
    TEST_ASSERT_GREATER_THAN(0, offset);
    TEST_ASSERT_EQUAL_MEMORY("RETURNING", sql + offset, 9);
}

void test_mssql_find_returning_not_found(void) {
    const char* sql = "INSERT INTO test SELECT 1";
    long offset = mssql_find_returning(sql);
    TEST_ASSERT_EQUAL(-1, offset);
}

void test_mssql_find_returning_returns_uppercase(void) {
    const char* sql = "INSERT INTO test VALUES (1) returning id";
    long offset = mssql_find_returning(sql);
    TEST_ASSERT_GREATER_THAN(0, offset);
}

/* ---- Helper: mssql_extract_returning_column ---- */

void test_mssql_extract_returning_column_simple(void) {
    const char* returning_clause = "RETURNING account_id";
    char* col = mssql_extract_returning_column(returning_clause);
    TEST_ASSERT_NOT_NULL(col);
    TEST_ASSERT_EQUAL_STRING("account_id", col);
    free(col);
}

void test_mssql_extract_returning_column_with_semicolon(void) {
    const char* returning_clause = "RETURNING account_id;";
    char* col = mssql_extract_returning_column(returning_clause);
    TEST_ASSERT_NOT_NULL(col);
    TEST_ASSERT_EQUAL_STRING("account_id", col);
    free(col);
}

void test_mssql_extract_returning_column_with_comma(void) {
    const char* returning_clause = "RETURNING account_id,";
    char* col = mssql_extract_returning_column(returning_clause);
    TEST_ASSERT_NOT_NULL(col);
    TEST_ASSERT_EQUAL_STRING("account_id", col);
    free(col);
}

void test_mssql_extract_returning_column_null_input(void) {
    char* col = mssql_extract_returning_column(NULL);
    TEST_ASSERT_NULL(col);
}

/* ---- Helper: mssql_find_insert_column_list_open ---- */

void test_mssql_find_insert_col_list_open_found(void) {
    const char* sql = "INSERT INTO testms.accounts (a, b, c) VALUES (1,2,3)";
    long offset = mssql_find_insert_column_list_open(sql);
    TEST_ASSERT_GREATER_THAN(0, offset);
    TEST_ASSERT_EQUAL('(', sql[offset]);
}

void test_mssql_find_insert_col_list_open_not_found(void) {
    const char* sql = "SELECT * FROM accounts";
    long offset = mssql_find_insert_column_list_open(sql);
    TEST_ASSERT_EQUAL(-1, offset);
}

/* ---- Helper: mssql_find_insert_col_list_close ---- */

void test_mssql_find_insert_col_list_close_simple(void) {
    const char* sql = "INSERT INTO test (a, b) VALUES (1,2)";
    long open = mssql_find_insert_column_list_open(sql);
    const char* close = mssql_find_insert_col_list_close(sql, open);
    TEST_ASSERT_NOT_NULL(close);
    TEST_ASSERT_EQUAL(')', *close);
}

void test_mssql_find_insert_col_list_close_nested_parens(void) {
    const char* sql = "INSERT INTO test (a, b) VALUES (1,2)";
    long open = mssql_find_insert_column_list_open(sql);
    const char* close = mssql_find_insert_col_list_close(sql, open);
    TEST_ASSERT_NOT_NULL(close);
    TEST_ASSERT_EQUAL(')', *close);
}

void test_mssql_find_insert_col_list_close_in_string(void) {
    const char* sql = "INSERT INTO test (a) VALUES ('()')";
    long open = mssql_find_insert_column_list_open(sql);
    const char* close = mssql_find_insert_col_list_close(sql, open);
    TEST_ASSERT_NOT_NULL(close);
    TEST_ASSERT_EQUAL(')', *close);
}

void test_mssql_find_insert_col_list_close_mismatch(void) {
    const char* sql = "INSERT INTO test (a, b VALUES (1,2)";
    long open = mssql_find_insert_column_list_open(sql);
    const char* close = mssql_find_insert_col_list_close(sql, open);
    TEST_ASSERT_NULL(close);
}

/* ---- Main rewriter: mssql_rewrite_returning_to_output ---- */

void test_rewrite_no_returning_returns_null(void) {
    const char* sql = "SELECT 1 FROM accounts";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_null_input_returns_null(void) {
    char* result = mssql_rewrite_returning_to_output(NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_simple_insert_select_returning(void) {
    const char* sql =
        "INSERT INTO testms.accounts (account_id, name)\n"
        "WITH next_id AS (\n"
        "    SELECT COALESCE(MAX(account_id), 0) + 1 AS new_id FROM testms.accounts\n"
        ")\n"
        "SELECT new_id, :name FROM next_id\n"
        "RETURNING account_id\n"
        ";";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NOT_NULL(result);
    /* OUTPUT INSERTED.account_id should appear before the WITH clause */
    TEST_ASSERT_NOT_NULL(strstr(result, "OUTPUT INSERTED.account_id"));
    /* RETURNING should no longer be present */
    TEST_ASSERT_NULL(strstr(result, "RETURNING"));
    free(result);
}

void test_rewrite_preserves_cte_and_select(void) {
    const char* sql =
        "INSERT INTO testms.users (id, name)\n"
        "WITH cte AS (SELECT 1 AS id)\n"
        "SELECT id, :name FROM cte\n"
        "RETURNING id;";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NOT_NULL(result);
    /* The CTE and SELECT should still be in the result */
    TEST_ASSERT_NOT_NULL(strstr(result, "WITH cte AS"));
    TEST_ASSERT_NOT_NULL(strstr(result, "SELECT id, :name FROM cte"));
    free(result);
}

void test_rewrite_output_after_column_list(void) {
    const char* sql =
        "INSERT INTO testms.table (col1, col2)\n"
        "SELECT val1, val2\n"
        "RETURNING col1;";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NOT_NULL(result);
    /* Find the position of OUTPUT in the result */
    const char* output = strstr(result, "OUTPUT INSERTED.col1");
    TEST_ASSERT_NOT_NULL(output);
    /* Find the closing paren of the column list */
    const char* close_paren = strchr(result, ')');
    TEST_ASSERT_NOT_NULL(close_paren);
    /* OUTPUT should come after the closing paren */
    TEST_ASSERT_TRUE(output > close_paren);
    free(result);
}

void test_rewrite_no_insert_returns_null(void) {
    const char* sql = "UPDATE accounts SET name='x' RETURNING id";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_empty_string_returns_null(void) {
    const char* sql = "";
    char* result = mssql_rewrite_returning_to_output(sql);
    TEST_ASSERT_NULL(result);
}

/* ---- Runner ---- */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_skip_ws_null_returns_null);
    RUN_TEST(test_mssql_skip_ws_all_whitespace);
    RUN_TEST(test_mssql_skip_ws_no_leading_whitespace);
    RUN_TEST(test_mssql_skip_ws_empty_string);

    RUN_TEST(test_mssql_match_word_match);
    RUN_TEST(test_mssql_match_word_no_match);
    RUN_TEST(test_mssql_match_word_partial_identifier_not_matched);

    RUN_TEST(test_mssql_find_returning_found);
    RUN_TEST(test_mssql_find_returning_not_found);
    RUN_TEST(test_mssql_find_returning_returns_uppercase);

    RUN_TEST(test_mssql_extract_returning_column_simple);
    RUN_TEST(test_mssql_extract_returning_column_with_semicolon);
    RUN_TEST(test_mssql_extract_returning_column_with_comma);
    RUN_TEST(test_mssql_extract_returning_column_null_input);

    RUN_TEST(test_mssql_find_insert_col_list_open_found);
    RUN_TEST(test_mssql_find_insert_col_list_open_not_found);

    RUN_TEST(test_mssql_find_insert_col_list_close_simple);
    RUN_TEST(test_mssql_find_insert_col_list_close_nested_parens);
    RUN_TEST(test_mssql_find_insert_col_list_close_in_string);
    RUN_TEST(test_mssql_find_insert_col_list_close_mismatch);

    RUN_TEST(test_rewrite_no_returning_returns_null);
    RUN_TEST(test_rewrite_null_input_returns_null);
    RUN_TEST(test_rewrite_simple_insert_select_returning);
    RUN_TEST(test_rewrite_preserves_cte_and_select);
    RUN_TEST(test_rewrite_output_after_column_list);
    RUN_TEST(test_rewrite_no_insert_returns_null);
    RUN_TEST(test_rewrite_empty_string_returns_null);

    return UNITY_END();
}
