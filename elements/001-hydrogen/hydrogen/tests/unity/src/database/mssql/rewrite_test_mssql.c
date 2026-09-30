/*
 * Unity Test File: MSSQL RETURNING → OUTPUT and INSERT...WITH rewrites
 *
 * Tests for mssql_rewrite_returning_to_output(),
 * mssql_rewrite_insert_with_to_with_insert(), and their helpers.
 * Covers the INSERT ... WITH cte ... SELECT shape emitted by Helium
 * QueryRef migrations, including a trailing RETURNING column.
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
void test_rewrite_insert_with_moves_cte_before_insert(void);
void test_rewrite_insert_with_preserves_quoted_payload(void);
void test_rewrite_insert_with_plain_insert_returns_null(void);
void test_rewrite_insert_with_select_returns_null(void);
void test_rewrite_insert_with_already_valid_returns_null(void);
void test_rewrite_insert_with_null_returns_null(void);
void test_rewrite_insert_with_cte_column_list(void);
void test_rewrite_cte_values_wraps_values_list(void);
void test_rewrite_cte_values_leaves_select_body(void);
void test_rewrite_cte_values_ignores_quoted_text(void);
void test_rewrite_cte_values_null_returns_null(void);
void test_rewrite_migration_sql_numbers_insert(void);
void test_rewrite_add_column_strips_keyword(void);
void test_rewrite_add_column_keeps_drop_column(void);
void test_rewrite_add_column_ignores_quoted_text(void);
void test_rewrite_add_column_null_returns_null(void);
void test_rewrite_migration_sql_add_column(void);

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

/* ---- Lock 22: INSERT ... WITH cte ... SELECT → WITH cte ... INSERT ... SELECT ---- */

void test_rewrite_insert_with_moves_cte_before_insert(void) {
    const char* sql =
        "INSERT INTO testms.queries (\n"
        "    query_id,\n"
        "    code\n"
        ")\n"
        "WITH next_query_id AS (\n"
        "    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id\n"
        "    FROM testms.queries\n"
        ")\n"
        "SELECT new_query_id, 'x' AS code\n"
        "FROM next_query_id;";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NOT_NULL(result);

    const char* with_pos = strstr(result, "WITH next_query_id AS");
    const char* insert_pos = strstr(result, "INSERT INTO testms.queries");
    const char* select_pos = strstr(result, "SELECT new_query_id, 'x' AS code");
    TEST_ASSERT_NOT_NULL(with_pos);
    TEST_ASSERT_NOT_NULL(insert_pos);
    TEST_ASSERT_NOT_NULL(select_pos);
    TEST_ASSERT_TRUE(with_pos < insert_pos);
    TEST_ASSERT_TRUE(insert_pos < select_pos);
    TEST_ASSERT_NOT_NULL(strstr(result, "FROM next_query_id;"));
    free(result);
}

void test_rewrite_insert_with_preserves_quoted_payload(void) {
    /* The stored QueryRef body is a quoted string that itself contains
     * WITH and parentheses. The CTE closer must stop at the real CTE. */
    const char* sql =
        "INSERT INTO testms.queries (query_id, code)\n"
        "WITH next_query_id AS (\n"
        "    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id\n"
        "    FROM testms.queries\n"
        ")\n"
        "SELECT new_query_id, 'WITH fake AS (not real) ) tail' AS code\n"
        "FROM next_query_id;";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "'WITH fake AS (not real) ) tail'"));
    TEST_ASSERT_NOT_NULL(strstr(result, "COALESCE(MAX(query_id), 0) + 1"));

    const char* with_pos = strstr(result, "WITH next_query_id AS");
    const char* insert_pos = strstr(result, "INSERT INTO testms.queries");
    TEST_ASSERT_NOT_NULL(with_pos);
    TEST_ASSERT_NOT_NULL(insert_pos);
    TEST_ASSERT_TRUE(with_pos < insert_pos);
    free(result);
}

void test_rewrite_insert_with_plain_insert_returns_null(void) {
    const char* sql = "INSERT INTO testms.lookups (lookup_id, key_idx) VALUES (1, 2);";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_insert_with_select_returns_null(void) {
    const char* sql = "SELECT 1 FROM testms.queries";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_insert_with_already_valid_returns_null(void) {
    const char* sql =
        "WITH next_query_id AS (\n"
        "    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id\n"
        "    FROM testms.queries\n"
        ")\n"
        "INSERT INTO testms.queries (query_id, code)\n"
        "SELECT new_query_id, 'x' AS code\n"
        "FROM next_query_id;";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_insert_with_null_returns_null(void) {
    char* result = mssql_rewrite_insert_with_to_with_insert(NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_insert_with_cte_column_list(void) {
    const char* sql =
        "INSERT INTO testms.numbers (\n"
        "    numbers\n"
        ")\n"
        "WITH digits(i) AS (\n"
        "    SELECT 0 AS i\n"
        ")\n"
        "SELECT i FROM digits;";
    char* result = mssql_rewrite_insert_with_to_with_insert(sql);
    TEST_ASSERT_NOT_NULL(result);
    const char* with_pos = strstr(result, "WITH digits(i) AS");
    const char* insert_pos = strstr(result, "INSERT INTO testms.numbers");
    TEST_ASSERT_NOT_NULL(with_pos);
    TEST_ASSERT_NOT_NULL(insert_pos);
    TEST_ASSERT_TRUE(with_pos < insert_pos);
    TEST_ASSERT_NOT_NULL(strstr(result, "SELECT 0 AS i"));
    TEST_ASSERT_NULL(strstr(result, "SELECT * FROM"));
    free(result);
}

void test_rewrite_cte_values_wraps_values_list(void) {
    const char* sql =
        "WITH digits(i) AS (\n"
        "    VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9)\n"
        ")\n"
        "SELECT i FROM digits;";
    char* result = mssql_rewrite_cte_values(sql);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result,
        "AS (SELECT * FROM (VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9)) AS v(i))"));
    TEST_ASSERT_NULL(strstr(result, "AS (VALUES"));
    TEST_ASSERT_NOT_NULL(strstr(result, "SELECT i FROM digits;"));
    free(result);
}

void test_rewrite_cte_values_leaves_select_body(void) {
    const char* sql = "WITH digits(i) AS (SELECT 0 AS i) SELECT i FROM digits";
    char* result = mssql_rewrite_cte_values(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_cte_values_ignores_quoted_text(void) {
    const char* sql = "SELECT 'WITH digits(i) AS (VALUES (0),(1))' AS code";
    char* result = mssql_rewrite_cte_values(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_cte_values_null_returns_null(void) {
    char* result = mssql_rewrite_cte_values(NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_migration_sql_numbers_insert(void) {
    const char* sql =
        "INSERT INTO testms.numbers (\n"
        "    numbers\n"
        ")\n"
        "WITH digits(i) AS (\n"
        "    VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9)\n"
        ")\n"
        "SELECT\n"
        "    thousands.i * 1000\n"
        "    + hundreds.i * 100\n"
        "    + tens.i * 10\n"
        "    + units.i\n"
        "FROM\n"
        "    digits units,\n"
        "    digits tens,\n"
        "    digits hundreds,\n"
        "    digits thousands;";
    char* result = mssql_rewrite_migration_sql(sql);
    TEST_ASSERT_NOT_NULL(result);

    const char* with_pos = strstr(result, "WITH digits(i) AS");
    const char* insert_pos = strstr(result, "INSERT INTO testms.numbers");
    const char* select_pos = strstr(result, "thousands.i * 1000");
    TEST_ASSERT_NOT_NULL(with_pos);
    TEST_ASSERT_NOT_NULL(insert_pos);
    TEST_ASSERT_NOT_NULL(select_pos);
    TEST_ASSERT_TRUE(with_pos < insert_pos);
    TEST_ASSERT_TRUE(insert_pos < select_pos);
    TEST_ASSERT_NOT_NULL(strstr(result,
        "AS (SELECT * FROM (VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9)) AS v(i))"));
    TEST_ASSERT_NOT_NULL(strstr(result, "digits thousands;"));
    free(result);
}

void test_rewrite_add_column_strips_keyword(void) {
    const char* sql =
        "ALTER TABLE testms.convos\n"
        "    ADD COLUMN segment_refs NVARCHAR(MAX);\n"
        "ALTER TABLE testms.convos\n"
        "    ADD COLUMN engine_name NVARCHAR(50);";
    char* result = mssql_rewrite_add_column(sql);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "ADD segment_refs NVARCHAR(MAX);"));
    TEST_ASSERT_NOT_NULL(strstr(result, "ADD engine_name NVARCHAR(50);"));
    TEST_ASSERT_NULL(strstr(result, "COLUMN"));
    free(result);
}

void test_rewrite_add_column_keeps_drop_column(void) {
    const char* sql = "ALTER TABLE testms.convos DROP COLUMN segment_refs;";
    char* result = mssql_rewrite_add_column(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_add_column_ignores_quoted_text(void) {
    const char* sql = "SELECT 'ADD COLUMN secret' AS name";
    char* result = mssql_rewrite_add_column(sql);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_add_column_null_returns_null(void) {
    char* result = mssql_rewrite_add_column(NULL);
    TEST_ASSERT_NULL(result);
}

void test_rewrite_migration_sql_add_column(void) {
    const char* sql =
        "ALTER TABLE testms.convos\n"
        "    ADD COLUMN segment_refs NVARCHAR(MAX);";
    char* result = mssql_rewrite_migration_sql(sql);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(
        "ALTER TABLE testms.convos\n"
        "    ADD segment_refs NVARCHAR(MAX);",
        result);
    free(result);
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

    RUN_TEST(test_rewrite_insert_with_moves_cte_before_insert);
    RUN_TEST(test_rewrite_insert_with_preserves_quoted_payload);
    RUN_TEST(test_rewrite_insert_with_plain_insert_returns_null);
    RUN_TEST(test_rewrite_insert_with_select_returns_null);
    RUN_TEST(test_rewrite_insert_with_already_valid_returns_null);
    RUN_TEST(test_rewrite_insert_with_null_returns_null);
    RUN_TEST(test_rewrite_insert_with_cte_column_list);

    RUN_TEST(test_rewrite_cte_values_wraps_values_list);
    RUN_TEST(test_rewrite_cte_values_leaves_select_body);
    RUN_TEST(test_rewrite_cte_values_ignores_quoted_text);
    RUN_TEST(test_rewrite_cte_values_null_returns_null);
    RUN_TEST(test_rewrite_migration_sql_numbers_insert);

    RUN_TEST(test_rewrite_add_column_strips_keyword);
    RUN_TEST(test_rewrite_add_column_keeps_drop_column);
    RUN_TEST(test_rewrite_add_column_ignores_quoted_text);
    RUN_TEST(test_rewrite_add_column_null_returns_null);
    RUN_TEST(test_rewrite_migration_sql_add_column);

    return UNITY_END();
}
