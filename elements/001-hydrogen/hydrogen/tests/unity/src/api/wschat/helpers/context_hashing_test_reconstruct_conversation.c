/*
 * Unity Test File: chat_context_reconstruct_conversation
 * This file contains unit tests for chat_context_reconstruct_conversation()
 * in src/api/wschat/helpers/context_hashing.c
 *
 * Uses the real LRU cache to seed "found" segments without a database.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/context_hashing.h>
#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>

void test_reconstruct_null_database(void);
void test_reconstruct_null_fallback(void);
void test_reconstruct_fallback_only(void);
void test_reconstruct_hashes_resolved(void);
void test_reconstruct_unparseable_content(void);
void test_reconstruct_gap_fallback(void);
void test_reconstruct_no_hashes_no_fallback(void);

static const char* DB = "test_reconstruct_db";
static const char* VALID = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG";

static json_t* make_message(const char* role, const char* content) {
    json_t* msg = json_object();
    json_object_set_new(msg, "role", json_string(role));
    json_object_set_new(msg, "content", json_string(content));
    return msg;
}

void setUp(void) {
    chat_storage_cache_shutdown(DB);
}

void tearDown(void) {
    chat_storage_cache_shutdown(DB);
}

/* NULL database -> NULL */
void test_reconstruct_null_database(void) {
    json_t* fallback = json_array();
    json_array_append_new(fallback, make_message("user", "hi"));
    json_t* result = chat_context_reconstruct_conversation(NULL, NULL, 0, fallback, 1);
    TEST_ASSERT_NULL(result);
    json_decref(fallback);
}

/* NULL fallback_messages -> NULL */
void test_reconstruct_null_fallback(void) {
    TEST_ASSERT_NULL(chat_context_reconstruct_conversation(DB, NULL, 0, NULL, 0));
}

/* No hashes, fallback messages present -> returns fallback messages */
void test_reconstruct_fallback_only(void) {
    json_t* fallback = json_array();
    json_array_append_new(fallback, make_message("user", "hello"));
    json_array_append_new(fallback, make_message("assistant", "hi there"));

    json_t* result = chat_context_reconstruct_conversation(DB, NULL, 0, fallback, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(2, json_array_size(result));

    json_decref(result);
    json_decref(fallback);
}

/* Hashes resolved to valid JSON messages are reconstructed */
void test_reconstruct_hashes_resolved(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache(DB);
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache,
        VALID, "{\"role\": \"user\", \"content\": \"from cache\"}",
        strlen("{\"role\": \"user\", \"content\": \"from cache\"}"), false));

    const char* hashes[1];
    hashes[0] = VALID;

    json_t* fallback = json_array();
    json_array_append_new(fallback, make_message("user", "fallback"));

    json_t* result = chat_context_reconstruct_conversation(DB, hashes, 1, fallback, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(result));

    json_t* msg = json_array_get(result, 0);
    json_t* content = json_object_get(msg, "content");
    TEST_ASSERT_EQUAL_STRING("from cache", json_string_value(content));

    json_decref(result);
    json_decref(fallback);
}

/* Hash content that is not valid JSON is logged and skipped, fallback used */
void test_reconstruct_unparseable_content(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache(DB);
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, VALID, "this is not json",
                                        strlen("this is not json"), false));

    const char* hashes[1];
    hashes[0] = VALID;

    json_t* fallback = json_array();
    json_array_append_new(fallback, make_message("user", "fallback"));

    json_t* result = chat_context_reconstruct_conversation(DB, hashes, 1, fallback, 1);
    TEST_ASSERT_NOT_NULL(result);
    /* unparseable content leads to gap which fallback fills */
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(result));
    json_t* msg = json_array_get(result, 0);
    json_t* content = json_object_get(msg, "content");
    TEST_ASSERT_EQUAL_STRING("fallback", json_string_value(content));

    json_decref(result);
    json_decref(fallback);
}

/* Hash missing from cache -> gap filled from fallback messages */
void test_reconstruct_gap_fallback(void) {
    const char* hashes[1];
    hashes[0] = VALID;

    json_t* fallback = json_array();
    json_array_append_new(fallback, make_message("user", "fallback for gap"));

    json_t* result = chat_context_reconstruct_conversation(DB, hashes, 1, fallback, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(result));
    json_t* msg = json_array_get(result, 0);
    json_t* content = json_object_get(msg, "content");
    TEST_ASSERT_EQUAL_STRING("fallback for gap", json_string_value(content));

    json_decref(result);
    json_decref(fallback);
}

/* Neither hashes nor valid fallback messages -> NULL */
void test_reconstruct_no_hashes_no_fallback(void) {
    json_t* fallback = json_array();
    /* non-object entry, so nothing valid to reconstruct */
    json_array_append_new(fallback, json_string("not an object"));

    json_t* result = chat_context_reconstruct_conversation(DB, NULL, 0, fallback, 1);
    TEST_ASSERT_NULL(result);
    json_decref(fallback);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_reconstruct_null_database);
    RUN_TEST(test_reconstruct_null_fallback);
    RUN_TEST(test_reconstruct_fallback_only);
    RUN_TEST(test_reconstruct_hashes_resolved);
    RUN_TEST(test_reconstruct_unparseable_content);
    RUN_TEST(test_reconstruct_gap_fallback);
    RUN_TEST(test_reconstruct_no_hashes_no_fallback);
    return UNITY_END();
}
