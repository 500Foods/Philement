/*
 * Unity Test File: chat_storage_get_or_create_cache
 * This file contains unit tests for chat_storage_get_or_create_cache()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>

/* Function prototype (exposed via header comment in storage.h) */
ChatLRUCache* chat_storage_get_or_create_cache(const char* database);

void setUp(void) {
    /* Best-effort: shutdown any lingering test caches */
    chat_storage_cache_shutdown("testdb");
    chat_storage_cache_shutdown("testdb2");
    chat_storage_cache_shutdown("full1");
}

void tearDown(void) {
    chat_storage_cache_shutdown("testdb");
    chat_storage_cache_shutdown("testdb2");
    chat_storage_cache_shutdown("full1");
}

/* NULL database returns NULL */
static void test_get_or_create_cache_null_database(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache(NULL);
    TEST_ASSERT_NULL(cache);
}

/* Returns an existing cache when called twice for same database */
static void test_get_or_create_cache_existing(void) {
    ChatLRUCache* c1 = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(c1);
    ChatLRUCache* c2 = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_EQUAL_PTR(c1, c2);
    TEST_ASSERT_NOT_NULL(chat_storage_get_cache("testdb"));
}

/* Creates a new cache for a new database */
static void test_get_or_create_cache_create_new(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb2");
    TEST_ASSERT_NOT_NULL(cache);
}

/* When the cache table is full, a new database cannot get a cache (NULL) */
static void test_get_or_create_cache_full(void) {
    /* MAX_CACHED_DATABASES is 32; fill it with distinct databases */
    char name[32];
    for (int i = 0; i < 32; i++) {
        snprintf(name, sizeof(name), "full_%d", i);
        ChatLRUCache* c = chat_storage_get_or_create_cache(name);
        TEST_ASSERT_NOT_NULL(c);
    }
    /* A 33rd distinct database should not get a cache */
    ChatLRUCache* overflow = chat_storage_get_or_create_cache("overflow_db");
    TEST_ASSERT_NULL(overflow);

    for (int i = 0; i < 32; i++) {
        snprintf(name, sizeof(name), "full_%d", i);
        chat_storage_cache_shutdown(name);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_or_create_cache_null_database);
    RUN_TEST(test_get_or_create_cache_existing);
    RUN_TEST(test_get_or_create_cache_create_new);
    RUN_TEST(test_get_or_create_cache_full);
    return UNITY_END();
}
