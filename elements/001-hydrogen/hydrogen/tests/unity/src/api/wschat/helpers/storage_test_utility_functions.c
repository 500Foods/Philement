/*
 * Unity Test File: chat_storage
 * This file contains unit tests for utility functions
 * in src/api/conduit/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage.h>

// Function prototypes
void test_update_access_null_database(void);
void test_update_access_null_segment_hash(void);
void test_update_access_valid_params(void);
void test_free_compressed_null(void);
void test_free_compressed_valid(void);
void test_free_decompressed_null(void);
void test_free_decompressed_valid(void);
void test_free_hash_null(void);
void test_free_hash_valid(void);
void test_cache_init_null_database(void);
void test_cache_shutdown_null_database(void);
void test_get_cache_null_database(void);
void test_cache_get_stats_null_database(void);
void test_cache_get_stats_null_hits(void);
void test_cache_get_stats_null_misses(void);
void test_cache_get_stats_null_ratio(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test update_access with NULL database */
void test_update_access_null_database(void) {
    bool result = chat_storage_update_access(NULL, "hash123");
    TEST_ASSERT_FALSE(result);
}

/* Test update_access with NULL segment_hash */
void test_update_access_null_segment_hash(void) {
    bool result = chat_storage_update_access("testdb", NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test update_access with valid params (currently always returns true) */
void test_update_access_valid_params(void) {
    bool result = chat_storage_update_access("testdb", "hash123");
    TEST_ASSERT_TRUE(result);
}

/* Test free_compressed with NULL */
void test_free_compressed_null(void) {
    // Should not crash
    chat_storage_free_compressed(NULL);
    TEST_ASSERT_TRUE(1); // If we got here, it passed
}

/* Test free_compressed with valid data */
void test_free_compressed_valid(void) {
    uint8_t* data = malloc(10);
    TEST_ASSERT_NOT_NULL(data);
    // Should not crash
    chat_storage_free_compressed(data);
    TEST_ASSERT_TRUE(1);
}

/* Test free_decompressed with NULL */
void test_free_decompressed_null(void) {
    // Should not crash
    chat_storage_free_decompressed(NULL);
    TEST_ASSERT_TRUE(1);
}

/* Test free_decompressed with valid data */
void test_free_decompressed_valid(void) {
    char* data = strdup("test");
    TEST_ASSERT_NOT_NULL(data);
    // Should not crash
    chat_storage_free_decompressed(data);
    TEST_ASSERT_TRUE(1);
}

/* Test free_hash with NULL */
void test_free_hash_null(void) {
    // Should not crash
    chat_storage_free_hash(NULL);
    TEST_ASSERT_TRUE(1);
}

/* Test free_hash with valid data */
void test_free_hash_valid(void) {
    char* hash = strdup("abc123");
    TEST_ASSERT_NOT_NULL(hash);
    // Should not crash
    chat_storage_free_hash(hash);
    TEST_ASSERT_TRUE(1);
}

/* Test cache_init with NULL database */
void test_cache_init_null_database(void) {
    bool result = chat_storage_cache_init(NULL, 0);
    TEST_ASSERT_FALSE(result);
}

/* Test cache_shutdown with NULL database */
void test_cache_shutdown_null_database(void) {
    // Should not crash
    chat_storage_cache_shutdown(NULL);
    TEST_ASSERT_TRUE(1);
}

/* Test get_cache with NULL database */
void test_get_cache_null_database(void) {
    ChatLRUCache* cache = chat_storage_get_cache(NULL);
    TEST_ASSERT_NULL(cache);
}

/* Test cache_get_stats with NULL database */
void test_cache_get_stats_null_database(void) {
    uint64_t hits, misses;
    double ratio;
    bool result = chat_storage_cache_get_stats(NULL, &hits, &misses, &ratio);
    TEST_ASSERT_FALSE(result);
}

/* Test cache_get_stats with NULL hits pointer */
void test_cache_get_stats_null_hits(void) {
    uint64_t misses;
    double ratio;
    bool result = chat_storage_cache_get_stats("testdb", NULL, &misses, &ratio);
    /* May return false if cache not initialized, which is expected */
    TEST_ASSERT_FALSE(result);
}

/* Test cache_get_stats with NULL misses pointer */
void test_cache_get_stats_null_misses(void) {
    uint64_t hits;
    double ratio;
    bool result = chat_storage_cache_get_stats("testdb", &hits, NULL, &ratio);
    TEST_ASSERT_FALSE(result);
}

/* Test cache_get_stats with NULL ratio pointer */
void test_cache_get_stats_null_ratio(void) {
    uint64_t hits, misses;
    bool result = chat_storage_cache_get_stats("testdb", &hits, &misses, NULL);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_access_null_database);
    RUN_TEST(test_update_access_null_segment_hash);
    RUN_TEST(test_update_access_valid_params);

    RUN_TEST(test_free_compressed_null);
    RUN_TEST(test_free_compressed_valid);
    RUN_TEST(test_free_decompressed_null);
    RUN_TEST(test_free_decompressed_valid);
    RUN_TEST(test_free_hash_null);
    RUN_TEST(test_free_hash_valid);

    RUN_TEST(test_cache_init_null_database);
    RUN_TEST(test_cache_shutdown_null_database);
    RUN_TEST(test_get_cache_null_database);

    RUN_TEST(test_cache_get_stats_null_database);
    RUN_TEST(test_cache_get_stats_null_hits);
    RUN_TEST(test_cache_get_stats_null_misses);
    RUN_TEST(test_cache_get_stats_null_ratio);

    return UNITY_END();
}
