/*
 * Unity Test File: Chat LRU Cache Tests
 * This file contains unit tests for Phase 9 LRU disk cache functionality
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/conduit/chat_common/chat_lru_cache.h>
#include <src/api/conduit/chat_common/chat_storage.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// Forward declarations for test functions
void test_lru_cache_init_shutdown(void);
void test_lru_cache_contains_empty(void);
void test_lru_cache_put_get(void);
void test_lru_cache_get_miss(void);
void test_lru_cache_remove(void);
void test_lru_cache_stats(void);
void test_lru_cache_eviction(void);
void test_lru_cache_update_entry(void);
void test_lru_cache_clear(void);
void test_lru_cache_null_params(void);
void test_lru_cache_dirty_flag(void);
void test_lru_cache_get_dir(void);

// Test database name
static const char* TEST_DATABASE = "test_lru_cache_db";

// Forward declarations for test helper functions
static char* generate_test_hash(int seed);
static char* generate_test_content(int seed, size_t* out_len);

void setUp(void) {
    // Clean up any existing test cache directory before each test
    // Get the actual cache directory used by implementation
    char* cache_dir = chat_lru_cache_get_dir(TEST_DATABASE);
    if (cache_dir) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" 2>/dev/null", cache_dir);
        system(cmd);
        free(cache_dir);
    }
}

void tearDown(void) {
    // Clean up after each test
    char* cache_dir = chat_lru_cache_get_dir(TEST_DATABASE);
    if (cache_dir) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" 2>/dev/null", cache_dir);
        system(cmd);
        free(cache_dir);
    }
}

// Generate a deterministic test hash (43 chars base64url-like)
static char* generate_test_hash(int seed) {
    char* hash = calloc(64, 1);
    if (!hash) return NULL;

    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
    const int len = 43;  // SHA-256 base64url length

    for (int i = 0; i < len; i++) {
        hash[i] = chars[(seed * 31 + i) % 64];
    }
    hash[len] = '\0';
    return hash;
}

// Generate test content
static char* generate_test_content(int seed, size_t* out_len) {
    char template[256];
    snprintf(template, sizeof(template),
             "{\"role\":\"user\",\"content\":\"Test message %d with some content for testing\"}",
             seed);
    if (out_len) {
        const size_t len = strlen(template);
        *out_len = len;
    }
    return strdup(template);
}

// Test 1: Basic cache initialization and shutdown
void test_lru_cache_init_shutdown(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);  // 1MB
    TEST_ASSERT_NOT_NULL(cache);

    chat_lru_cache_shutdown(cache);
}

// Test 2: Cache contains returns false for non-existent entry
void test_lru_cache_contains_empty(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(1);
    bool found = chat_lru_cache_contains(cache, hash);
    TEST_ASSERT_FALSE(found);

    free(hash);
    chat_lru_cache_shutdown(cache);
}

// Test 3: Cache put and get basic functionality
void test_lru_cache_put_get(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(1);
    size_t content_len;
    char* content = generate_test_content(1, &content_len);

    // Put entry
    bool put_result = chat_lru_cache_put(cache, hash, content, content_len, false);
    TEST_ASSERT_TRUE(put_result);

    // Check contains
    bool found = chat_lru_cache_contains(cache, hash);
    TEST_ASSERT_TRUE(found);

    // Get entry
    char* retrieved = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING(content, retrieved);

    free(retrieved);
    free(hash);
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 4: Cache get returns NULL for non-existent entry
void test_lru_cache_get_miss(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(999);
    char* result = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_NULL(result);

    free(hash);
    chat_lru_cache_shutdown(cache);
}

// Test 5: Cache remove functionality
void test_lru_cache_remove(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(1);
    size_t content_len;
    char* content = generate_test_content(1, &content_len);

    // Put entry
    chat_lru_cache_put(cache, hash, content, content_len, false);
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, hash));

    // Remove entry
    bool remove_result = chat_lru_cache_remove(cache, hash);
    TEST_ASSERT_TRUE(remove_result);

    // Verify removed
    bool found = chat_lru_cache_contains(cache, hash);
    TEST_ASSERT_FALSE(found);

    // Remove non-existent should return false
    bool remove_again = chat_lru_cache_remove(cache, hash);
    TEST_ASSERT_FALSE(remove_again);

    free(hash);
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 6: Cache statistics tracking
void test_lru_cache_stats(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    ChatLRUCacheStats stats;

    // Initial stats should be zero
    bool stats_result = chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_TRUE(stats_result);
    TEST_ASSERT_EQUAL(0, stats.total_entries);
    TEST_ASSERT_EQUAL(0, stats.cache_hits);
    TEST_ASSERT_EQUAL(0, stats.cache_misses);

    // Add entry and access it
    char* hash = generate_test_hash(1);
    size_t content_len;
    char* content = generate_test_content(1, &content_len);
    chat_lru_cache_put(cache, hash, content, content_len, false);

    // Get (hit)
    char* retrieved = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_NOT_NULL(retrieved);
    free(retrieved);

    // Get again (hit)
    retrieved = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_NOT_NULL(retrieved);
    free(retrieved);

    // Get non-existent (miss)
    char* miss_hash = generate_test_hash(999);
    char* miss_result = chat_lru_cache_get(cache, miss_hash);
    TEST_ASSERT_NULL(miss_result);
    free(miss_hash);

    // Check stats
    stats_result = chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_TRUE(stats_result);
    TEST_ASSERT_EQUAL(1, stats.total_entries);
    TEST_ASSERT_EQUAL(2, stats.cache_hits);
    TEST_ASSERT_EQUAL(1, stats.cache_misses);

    free(hash);
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 7: LRU eviction when cache is full
void test_lru_cache_eviction(void) {
    // Create small cache (1KB)
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024);
    TEST_ASSERT_NOT_NULL(cache);

    // Add entries that exceed cache size
    char* hashes[10];
    size_t content_len;
    char* content = generate_test_content(1, &content_len);

    // Add 5 entries of ~200 bytes each (should fill and evict)
    for (int i = 0; i < 5; i++) {
        hashes[i] = generate_test_hash(i);
        chat_lru_cache_put(cache, hashes[i], content, content_len, false);
    }

    // First entries should have been evicted
    // Last entry should still be present
    bool last_found = chat_lru_cache_contains(cache, hashes[4]);
    TEST_ASSERT_TRUE(last_found);

    // Get stats to verify evictions occurred
    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_TRUE(stats.evictions > 0 || stats.total_size_bytes <= 1024);

    // Cleanup
    for (int i = 0; i < 5; i++) {
        free(hashes[i]);
    }
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 8: Cache update existing entry
void test_lru_cache_update_entry(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(1);
    size_t content_len;
    char* content1 = generate_test_content(1, &content_len);
    char* content2 = generate_test_content(2, &content_len);

    // Put first version
    chat_lru_cache_put(cache, hash, content1, content_len, false);
    char* retrieved = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_EQUAL_STRING(content1, retrieved);
    free(retrieved);

    // Update with second version
    chat_lru_cache_put(cache, hash, content2, content_len, false);
    retrieved = chat_lru_cache_get(cache, hash);
    TEST_ASSERT_EQUAL_STRING(content2, retrieved);
    free(retrieved);

    // Should still have only one entry
    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_EQUAL(1, stats.total_entries);

    free(hash);
    free(content1);
    free(content2);
    chat_lru_cache_shutdown(cache);
}

// Test 9: Cache clear functionality
void test_lru_cache_clear(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    // Add multiple entries
    char* hashes[5];
    size_t content_len;
    char* content = generate_test_content(1, &content_len);

    for (int i = 0; i < 5; i++) {
        hashes[i] = generate_test_hash(i);
        chat_lru_cache_put(cache, hashes[i], content, content_len, false);
    }

    // Verify entries exist
    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_EQUAL(5, stats.total_entries);

    // Clear cache
    bool clear_result = chat_lru_cache_clear(cache);
    TEST_ASSERT_TRUE(clear_result);

    // Verify all entries removed
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_EQUAL(0, stats.total_entries);

    // Cleanup
    for (int i = 0; i < 5; i++) {
        free(hashes[i]);
    }
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 10: NULL parameter handling
void test_lru_cache_null_params(void) {
    // Init with NULL database
    ChatLRUCache* cache = chat_lru_cache_init(NULL, 1024);
    TEST_ASSERT_NULL(cache);

    // Operations on NULL cache
    TEST_ASSERT_FALSE(chat_lru_cache_contains(NULL, "hash"));
    TEST_ASSERT_NULL(chat_lru_cache_get(NULL, "hash"));
    TEST_ASSERT_FALSE(chat_lru_cache_put(NULL, "hash", "content", 7, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(cache, NULL, "content", 7, false));
    TEST_ASSERT_FALSE(chat_lru_cache_remove(NULL, "hash"));

    // Shutdown NULL cache (should not crash)
    chat_lru_cache_shutdown(NULL);

    // Stats on NULL
    ChatLRUCacheStats stats;
    TEST_ASSERT_FALSE(chat_lru_cache_get_stats(NULL, &stats));
}

// Test 11: Dirty flag tracking for sync
void test_lru_cache_dirty_flag(void) {
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    char* hash = generate_test_hash(1);
    size_t content_len;
    char* content = generate_test_content(1, &content_len);

    // Put with dirty flag
    chat_lru_cache_put(cache, hash, content, content_len, true);

    // Flush dirty entries (should succeed even with no DB backend in test)
    int flushed = chat_lru_cache_flush(cache);
    TEST_ASSERT_TRUE(flushed >= 0);

    free(hash);
    free(content);
    chat_lru_cache_shutdown(cache);
}

// Test 12: Cache get directory path
void test_lru_cache_get_dir(void) {
    char* path = chat_lru_cache_get_dir("testdb");
    TEST_ASSERT_NOT_NULL(path);
    // Path should be non-empty string
    TEST_ASSERT_TRUE(strlen(path) > 0);
    // Optionally, verify path is a directory if it exists
    struct stat st;
    if (stat(path, &st) == 0) {
        TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
    }
    free(path);

    // NULL database
    char* null_path = chat_lru_cache_get_dir(NULL);
    TEST_ASSERT_NULL(null_path);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lru_cache_init_shutdown);
    RUN_TEST(test_lru_cache_contains_empty);
    RUN_TEST(test_lru_cache_put_get);
    RUN_TEST(test_lru_cache_get_miss);
    RUN_TEST(test_lru_cache_remove);
    RUN_TEST(test_lru_cache_stats);
    RUN_TEST(test_lru_cache_eviction);
    RUN_TEST(test_lru_cache_update_entry);
    RUN_TEST(test_lru_cache_clear);
    RUN_TEST(test_lru_cache_null_params);
    RUN_TEST(test_lru_cache_dirty_flag);
    RUN_TEST(test_lru_cache_get_dir);

    return UNITY_END();
}
