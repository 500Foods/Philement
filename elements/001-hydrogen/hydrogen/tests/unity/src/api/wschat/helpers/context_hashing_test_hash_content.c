/*
 * Unity Test File: chat_context_hash_content / chat_context_hash_json
 * This file contains unit tests for chat_context_hash_content() and
 * chat_context_hash_json() in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/context_hashing.h>
#include <src/api/wschat/helpers/storage_hash.h>

void test_hash_content_valid(void);
void test_hash_content_null(void);
void test_hash_content_zero_length(void);
void test_hash_content_deterministic(void);
void test_hash_json_valid(void);
void test_hash_json_null(void);

void setUp(void) {
}

void tearDown(void) {
}

/* Valid content produces a 43-char base64url hash */
void test_hash_content_valid(void) {
    const char* content = "Hello, World!";
    char* hash = chat_context_hash_content(content, strlen(content));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_size_t(43, strlen(hash));
    free(hash);
}

/* NULL content returns NULL */
void test_hash_content_null(void) {
    TEST_ASSERT_NULL(chat_context_hash_content(NULL, 5));
}

/* Zero length returns NULL */
void test_hash_content_zero_length(void) {
    TEST_ASSERT_NULL(chat_context_hash_content("x", 0));
}

/* Same input -> same hash (deterministic) */
void test_hash_content_deterministic(void) {
    const char* content = "deterministic";
    char* h1 = chat_context_hash_content(content, strlen(content));
    char* h2 = chat_context_hash_content(content, strlen(content));
    TEST_ASSERT_NOT_NULL(h1);
    TEST_ASSERT_NOT_NULL(h2);
    TEST_ASSERT_EQUAL_STRING(h1, h2);
    free(h1);
    free(h2);
}

/* Valid JSON string produces a hash */
void test_hash_json_valid(void) {
    const char* json = "{\"role\": \"user\", \"content\": \"hi\"}";
    char* hash = chat_context_hash_json(json);
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_size_t(43, strlen(hash));
    free(hash);
}

/* NULL JSON returns NULL */
void test_hash_json_null(void) {
    TEST_ASSERT_NULL(chat_context_hash_json(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hash_content_valid);
    RUN_TEST(test_hash_content_null);
    RUN_TEST(test_hash_content_zero_length);
    RUN_TEST(test_hash_content_deterministic);
    RUN_TEST(test_hash_json_valid);
    RUN_TEST(test_hash_json_null);
    return UNITY_END();
}
