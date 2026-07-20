/*
 * Unity Test File: chat_context_free helpers
 * This file contains unit tests for chat_context_free_hash(),
 * chat_context_free_hash_array(), and chat_context_free_result()
 * in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/context_hashing.h>

void test_free_hash_null(void);
void test_free_hash_valid(void);
void test_free_hash_array_null(void);
void test_free_hash_array_valid(void);
void test_free_result_null(void);
void test_free_result_valid(void);

void setUp(void) {
}

void tearDown(void) {
}

/* free_hash(NULL) is safe */
void test_free_hash_null(void) {
    chat_context_free_hash(NULL);
}

/* free_hash frees a valid hash */
void test_free_hash_valid(void) {
    char* hash = strdup("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG");
    TEST_ASSERT_NOT_NULL(hash);
    chat_context_free_hash(hash);
}

/* free_hash_array(NULL) is safe */
void test_free_hash_array_null(void) {
    chat_context_free_hash_array(NULL, 5);
}

/* free_hash_array frees all strings and the array */
void test_free_hash_array_valid(void) {
    char** hashes = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(hashes);
    hashes[0] = strdup("a");
    hashes[1] = strdup("b");
    hashes[2] = strdup("c");
    chat_context_free_hash_array(hashes, 3);
}

/* free_result(NULL) is safe */
void test_free_result_null(void) {
    chat_context_free_result(NULL);
}

/* free_result frees entries and content */
void test_free_result_valid(void) {
    ContextHashResult* result = calloc(1, sizeof(ContextHashResult));
    TEST_ASSERT_NOT_NULL(result);
    result->entry_count = 2;
    result->entries = calloc(2, sizeof(ContextHashEntry));
    TEST_ASSERT_NOT_NULL(result->entries);
    result->entries[0].hash = strdup("hash1");
    result->entries[0].content = strdup("content1");
    result->entries[1].hash = strdup("hash2");
    result->entries[1].content = NULL;

    chat_context_free_result(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_free_hash_null);
    RUN_TEST(test_free_hash_valid);
    RUN_TEST(test_free_hash_array_null);
    RUN_TEST(test_free_hash_array_valid);
    RUN_TEST(test_free_result_null);
    RUN_TEST(test_free_result_valid);
    return UNITY_END();
}
