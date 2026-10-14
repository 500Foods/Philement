/*
 * Unity Test File: transaction_blocks_test_transaction_blocks_free
 * Unit tests for transaction_blocks_free cleanup behavior.
 */

#include <stdlib.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/transaction_blocks.h>

void setUp(void);
void tearDown(void);
void test_free_null_list_is_safe(void);
void test_free_parsed_multi_block_list(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_free_null_list_is_safe(void) {
    transaction_blocks_free(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_free_parsed_multi_block_list(void) {
    const char* sql =
        "CREATE TABLE t (a INTEGER);\n"
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;\n"
        "-- STATEMENT\n"
        "DELETE FROM t WHERE a = 1;\n"
        "-- TRANSACTION_END\n"
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (2);\n"
        "-- TRANSACTION_END\n"
        "DROP TABLE t;";

    TransactionBlockList* blocks = NULL;
    char* error = NULL;
    TEST_ASSERT_TRUE(transaction_blocks_parse(sql, &blocks, &error));
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_EQUAL_size_t(4, blocks->block_count);

    transaction_blocks_free(blocks);
    // ASAN flags any statement, block array, or list leak on the line above.
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_null_list_is_safe);
    RUN_TEST(test_free_parsed_multi_block_list);

    return UNITY_END();
}
