/*
 * Unity Test File: transaction_blocks_test_transaction_blocks_parse
 * Unit tests for the valid shapes accepted by transaction_blocks_parse.
 */

#include <stdlib.h>
#include <string.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/transaction_blocks.h>

void setUp(void);
void tearDown(void);
void test_parse_single_block_two_statements(void);
void test_parse_multiple_blocks(void);
void test_parse_bare_statement_before_and_after_blocks(void);
void test_parse_markers_with_leading_whitespace(void);
void test_parse_skips_empty_and_comment_only_segments(void);
void test_parse_multiline_statement_keeps_interior_newlines(void);
void test_parse_null_arguments(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_parse_single_block_two_statements(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t (a) VALUES (1);\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(1, blocks->block_count);

    TransactionBlock* block = &blocks->blocks[0];
    TEST_ASSERT_TRUE(block->is_transaction);
    TEST_ASSERT_EQUAL_size_t(2, block->statement_count);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO t (a) VALUES (1);", block->statements[0]);
    TEST_ASSERT_EQUAL_STRING("UPDATE t SET a = 2;", block->statements[1]);

    transaction_blocks_free(blocks);
}

void test_parse_multiple_blocks(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t (a) VALUES (1);\n"
        "-- TRANSACTION_END\n"
        "-- TRANSACTION_START\n"
        "DELETE FROM t;\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 3;\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(2, blocks->block_count);

    TEST_ASSERT_TRUE(blocks->blocks[0].is_transaction);
    TEST_ASSERT_EQUAL_size_t(1, blocks->blocks[0].statement_count);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO t (a) VALUES (1);", blocks->blocks[0].statements[0]);

    TEST_ASSERT_TRUE(blocks->blocks[1].is_transaction);
    TEST_ASSERT_EQUAL_size_t(2, blocks->blocks[1].statement_count);
    TEST_ASSERT_EQUAL_STRING("DELETE FROM t;", blocks->blocks[1].statements[0]);
    TEST_ASSERT_EQUAL_STRING("UPDATE t SET a = 3;", blocks->blocks[1].statements[1]);

    transaction_blocks_free(blocks);
}

void test_parse_bare_statement_before_and_after_blocks(void) {
    const char* sql =
        "CREATE TABLE t (a INTEGER);\n"
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- TRANSACTION_END\n"
        "DROP TABLE t;";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(3, blocks->block_count);

    TEST_ASSERT_FALSE(blocks->blocks[0].is_transaction);
    TEST_ASSERT_EQUAL_size_t(1, blocks->blocks[0].statement_count);
    TEST_ASSERT_EQUAL_STRING("CREATE TABLE t (a INTEGER);", blocks->blocks[0].statements[0]);

    TEST_ASSERT_TRUE(blocks->blocks[1].is_transaction);
    TEST_ASSERT_EQUAL_size_t(1, blocks->blocks[1].statement_count);

    TEST_ASSERT_FALSE(blocks->blocks[2].is_transaction);
    TEST_ASSERT_EQUAL_size_t(1, blocks->blocks[2].statement_count);
    TEST_ASSERT_EQUAL_STRING("DROP TABLE t;", blocks->blocks[2].statements[0]);

    transaction_blocks_free(blocks);
}

void test_parse_markers_with_leading_whitespace(void) {
    const char* sql =
        "   -- TRANSACTION_START\n"
        "   INSERT INTO t VALUES (1);\n"
        "\t-- STATEMENT\n"
        "   UPDATE t SET a = 2;\n"
        "   -- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(1, blocks->block_count);
    TEST_ASSERT_EQUAL_size_t(2, blocks->blocks[0].statement_count);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO t VALUES (1);", blocks->blocks[0].statements[0]);
    TEST_ASSERT_EQUAL_STRING("UPDATE t SET a = 2;", blocks->blocks[0].statements[1]);

    transaction_blocks_free(blocks);
}

void test_parse_skips_empty_and_comment_only_segments(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "\n"
        "-- STATEMENT\n"
        "-- just a comment, not executable SQL\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(1, blocks->block_count);
    TEST_ASSERT_EQUAL_size_t(2, blocks->blocks[0].statement_count);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO t VALUES (1);", blocks->blocks[0].statements[0]);
    TEST_ASSERT_EQUAL_STRING("UPDATE t SET a = 2;", blocks->blocks[0].statements[1]);

    transaction_blocks_free(blocks);
}

void test_parse_multiline_statement_keeps_interior_newlines(void) {
    const char* sql =
        "-- TRANSACTION_START\n"
        "INSERT INTO t (a, b)\n"
        "VALUES\n"
        "(1, 2);\n"
        "-- TRANSACTION_END";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(1, blocks->block_count);
    TEST_ASSERT_EQUAL_size_t(1, blocks->blocks[0].statement_count);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO t (a, b)\nVALUES\n(1, 2);",
                             blocks->blocks[0].statements[0]);

    transaction_blocks_free(blocks);
}

void test_parse_null_arguments(void) {
    TransactionBlockList* blocks = NULL;
    char* error = NULL;

    TEST_ASSERT_FALSE(transaction_blocks_parse(NULL, &blocks, &error));
    TEST_ASSERT_NULL(blocks);
    TEST_ASSERT_NOT_NULL(error);
    free(error);

    TEST_ASSERT_FALSE(transaction_blocks_parse("SELECT 1;", NULL, &error));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_single_block_two_statements);
    RUN_TEST(test_parse_multiple_blocks);
    RUN_TEST(test_parse_bare_statement_before_and_after_blocks);
    RUN_TEST(test_parse_markers_with_leading_whitespace);
    RUN_TEST(test_parse_skips_empty_and_comment_only_segments);
    RUN_TEST(test_parse_multiline_statement_keeps_interior_newlines);
    RUN_TEST(test_parse_null_arguments);

    return UNITY_END();
}
