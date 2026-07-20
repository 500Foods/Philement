/*
 * Unity Test File: chat_storage_cache_management
 * This file contains unit tests for the LRU cache management entry points
 * in src/api/wschat/helpers/storage.c:
 *   - chat_storage_cache_init
 *   - chat_storage_get_cache
 *   - chat_storage_cache_shutdown
 *   - chat_storage_cache_get_stats
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/storage.h>

void setUp(void) {
    chat_storage_cache_shutdown("testdb");
}

void tearDown(void) {
    chat_storage_cache_shutdown("testdb");
}

/* cache_init with valid database creates a cache */
static void test_cache_init_valid(void) {
    TEST_ASSERT_TRUE(chat_storage_cache_init("testdb", 0));
    TEST_ASSERT_NOT_NULL(chat_storage_get_cache("testdb"));
}

/* cache_init returns true if cache already exists */
static void test_cache_init_already_exists(void) {
    TEST_ASSERT_TRUE(chat_storage_cache_init("testdb", 0));
    TEST_ASSERT_TRUE(chat_storage_cache_init("testdb", 0));
}

/* get_cache returns NULL for unknown database */
static void test_get_cache_unknown(void) {
    TEST_ASSERT_NULL(chat_storage_get_cache("doesnotexist"));
}

/* cache_get_stats returns valid stats for an initialized cache */
static void test_cache_get_stats_valid(void) {
    TEST_ASSERT_TRUE(chat_storage_cache_init("testdb", 0));

    uint64_t hits = 0, misses = 0;
    double ratio = -1.0;
    bool result = chat_storage_cache_get_stats("testdb", &hits, &misses, &ratio);
    TEST_ASSERT_TRUE(result);
    /* hit_ratio should be a finite number in [0,1] */
    TEST_ASSERT_TRUE(ratio >= 0.0 && ratio <= 1.0);
}

/* cache_get_stats unknown database returns false */
static void test_cache_get_stats_unknown(void) {
    uint64_t hits = 0, misses = 0;
    double ratio = -1.0;
    TEST_ASSERT_FALSE(chat_storage_cache_get_stats("doesnotexist", &hits, &misses, &ratio));
}

/* shutdown removes the cache so get_cache returns NULL afterwards */
static void test_cache_shutdown_removes(void) {
    TEST_ASSERT_TRUE(chat_storage_cache_init("testdb", 0));
    TEST_ASSERT_NOT_NULL(chat_storage_get_cache("testdb"));
    chat_storage_cache_shutdown("testdb");
    TEST_ASSERT_NULL(chat_storage_get_cache("testdb"));
}

/* shutdown is a no-op for unknown database (does not crash) */
static void test_cache_shutdown_unknown(void) {
    chat_storage_cache_shutdown("doesnotexist");
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_init_valid);
    RUN_TEST(test_cache_init_already_exists);
    RUN_TEST(test_get_cache_unknown);
    RUN_TEST(test_cache_get_stats_valid);
    RUN_TEST(test_cache_get_stats_unknown);
    RUN_TEST(test_cache_shutdown_removes);
    RUN_TEST(test_cache_shutdown_unknown);
    return UNITY_END();
}
