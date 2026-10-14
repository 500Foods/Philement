/*
 * Unity Test File: transaction_blocks_test_transaction_blocks_parse_errors
 * Unit tests for the malformed shapes rejected by transaction_blocks_parse.
 * Every rejection must name the violated rule and leave *out_blocks NULL.
 */

#include <stdlib.h>
#include <string.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/transaction_blocks.h>

void setUp(void);
void tearDown(void);
void test_parse_statement_marker_outside_block(void);
void test_parse_end_marker_without_start(void);
void test_parse_start_inside_open_block(void);
void test_parse_empty_block(void);
void test_parse_empty_block_with_only_comments(void);
void test_parse_unterminated_block(void);
void test_parse_error_after_valid_block_leaves_nothing_behind(void);
void test_parse_error_inside_block_frees_accumulated_statements(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_parse_statement_marker_outside_block(void) {
    const char* sql =
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "STATEMENT outside a transaction block"));
    TEST_ASSERT_NOT_NULL(strstr(error, "line 2"));
    free(error);
}

void test_parse_end_marker_without_start(void) {
    const char* sql = "UPDATE t SET a = 2;\n-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "TRANSACTION_END outside a transaction block"));
    free(error);
}

void test_parse_start_inside_open_block(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- TRANSACTION_START\n"
        "UPDATE t SET a = 2;\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "TRANSACTION_START inside an open transaction block"));
    free(error);
}

void test_parse_empty_block(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "empty transaction block"));
    free(error);
}

void test_parse_empty_block_with_only_comments(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "-- nothing but comments here\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "empty transaction block"));
    free(error);
}

void test_parse_unterminated_block(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "unterminated transaction block"));
    free(error);
}

void test_parse_error_after_valid_block_leaves_nothing_behind(void) {
    // A valid block followed by a stray END must fail as a whole and free
    // the partial list (ASAN catches a miss here).
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- TRANSACTION_END\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

void test_parse_error_inside_block_frees_accumulated_statements(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;\n"
        "-- TRANSACTION_START\n";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_FALSE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_statement_marker_outside_block);
    RUN_TEST(test_parse_end_marker_without_start);
    RUN_TEST(test_parse_start_inside_open_block);
    RUN_TEST(test_parse_empty_block);
    RUN_TEST(test_parse_empty_block_with_only_comments);
    RUN_TEST(test_parse_unterminated_block);
    RUN_TEST(test_parse_error_after_valid_block_leaves_nothing_behind);
    RUN_TEST(test_parse_error_inside_block_frees_accumulated_statements);

    return UNITY_END();
}
