/*
 * Unity Test File: chat_context_calculate_bandwidth_savings
 * This file contains unit tests for chat_context_calculate_bandwidth_savings()
 * in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/context_hashing.h>

void test_savings_null_result(void);
void test_savings_zero_hashes_found(void);
void test_savings_zero_request_size(void);
void test_savings_content_too_small(void);
void test_savings_positive(void);

void setUp(void) {
}

void tearDown(void) {
}

/* NULL result -> 0.0 */
void test_savings_null_result(void) {
    TEST_ASSERT_EQUAL_DOUBLE(0.0, chat_context_calculate_bandwidth_savings(NULL, 100));
}

/* No hashes found -> 0.0 */
void test_savings_zero_hashes_found(void) {
    ContextHashResult result = {0};
    result.hashes_found = 0;
    TEST_ASSERT_EQUAL_DOUBLE(0.0, chat_context_calculate_bandwidth_savings(&result, 100));
}

/* Zero request size -> 0.0 */
void test_savings_zero_request_size(void) {
    ContextHashResult result = {0};
    result.hashes_found = 1;
    TEST_ASSERT_EQUAL_DOUBLE(0.0, chat_context_calculate_bandwidth_savings(&result, 0));
}

/* Content smaller than hash representation -> 0.0 */
void test_savings_content_too_small(void) {
    ContextHashResult result = {0};
    result.hashes_found = 1;
    result.total_content_size = 10;   /* less than 53 */
    TEST_ASSERT_EQUAL_DOUBLE(0.0, chat_context_calculate_bandwidth_savings(&result, 100));
}

/* Real savings when content exceeds hash overhead */
void test_savings_positive(void) {
    ContextHashResult result = {0};
    result.hashes_found = 2;
    result.total_content_size = 200;   /* 200 - 106 = 94 saved */
    double pct = chat_context_calculate_bandwidth_savings(&result, 500);
    TEST_ASSERT_TRUE(pct > 0.0);
    /* 94 / 500 * 100 = 18.8% */
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 18.8, pct);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_savings_null_result);
    RUN_TEST(test_savings_zero_hashes_found);
    RUN_TEST(test_savings_zero_request_size);
    RUN_TEST(test_savings_content_too_small);
    RUN_TEST(test_savings_positive);
    return UNITY_END();
}
