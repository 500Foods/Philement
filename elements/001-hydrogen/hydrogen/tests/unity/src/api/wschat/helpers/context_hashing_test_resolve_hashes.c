/*
 * Unity Test File: chat_context_resolve_hashes
 * This file contains unit tests for chat_context_resolve_hashes()
 * in src/api/wschat/helpers/context_hashing.c
 *
 * Uses the real LRU cache (chat_storage_get_or_create_cache /
 * chat_lru_cache_put) to seed "found" segments without a database.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/context_hashing.h>
#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>

void test_resolve_null_params(void);
void test_resolve_found(void);
void test_resolve_missing(void);
void test_resolve_null_hash_pointer(void);
void test_resolve_bandwidth_saved(void);

static const char* DB = "test_resolve_db";
static const char* VALID = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG";

void setUp(void) {
    chat_storage_cache_shutdown(DB);
}

void tearDown(void) {
    chat_storage_cache_shutdown(DB);
}

/* NULL database / hashes / zero count -> NULL */
void test_resolve_null_params(void) {
    const char* h = VALID;
    TEST_ASSERT_NULL(chat_context_resolve_hashes(NULL, &h, 1));
    TEST_ASSERT_NULL(chat_context_resolve_hashes(DB, NULL, 1));
    TEST_ASSERT_NULL(chat_context_resolve_hashes(DB, &h, 0));
}

/* Hash present in LRU cache -> found */
void test_resolve_found(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache(DB);
    TEST_ASSERT_NOT_NULL(cache);
    const char* content = "{\"role\": \"user\", \"content\": \"hello\"}";
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, VALID, content, strlen(content), false));

    const char* hashes[1];
    hashes[0] = VALID;

    ContextHashResult* result = chat_context_resolve_hashes(DB, hashes, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(1, result->entry_count);
    TEST_ASSERT_EQUAL_size_t(1, result->hashes_found);
    TEST_ASSERT_EQUAL_size_t(0, result->hashes_missing);
    TEST_ASSERT_EQUAL_size_t(strlen(content), result->total_content_size);
    TEST_ASSERT_TRUE(result->entries[0].is_valid);
    TEST_ASSERT_TRUE(result->entries[0].from_cache);
    TEST_ASSERT_EQUAL_STRING(content, result->entries[0].content);

    chat_context_free_result(result);
}

/* Hash absent from cache (no DB) -> missing */
void test_resolve_missing(void) {
    const char* hashes[1];
    hashes[0] = VALID;

    ContextHashResult* result = chat_context_resolve_hashes(DB, hashes, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(1, result->entry_count);
    TEST_ASSERT_EQUAL_size_t(0, result->hashes_found);
    TEST_ASSERT_EQUAL_size_t(1, result->hashes_missing);
    TEST_ASSERT_EQUAL_size_t(0, result->hashes_found);

    chat_context_free_result(result);
}

/* A NULL hash pointer inside the array skips the entry (no lookup attempted) */
void test_resolve_null_hash_pointer(void) {
    const char* hashes[1];
    hashes[0] = NULL;

    ContextHashResult* result = chat_context_resolve_hashes(DB, hashes, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_size_t(1, result->entry_count);
    TEST_ASSERT_EQUAL_size_t(0, result->hashes_found);
    TEST_ASSERT_EQUAL_size_t(0, result->hashes_missing);

    chat_context_free_result(result);
}

/* Bandwidth saved calculated when content exceeds hash overhead */
void test_resolve_bandwidth_saved(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache(DB);
    TEST_ASSERT_NOT_NULL(cache);
    const char* content = "{\"role\": \"user\", \"content\": \"this is a fairly long message that should exceed the 53 byte hash overhead\"}";
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, VALID, content, strlen(content), false));

    const char* hashes[1];
    hashes[0] = VALID;

    ContextHashResult* result = chat_context_resolve_hashes(DB, hashes, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->bandwidth_saved_bytes > 0.0);
    TEST_ASSERT_TRUE(result->bandwidth_saved_percent > 0.0);

    chat_context_free_result(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_resolve_null_params);
    RUN_TEST(test_resolve_found);
    RUN_TEST(test_resolve_missing);
    RUN_TEST(test_resolve_null_hash_pointer);
    RUN_TEST(test_resolve_bandwidth_saved);
    return UNITY_END();
}
