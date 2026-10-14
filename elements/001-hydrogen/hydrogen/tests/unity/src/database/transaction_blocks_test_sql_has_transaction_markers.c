/*
 * Unity Test File: transaction_blocks_test_sql_has_transaction_markers
 * Unit tests for the sql_has_transaction_markers pre-filter.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/transaction_blocks.h>

void setUp(void);
void tearDown(void);
void test_sql_has_transaction_markers_null_sql(void);
void test_sql_has_transaction_markers_empty_sql(void);
void test_sql_has_transaction_markers_plain_sql(void);
void test_sql_has_transaction_markers_start_marker(void);
void test_sql_has_transaction_markers_statement_marker(void);
void test_sql_has_transaction_markers_end_marker(void);
void test_sql_has_transaction_markers_marker_text_inside_literal(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_sql_has_transaction_markers_null_sql(void) {
    TEST_ASSERT_FALSE(sql_has_transaction_markers(NULL));
}

void test_sql_has_transaction_markers_empty_sql(void) {
    TEST_ASSERT_FALSE(sql_has_transaction_markers(""));
}

void test_sql_has_transaction_markers_plain_sql(void) {
    TEST_ASSERT_FALSE(sql_has_transaction_markers("SELECT 1;"));
    TEST_ASSERT_FALSE(sql_has_transaction_markers("INSERT INTO t (a) VALUES (1);"));
    TEST_ASSERT_FALSE(sql_has_transaction_markers("UPDATE t SET a = 2 WHERE b = 3;"));
    TEST_ASSERT_FALSE(sql_has_transaction_markers("DELETE FROM t;"));
    TEST_ASSERT_FALSE(sql_has_transaction_markers("CREATE TABLE t (a INTEGER);"));
}

void test_sql_has_transaction_markers_start_marker(void) {
    TEST_ASSERT_TRUE(sql_has_transaction_markers(
        "-- TRANSACTION_START\nINSERT INTO t VALUES (1);\n-- TRANSACTION_END"));
}

void test_sql_has_transaction_markers_statement_marker(void) {
    TEST_ASSERT_TRUE(sql_has_transaction_markers(
        "-- TRANSACTION_START\nINSERT INTO t VALUES (1);\n-- STATEMENT\nUPDATE t SET a = 2;\n-- TRANSACTION_END"));
}

void test_sql_has_transaction_markers_end_marker(void) {
    TEST_ASSERT_TRUE(sql_has_transaction_markers(
        "-- TRANSACTION_START\nINSERT INTO t VALUES (1);\n-- TRANSACTION_END"));
}

void test_sql_has_transaction_markers_marker_text_inside_literal(void) {
    // The pre-filter is a substring test by design: marker text inside a
    // string literal makes the caller take the parse path, which then
    // follows the documented "markers are line-start only" rule.
    TEST_ASSERT_TRUE(sql_has_transaction_markers(
        "SELECT '-- TRANSACTION_START' AS label;"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sql_has_transaction_markers_null_sql);
    RUN_TEST(test_sql_has_transaction_markers_empty_sql);
    RUN_TEST(test_sql_has_transaction_markers_plain_sql);
    RUN_TEST(test_sql_has_transaction_markers_start_marker);
    RUN_TEST(test_sql_has_transaction_markers_statement_marker);
    RUN_TEST(test_sql_has_transaction_markers_end_marker);
    RUN_TEST(test_sql_has_transaction_markers_marker_text_inside_literal);

    return UNITY_END();
}
