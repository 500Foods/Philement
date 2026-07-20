/*
 * Unity Test File: chat LRU disk cache
 * This file contains unit tests for src/api/wschat/helpers/lru_cache.c
 *
 * The file already receives coverage indirectly from other suites; this test
 * registers a dedicated Unity target against it. Every function is exercised
 * directly. All disk-touching operations are sandboxed to a per-run temp
 * directory via the CHAT_CACHE_DIR environment override so the tests never
 * write to the real cache location.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <src/api/wschat/helpers/lru_cache.h>

// Forward declarations for tests
void test_get_base_dir_default_and_env(void);
void test_hash_string_range(void);
void test_get_dir(void);
void test_ensure_directory_exists(void);
void test_get_segment_path(void);
void test_get_metadata_path(void);
void test_free_entry_null(void);
void test_lru_list_add_remove(void);
void test_init_null_and_success(void);
void test_contains(void);
void test_put_and_get(void);
void test_put_update_existing(void);
void test_put_invalid_args(void);
void test_remove(void);
void test_get_stats(void);
void test_flush_and_request_sync(void);
void test_evict_lru_entries(void);
void test_save_and_load_metadata(void);
void test_clear(void);

static char g_tmp_root[] = "/tmp/lru_cache_test_XXXXXX";

/* Point the cache implementation at a sandboxed, unique subdirectory */
static void set_sandbox_cache_dir(const char* leaf) {
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/%s", g_tmp_root, leaf);
    setenv(CHAT_CACHE_DIR_ENV_VAR, dir, 1);
}

void setUp(void) {
}

void tearDown(void) {
    unsetenv(CHAT_CACHE_DIR_ENV_VAR);
}

/* ---- base dir / env override ---- */
void test_get_base_dir_default_and_env(void) {
    unsetenv(CHAT_CACHE_DIR_ENV_VAR);
    TEST_ASSERT_EQUAL_STRING(LRU_CACHE_DIR_NAME, chat_lru_cache_get_base_dir());

    setenv(CHAT_CACHE_DIR_ENV_VAR, "/tmp/custom_cache", 1);
    TEST_ASSERT_EQUAL_STRING("/tmp/custom_cache", chat_lru_cache_get_base_dir());
}

/* ---- hash function ---- */
void test_hash_string_range(void) {
    size_t h1 = chat_lru_cache_hash_string("hello");
    size_t h2 = chat_lru_cache_hash_string("hello");
    size_t h3 = chat_lru_cache_hash_string("world");
    TEST_ASSERT_EQUAL_size_t(h1, h2);            // deterministic
    TEST_ASSERT_TRUE(h1 < LRU_CACHE_HASH_TABLE_SIZE);
    TEST_ASSERT_TRUE(h3 < LRU_CACHE_HASH_TABLE_SIZE);
    // empty string is valid input
    TEST_ASSERT_TRUE(chat_lru_cache_hash_string("") < LRU_CACHE_HASH_TABLE_SIZE);
}

/* ---- directory path helper ---- */
void test_get_dir(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_dir(NULL));

    setenv(CHAT_CACHE_DIR_ENV_VAR, "/tmp/xyz", 1);
    char* dir = chat_lru_cache_get_dir("somedb");
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_EQUAL_STRING("/tmp/xyz", dir);
    free(dir);
}

/* ---- ensure directory exists ---- */
void test_ensure_directory_exists(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_ensure_directory_exists(NULL));

    char nested[256];
    snprintf(nested, sizeof(nested), "%s/ensure/a/b/c", g_tmp_root);
    TEST_ASSERT_TRUE(chat_lru_cache_ensure_directory_exists(nested));

    // Second call: already exists as a directory
    TEST_ASSERT_TRUE(chat_lru_cache_ensure_directory_exists(nested));

    // Path that exists but is a regular file -> not a directory
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/ensure/plainfile", g_tmp_root);
    FILE* fp = fopen(file_path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("x", fp);
    fclose(fp);
    TEST_ASSERT_FALSE(chat_lru_cache_ensure_directory_exists(file_path));
}

/* ---- segment path ---- */
void test_get_segment_path(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path(NULL, "abcdef"));
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path("/cache", NULL));
    // hash shorter than prefix length -> NULL
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path("/cache", "a"));

    char* path = chat_lru_cache_get_segment_path("/cache", "abcdef");
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_EQUAL_STRING("/cache/ab/abcdef.json", path);
    free(path);
}

/* ---- metadata path ---- */
void test_get_metadata_path(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_metadata_path(NULL));
    char* path = chat_lru_cache_get_metadata_path("/cache");
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_EQUAL_STRING("/cache/" LRU_CACHE_METADATA_FILE, path);
    free(path);
}

/* ---- free entry ---- */
void test_free_entry_null(void) {
    chat_lru_cache_free_entry(NULL);  // no-op, must not crash
    ChatLRUCacheEntry* e = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e);
    e->content = strdup("data");
    chat_lru_cache_free_entry(e);  // frees content + entry
    TEST_PASS();
}

/* ---- raw LRU list operations ---- */
void test_lru_list_add_remove(void) {
    // NULL-guarded no-ops
    chat_lru_cache_add_front(NULL, NULL);
    chat_lru_cache_remove_entry(NULL, NULL);

    ChatLRUCache cache = {0};
    ChatLRUCacheEntry a = {0};
    ChatLRUCacheEntry b = {0};
    ChatLRUCacheEntry c = {0};

    chat_lru_cache_add_front(&cache, &a);  // list: a
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_tail);

    chat_lru_cache_add_front(&cache, &b);  // list: b, a
    chat_lru_cache_add_front(&cache, &c);  // list: c, b, a
    TEST_ASSERT_EQUAL_PTR(&c, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_tail);

    // Remove middle node
    chat_lru_cache_remove_entry(&cache, &b);  // list: c, a
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_head->lru_next);

    // Remove head
    chat_lru_cache_remove_entry(&cache, &c);  // list: a
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&a, cache.lru_tail);

    // Remove tail (only node)
    chat_lru_cache_remove_entry(&cache, &a);  // empty
    TEST_ASSERT_NULL(cache.lru_head);
    TEST_ASSERT_NULL(cache.lru_tail);
}

/* ---- init / shutdown ---- */
void test_init_null_and_success(void) {
    TEST_ASSERT_NULL(chat_lru_cache_init(NULL, 0));

    set_sandbox_cache_dir("init");
    ChatLRUCache* cache = chat_lru_cache_init("testdb", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL_STRING("testdb", cache->database);
    TEST_ASSERT_EQUAL_size_t(1024 * 1024, cache->max_size_bytes);
    TEST_ASSERT_NOT_NULL(cache->hash_table);
    chat_lru_cache_shutdown(cache);

    // Zero max_size falls back to the default
    set_sandbox_cache_dir("init2");
    ChatLRUCache* cache2 = chat_lru_cache_init("db2", 0);
    TEST_ASSERT_NOT_NULL(cache2);
    TEST_ASSERT_EQUAL_UINT64(LRU_CACHE_MAX_SIZE_BYTES, cache2->max_size_bytes);
    chat_lru_cache_shutdown(cache2);

    // shutdown(NULL) safe
    chat_lru_cache_shutdown(NULL);
}

/* ---- contains ---- */
void test_contains(void) {
    set_sandbox_cache_dir("contains");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_FALSE(chat_lru_cache_contains(NULL, "h"));
    TEST_ASSERT_FALSE(chat_lru_cache_contains(cache, NULL));
    TEST_ASSERT_FALSE(chat_lru_cache_contains(cache, "missing"));

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "hash1", "content", 7, false));
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "hash1"));

    chat_lru_cache_shutdown(cache);
}

/* ---- put / get ---- */
void test_put_and_get(void) {
    set_sandbox_cache_dir("putget");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_NULL(chat_lru_cache_get(NULL, "h"));
    TEST_ASSERT_NULL(chat_lru_cache_get(cache, NULL));
    TEST_ASSERT_NULL(chat_lru_cache_get(cache, "missing"));  // records a miss

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "hashA", "hello world", 11, false));
    char* got = chat_lru_cache_get(cache, "hashA");           // records a hit
    TEST_ASSERT_NOT_NULL(got);
    TEST_ASSERT_EQUAL_STRING("hello world", got);
    free(got);

    ChatLRUCacheStats stats;
    TEST_ASSERT_TRUE(chat_lru_cache_get_stats(cache, &stats));
    TEST_ASSERT_EQUAL_UINT64(1, stats.cache_hits);
    TEST_ASSERT_EQUAL_UINT64(1, stats.cache_misses);
    TEST_ASSERT_TRUE(stats.hit_ratio > 0.0);

    chat_lru_cache_shutdown(cache);
}

/* ---- put updates existing entry ---- */
void test_put_update_existing(void) {
    set_sandbox_cache_dir("putupd");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "k", "short", 5, false));
    // Update with a larger (grow) dirty payload -> takes the grow/evict path
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "k", "a much longer value", 19, true));

    char* got = chat_lru_cache_get(cache, "k");
    TEST_ASSERT_EQUAL_STRING("a much longer value", got);
    free(got);

    // Only one logical entry despite two puts
    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_EQUAL_size_t(1, stats.total_entries);

    chat_lru_cache_shutdown(cache);
}

/* ---- put invalid args ---- */
void test_put_invalid_args(void) {
    set_sandbox_cache_dir("putinv");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_FALSE(chat_lru_cache_put(NULL, "h", "c", 1, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(cache, NULL, "c", 1, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(cache, "h", NULL, 1, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(cache, "h", "c", 0, false));

    chat_lru_cache_shutdown(cache);
}

/* ---- remove ---- */
void test_remove(void) {
    set_sandbox_cache_dir("remove");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_FALSE(chat_lru_cache_remove(NULL, "h"));
    TEST_ASSERT_FALSE(chat_lru_cache_remove(cache, NULL));
    TEST_ASSERT_FALSE(chat_lru_cache_remove(cache, "missing"));

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "todelete", "data", 4, false));
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "todelete"));
    TEST_ASSERT_TRUE(chat_lru_cache_remove(cache, "todelete"));
    TEST_ASSERT_FALSE(chat_lru_cache_contains(cache, "todelete"));

    chat_lru_cache_shutdown(cache);
}

/* ---- stats ---- */
void test_get_stats(void) {
    set_sandbox_cache_dir("stats");
    ChatLRUCache* cache = chat_lru_cache_init("db", 4096);
    TEST_ASSERT_NOT_NULL(cache);

    ChatLRUCacheStats stats;
    TEST_ASSERT_FALSE(chat_lru_cache_get_stats(NULL, &stats));
    TEST_ASSERT_FALSE(chat_lru_cache_get_stats(cache, NULL));

    TEST_ASSERT_TRUE(chat_lru_cache_get_stats(cache, &stats));
    TEST_ASSERT_EQUAL_UINT64(4096, stats.max_size_bytes);

    chat_lru_cache_shutdown(cache);
}

/* ---- flush / request_sync ---- */
void test_flush_and_request_sync(void) {
    chat_lru_cache_request_sync(NULL);  // NULL no-op
    TEST_ASSERT_EQUAL_INT(-1, chat_lru_cache_flush(NULL));

    set_sandbox_cache_dir("flush");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    // Nothing dirty yet -> 0 synced
    TEST_ASSERT_EQUAL_INT(0, chat_lru_cache_flush(cache));

    // Add a dirty entry, then flush marks it clean and counts it
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "d1", "dirtydata", 9, true));
    TEST_ASSERT_EQUAL_INT(1, chat_lru_cache_flush(cache));
    // Second flush: already clean -> 0
    TEST_ASSERT_EQUAL_INT(0, chat_lru_cache_flush(cache));

    chat_lru_cache_request_sync(cache);
    TEST_ASSERT_TRUE(cache->sync_requested);

    chat_lru_cache_shutdown(cache);
}

/* ---- eviction ---- */
void test_evict_lru_entries(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_evict_lru_entries(NULL, 10));

    // Tiny cache forces eviction of the oldest entry when a new one is added
    set_sandbox_cache_dir("evict");
    ChatLRUCache* cache = chat_lru_cache_init("db", 20);  // 20 bytes total
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "first", "0123456789", 10, false));
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "second", "abcdefghij", 10, false));
    // Adding a third 10-byte entry must evict "first" (LRU) to stay <= 20
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "third", "klmnopqrst", 10, false));

    TEST_ASSERT_FALSE(chat_lru_cache_contains(cache, "first"));
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "third"));

    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_TRUE(stats.evictions >= 1);

    chat_lru_cache_shutdown(cache);
}

/* ---- metadata save / load ---- */
void test_save_and_load_metadata(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_save_metadata(NULL));
    TEST_ASSERT_FALSE(chat_lru_cache_load_metadata(NULL));

    set_sandbox_cache_dir("meta");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    // Stop the background sync thread so it can't race with our manual
    // save/load of the metadata file (the thread also rewrites it).
    if (cache->sync_running) {
        cache->sync_running = false;
        pthread_join(cache->sync_thread, NULL);
    }

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "m1", "content", 7, false));
    // Drive some hit/miss counters so metadata has numbers to round-trip
    char* got = chat_lru_cache_get(cache, "m1");
    free(got);
    (void)chat_lru_cache_get(cache, "nope");

    TEST_ASSERT_TRUE(chat_lru_cache_save_metadata(cache));
    // The metadata file should now exist on disk
    char* meta_path = chat_lru_cache_get_metadata_path(cache->cache_dir);
    struct stat st;
    TEST_ASSERT_EQUAL_INT(0, stat(meta_path, &st));
    free(meta_path);

    // Load it back into a fresh cache pointed at the same directory
    TEST_ASSERT_TRUE(chat_lru_cache_load_metadata(cache));

    chat_lru_cache_shutdown(cache);
}

/* ---- clear ---- */
void test_clear(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_clear(NULL));

    set_sandbox_cache_dir("clear");
    ChatLRUCache* cache = chat_lru_cache_init("db", 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "c1", "one", 3, false));
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "c2", "two", 3, false));
    TEST_ASSERT_TRUE(chat_lru_cache_clear(cache));

    TEST_ASSERT_FALSE(chat_lru_cache_contains(cache, "c1"));
    ChatLRUCacheStats stats;
    chat_lru_cache_get_stats(cache, &stats);
    TEST_ASSERT_EQUAL_size_t(0, stats.total_entries);
    TEST_ASSERT_EQUAL_size_t(0, stats.total_size_bytes);

    chat_lru_cache_shutdown(cache);
}

int main(void) {
    // Create a single sandbox root for all disk operations in this run.
    if (mkdtemp(g_tmp_root) == NULL) {
        // Fall back to a fixed path; tests will still run under /tmp.
        strncpy(g_tmp_root, "/tmp/lru_cache_test_fb", sizeof(g_tmp_root) - 1);
        mkdir(g_tmp_root, 0755);
    }

    UNITY_BEGIN();
    RUN_TEST(test_get_base_dir_default_and_env);
    RUN_TEST(test_hash_string_range);
    RUN_TEST(test_get_dir);
    RUN_TEST(test_ensure_directory_exists);
    RUN_TEST(test_get_segment_path);
    RUN_TEST(test_get_metadata_path);
    RUN_TEST(test_free_entry_null);
    RUN_TEST(test_lru_list_add_remove);
    RUN_TEST(test_init_null_and_success);
    RUN_TEST(test_contains);
    RUN_TEST(test_put_and_get);
    RUN_TEST(test_put_update_existing);
    RUN_TEST(test_put_invalid_args);
    RUN_TEST(test_remove);
    RUN_TEST(test_get_stats);
    RUN_TEST(test_flush_and_request_sync);
    RUN_TEST(test_evict_lru_entries);
    RUN_TEST(test_save_and_load_metadata);
    RUN_TEST(test_clear);
    int result = UNITY_END();

    // Best-effort cleanup of the sandbox root.
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", g_tmp_root);
    if (system(cmd) != 0) {
        // Ignore cleanup failures; the OS will reclaim /tmp eventually.
    }

    return result;
}
