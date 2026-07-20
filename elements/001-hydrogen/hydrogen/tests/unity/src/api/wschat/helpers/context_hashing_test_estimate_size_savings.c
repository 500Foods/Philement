/*
 * Unity Test File: chat_context_estimate_size_savings
 * This file contains unit tests for chat_context_estimate_size_savings()
 * in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/context_hashing.h>

void test_estimate_null(void);
void test_estimate_not_array(void);
void test_estimate_zero_hash_count(void);
void test_estimate_small_messages(void);
void test_estimate_large_messages(void);

static json_t* make_message(const char* content) {
    json_t* msg = json_object();
    json_object_set_new(msg, "content", json_string(content));
    return msg;
}

void setUp(void) {
}

void tearDown(void) {
}

/* NULL or non-array messages -> 0 */
void test_estimate_null(void) {
    TEST_ASSERT_EQUAL_size_t(0, chat_context_estimate_size_savings(NULL, 5));
}

/* Non-array messages -> 0 */
void test_estimate_not_array(void) {
    json_t* not_array = json_string("x");
    TEST_ASSERT_EQUAL_size_t(0, chat_context_estimate_size_savings(not_array, 5));
    json_decref(not_array);
}

/* Zero hash count -> 0 */
void test_estimate_zero_hash_count(void) {
    json_t* messages = json_array();
    json_array_append_new(messages, make_message("hello"));
    TEST_ASSERT_EQUAL_size_t(0, chat_context_estimate_size_savings(messages, 0));
    json_decref(messages);
}

/* Small messages: total smaller than hash representation -> 0 */
void test_estimate_small_messages(void) {
    json_t* messages = json_array();
    json_array_append_new(messages, make_message("hi"));
    size_t saved = chat_context_estimate_size_savings(messages, 1);
    TEST_ASSERT_EQUAL_size_t(0, saved);
    json_decref(messages);
}

/* Large messages: total exceeds hash representation -> positive savings */
void test_estimate_large_messages(void) {
    json_t* messages = json_array();
    json_array_append_new(messages, make_message("this is a long message that should be larger than 53 bytes of hash overhead"));

    size_t saved = chat_context_estimate_size_savings(messages, 1);
    TEST_ASSERT_TRUE(saved > 0);

    json_decref(messages);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_estimate_null);
    RUN_TEST(test_estimate_not_array);
    RUN_TEST(test_estimate_zero_hash_count);
    RUN_TEST(test_estimate_small_messages);
    RUN_TEST(test_estimate_large_messages);
    return UNITY_END();
}
