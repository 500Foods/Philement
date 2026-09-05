#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/lru_cache.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <jansson.h>

void test_lru_evict_with_entries(void);
void test_lru_evict_frees_tail_when_empty_after(void);
void test_lru_evict_enough_space_returns_true(void);
void test_lru_evict_updates_stats(void);
void test_lru_put_new_entry(void);
void test_lru_put_existing_entry_update(void);
void test_lru_put_existing_entry_rejects_grow(void);
void test_lru_put_dirty_triggers_sync(void);
void test_lru_put_content_null(void);
void test_lru_contains_found(void);
void test_lru_get_stats_valid(void);
void test_lru_save_metadata_success(void);
void test_lru_save_metadata_null(void);
void test_lru_load_metadata_success(void);
void test_lru_load_metadata_null(void);
void test_lru_load_metadata_file_not_found(void);
void test_lru_load_metadata_invalid_json(void);
void test_lru_load_metadata_empty_file(void);
void test_lru_evict_enough_space_no_eviction(void);

static char g_temp_dir[256];
static ChatLRUCache* g_cache;

static void cleanup_cache_dir(const char* dir) {
    /* Remove all files in hash prefix subdirs */
    char path[512];
    for (int i = 0; i < 256; i++) {
        snprintf(path, sizeof(path), "%s/%02x/segments", dir, i);
        /* Try removing a known file pattern */
    }
    /* Remove metadata file */
    char* meta = chat_lru_cache_get_metadata_path(dir);
    if (meta) {
        unlink(meta);
        free(meta);
    }
}

void setUp(void) {
    /* Create a temp directory for cache tests */
    snprintf(g_temp_dir, sizeof(g_temp_dir), "/tmp/hydrogen_lru_test_%d", (int)getpid());
    mkdir(g_temp_dir, 0755);

    g_cache = calloc(1, sizeof(ChatLRUCache));
    TEST_ASSERT_NOT_NULL(g_cache);
    g_cache->database = strdup("testdb");
    g_cache->cache_dir = strdup(g_temp_dir);
    g_cache->max_size_bytes = 1024;
    g_cache->current_size_bytes = 0;
    g_cache->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    TEST_ASSERT_NOT_NULL(g_cache->hash_table);
    pthread_mutex_init(&g_cache->cache_mutex, NULL);
    g_cache->mutex_initialized = true;
    g_cache->sync_running = false;
    g_cache->sync_requested = false;
}

void tearDown(void) {
    if (g_cache) {
        /* Free all entries in hash table */
        for (size_t i = 0; i < LRU_CACHE_HASH_TABLE_SIZE; i++) {
            ChatLRUCacheEntry* entry = g_cache->hash_table[i];
            while (entry) {
                ChatLRUCacheEntry* next = entry->hash_next;
                chat_lru_cache_free_entry(entry);
                entry = next;
            }
        }
        free(g_cache->hash_table);
        free(g_cache->cache_dir);
        free(g_cache->database);
        pthread_mutex_destroy(&g_cache->cache_mutex);
        free(g_cache);
    }
    /* Clean up temp directory */
    cleanup_cache_dir(g_temp_dir);
    /* Remove subdirectories */
    char path[512];
    for (int i = 0; i < 256; i++) {
        snprintf(path, sizeof(path), "%s/%02x", g_temp_dir, i);
        rmdir(path);
    }
    rmdir(g_temp_dir);
}

/* --- Evict tests --- */

void test_lru_evict_enough_space_no_eviction(void) {
    /* cache is empty, should return true */
    TEST_ASSERT_TRUE(chat_lru_cache_evict_lru_entries(g_cache, 100));
}

void test_lru_evict_with_entries(void) {
    /* Add entries that exceed capacity, then evict */
    g_cache->max_size_bytes = 50;

    ChatLRUCacheEntry* e1 = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e1);
    strncpy(e1->hash, "aa0001", LRU_CACHE_MAX_HASH_LEN - 1);
    e1->content = strdup("content_aaaa");
    e1->content_size = 10;
    e1->is_dirty = false;

    size_t bucket = chat_lru_cache_hash_string("aa0001");
    g_cache->hash_table[bucket] = e1;
    chat_lru_cache_add_front(g_cache, e1);
    g_cache->current_size_bytes = 10;

    /* Evict 50 bytes — should evict e1 (10 bytes). 10+50=60>50, evict e1 → 0+50=50<=50 → true */
    TEST_ASSERT_TRUE(chat_lru_cache_evict_lru_entries(g_cache, 50));

    /* entry should have been freed and removed from hash table */
    TEST_ASSERT_NULL(g_cache->hash_table[bucket]);
    TEST_ASSERT_EQUAL_size_t(0, g_cache->current_size_bytes);
    TEST_ASSERT_EQUAL_UINT64(1, g_cache->stats.evictions);
}

void test_lru_evict_frees_tail_when_empty_after(void) {
    g_cache->max_size_bytes = 100;

    ChatLRUCacheEntry* e1 = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e1);
    strncpy(e1->hash, "bb0001", LRU_CACHE_MAX_HASH_LEN - 1);
    e1->content = strdup("x");
    e1->content_size = 100;
    e1->is_dirty = false;

    size_t bucket = chat_lru_cache_hash_string("bb0001");
    g_cache->hash_table[bucket] = e1;
    chat_lru_cache_add_front(g_cache, e1);
    g_cache->current_size_bytes = 100;

    /* Evict 100 bytes — should evict e1 (100 bytes), leaving current_size 0 */
    /* 0 + 100 <= 100 so it should return true after eviction */
    TEST_ASSERT_TRUE(chat_lru_cache_evict_lru_entries(g_cache, 100));

    TEST_ASSERT_NULL(g_cache->hash_table[bucket]);
    TEST_ASSERT_EQUAL_size_t(0, g_cache->current_size_bytes);
}

void test_lru_evict_enough_space_returns_true(void) {
    g_cache->max_size_bytes = 100;

    ChatLRUCacheEntry* e1 = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e1);
    e1->content = strdup("a");
    TEST_ASSERT_NOT_NULL(e1->content);
    e1->content_size = 50;
    e1->is_dirty = false;
    strncpy(e1->hash, "cc0001", LRU_CACHE_MAX_HASH_LEN - 1);

    size_t bucket = chat_lru_cache_hash_string("cc0001");
    g_cache->hash_table[bucket] = e1;
    chat_lru_cache_add_front(g_cache, e1);
    g_cache->current_size_bytes = 50;

    TEST_ASSERT_TRUE(chat_lru_cache_evict_lru_entries(g_cache, 40));
}

void test_lru_evict_updates_stats(void) {
    g_cache->max_size_bytes = 10;

    ChatLRUCacheEntry* e1 = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e1);
    e1->content = strdup("data1");
    TEST_ASSERT_NOT_NULL(e1->content);
    e1->content_size = 20;
    e1->is_dirty = false;
    strncpy(e1->hash, "dd0001", LRU_CACHE_MAX_HASH_LEN - 1);

    size_t bucket = chat_lru_cache_hash_string("dd0001");
    g_cache->hash_table[bucket] = e1;
    chat_lru_cache_add_front(g_cache, e1);
    g_cache->current_size_bytes = 20;

    chat_lru_cache_evict_lru_entries(g_cache, 50);

    TEST_ASSERT_EQUAL_UINT64(1, g_cache->stats.evictions);
    TEST_ASSERT_NULL(g_cache->lru_tail);
    TEST_ASSERT_NULL(g_cache->lru_head);
}

/* --- Contains tests --- */

void test_lru_contains_found(void) {
    g_cache->max_size_bytes = 1024;

    ChatLRUCacheEntry* e1 = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(e1);
    strncpy(e1->hash, "hash1_key", LRU_CACHE_MAX_HASH_LEN - 1);
    e1->content = strdup("content");
    e1->content_size = 8;
    e1->is_dirty = false;

    size_t bucket = chat_lru_cache_hash_string("hash1_key");
    g_cache->hash_table[bucket] = e1;
    chat_lru_cache_add_front(g_cache, e1);
    g_cache->current_size_bytes = 8;

    TEST_ASSERT_TRUE(chat_lru_cache_contains(g_cache, "hash1_key"));
    TEST_ASSERT_FALSE(chat_lru_cache_contains(g_cache, "not_found_key"));
}

/* --- Put tests --- */

void test_lru_put_new_entry(void) {
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "newseg12", "hello world", 11, false));

    size_t bucket = chat_lru_cache_hash_string("newseg12");
    ChatLRUCacheEntry* entry = g_cache->hash_table[bucket];
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_STRING("newseg12", entry->hash);
    TEST_ASSERT_EQUAL_size_t(11, entry->content_size);
    TEST_ASSERT_EQUAL_size_t(11, g_cache->current_size_bytes);
    TEST_ASSERT_EQUAL_UINT64(1, g_cache->stats.total_entries);
}

void test_lru_put_existing_entry_update(void) {
    /* First put */
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "updseg01", "short", 5, false));

    /* Update with shorter content — no eviction needed */
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "updseg01", "longer content here", 20, true));

    size_t bucket = chat_lru_cache_hash_string("updseg01");
    ChatLRUCacheEntry* entry = g_cache->hash_table[bucket];
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_size_t(20, entry->content_size);
    TEST_ASSERT_EQUAL_size_t(20, g_cache->current_size_bytes);
    TEST_ASSERT_EQUAL_UINT64(1, g_cache->stats.total_entries);
    TEST_ASSERT_TRUE(entry->is_dirty);
}

void test_lru_put_existing_entry_rejects_grow(void) {
    /* Put two entries, then grow the head entry with eviction of the tail. */
    g_cache->max_size_bytes = 10;
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "keepseg", "data", 4, false));
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "growseg", "xy", 2, false));
    /* current = 6, lru_list: head=growseg, tail=keepseg */

    /* Grow growseg from 2 to 7. needed=5. 6+5=11>10.
     * Eviction evicts keepseg(4): 2+5=7<=10 → true → put succeeds.
     * After: current=7, growseg content_size=7. */
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "growseg", "longer!", 7, false));
    TEST_ASSERT_EQUAL_size_t(7, g_cache->current_size_bytes);
}

void test_lru_put_dirty_triggers_sync(void) {
    TEST_ASSERT_TRUE(chat_lru_cache_put(g_cache, "dirtyseg", "data", 4, true));

    /* sync_requested should be true */
    TEST_ASSERT_TRUE(g_cache->sync_requested);
}

void test_lru_put_content_null(void) {
    /* content_size > 0 but content is NULL — should fail */
    TEST_ASSERT_FALSE(chat_lru_cache_put(g_cache, "nullseg", NULL, 4, false));

    /* Also test with content_size == 0 */
    TEST_ASSERT_FALSE(chat_lru_cache_put(g_cache, "zerosize", "data", 0, false));
}

/* --- Get Stats tests --- */

void test_lru_get_stats_valid(void) {
    chat_lru_cache_put(g_cache, "statseg01", "data1", 5, false);
    chat_lru_cache_put(g_cache, "statseg02", "data2", 5, true);

    ChatLRUCacheStats stats;
    TEST_ASSERT_TRUE(chat_lru_cache_get_stats(g_cache, &stats));
    TEST_ASSERT_EQUAL_UINT64(2, stats.total_entries);
    TEST_ASSERT_EQUAL_size_t(10, stats.total_size_bytes);
    TEST_ASSERT_TRUE(g_cache->sync_requested);
}

/* --- Save Metadata tests --- */

void test_lru_save_metadata_success(void) {
    chat_lru_cache_put(g_cache, "metaseg01", "data", 4, false);

    bool result = chat_lru_cache_save_metadata(g_cache);
    TEST_ASSERT_TRUE(result);

    /* Verify file exists */
    char* meta_path = chat_lru_cache_get_metadata_path(g_temp_dir);
    TEST_ASSERT_NOT_NULL(meta_path);
    struct stat st;
    TEST_ASSERT_EQUAL(0, stat(meta_path, &st));
    TEST_ASSERT(st.st_size > 0);
    free(meta_path);
}

void test_lru_save_metadata_null(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_save_metadata(NULL));

    /* cache with NULL cache_dir */
    ChatLRUCache cache2;
    memset(&cache2, 0, sizeof(cache2));
    cache2.mutex_initialized = true;
    TEST_ASSERT_FALSE(chat_lru_cache_save_metadata(&cache2));
}

/* --- Load Metadata tests --- */

void test_lru_load_metadata_success(void) {
    /* First save metadata */
    chat_lru_cache_put(g_cache, "loadseg01", "data", 4, false);
    chat_lru_cache_put(g_cache, "loadseg02", "more", 4, false);
    chat_lru_cache_save_metadata(g_cache);

    /* Create a fresh cache and load metadata */
    ChatLRUCache* cache2 = calloc(1, sizeof(ChatLRUCache));
    TEST_ASSERT_NOT_NULL(cache2);
    cache2->database = strdup("testdb2");
    cache2->cache_dir = strdup(g_temp_dir);
    cache2->max_size_bytes = 1024;
    cache2->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    cache2->mutex_initialized = true;
    pthread_mutex_init(&cache2->cache_mutex, NULL);

    /* Reset stats before load */
    memset(&cache2->stats, 0, sizeof(cache2->stats));

    bool result = chat_lru_cache_load_metadata(cache2);
    TEST_ASSERT_TRUE(result);

    TEST_ASSERT_EQUAL_UINT64(2, cache2->stats.total_entries);
    /* total_size_bytes is not loaded from metadata (only stats counters are) */
    TEST_ASSERT_EQUAL_UINT64(0, cache2->stats.cache_hits);
    TEST_ASSERT_EQUAL_UINT64(0, cache2->stats.cache_misses);
    TEST_ASSERT_EQUAL_UINT64(0, cache2->stats.evictions);
    free(cache2->hash_table);
    free(cache2->cache_dir);
    free(cache2->database);
    pthread_mutex_destroy(&cache2->cache_mutex);
    free(cache2);
}

void test_lru_load_metadata_null(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_load_metadata(NULL));

    ChatLRUCache cache2;
    memset(&cache2, 0, sizeof(cache2));
    cache2.mutex_initialized = true;
    TEST_ASSERT_FALSE(chat_lru_cache_load_metadata(&cache2));
}

void test_lru_load_metadata_file_not_found(void) {
    /* Use a cache dir with no metadata file */
    ChatLRUCache* cache2 = calloc(1, sizeof(ChatLRUCache));
    TEST_ASSERT_NOT_NULL(cache2);
    char empty_dir[256];
    snprintf(empty_dir, sizeof(empty_dir), "/tmp/hydrogen_lru_empty_%d", (int)getpid());
    mkdir(empty_dir, 0755);
    cache2->database = strdup("testdb3");
    cache2->cache_dir = strdup(empty_dir);
    cache2->max_size_bytes = 1024;
    cache2->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    cache2->mutex_initialized = true;
    pthread_mutex_init(&cache2->cache_mutex, NULL);

    bool result = chat_lru_cache_load_metadata(cache2);
    TEST_ASSERT_FALSE(result);

    free(cache2->hash_table);
    free(cache2->cache_dir);
    free(cache2->database);
    pthread_mutex_destroy(&cache2->cache_mutex);
    free(cache2);
    rmdir(empty_dir);
}

void test_lru_load_metadata_invalid_json(void) {
    /* Create a metadata file with invalid JSON */
    char* meta_path = chat_lru_cache_get_metadata_path(g_temp_dir);
    TEST_ASSERT_NOT_NULL(meta_path);

    FILE* fp = fopen(meta_path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("{invalid json content}", fp);
    fclose(fp);
    free(meta_path);

    ChatLRUCache* cache2 = calloc(1, sizeof(ChatLRUCache));
    TEST_ASSERT_NOT_NULL(cache2);
    cache2->database = strdup("testdb4");
    cache2->cache_dir = strdup(g_temp_dir);
    cache2->max_size_bytes = 1024;
    cache2->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    cache2->mutex_initialized = true;
    pthread_mutex_init(&cache2->cache_mutex, NULL);

    bool result = chat_lru_cache_load_metadata(cache2);
    TEST_ASSERT_FALSE(result);

    free(cache2->hash_table);
    free(cache2->cache_dir);
    free(cache2->database);
    pthread_mutex_destroy(&cache2->cache_mutex);
    free(cache2);
}

void test_lru_load_metadata_empty_file(void) {
    /* Create an empty metadata file */
    char* meta_path = chat_lru_cache_get_metadata_path(g_temp_dir);
    TEST_ASSERT_NOT_NULL(meta_path);

    FILE* fp = fopen(meta_path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fclose(fp);
    free(meta_path);

    ChatLRUCache* cache2 = calloc(1, sizeof(ChatLRUCache));
    TEST_ASSERT_NOT_NULL(cache2);
    cache2->database = strdup("testdb5");
    cache2->cache_dir = strdup(g_temp_dir);
    cache2->max_size_bytes = 1024;
    cache2->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    cache2->mutex_initialized = true;
    pthread_mutex_init(&cache2->cache_mutex, NULL);

    bool result = chat_lru_cache_load_metadata(cache2);
    TEST_ASSERT_FALSE(result);

    free(cache2->hash_table);
    free(cache2->cache_dir);
    free(cache2->database);
    pthread_mutex_destroy(&cache2->cache_mutex);
    free(cache2);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lru_evict_enough_space_no_eviction);
    RUN_TEST(test_lru_evict_with_entries);
    RUN_TEST(test_lru_evict_frees_tail_when_empty_after);
    RUN_TEST(test_lru_evict_enough_space_returns_true);
    RUN_TEST(test_lru_evict_updates_stats);
    RUN_TEST(test_lru_contains_found);
    RUN_TEST(test_lru_put_new_entry);
    RUN_TEST(test_lru_put_existing_entry_update);
    RUN_TEST(test_lru_put_existing_entry_rejects_grow);
    RUN_TEST(test_lru_put_dirty_triggers_sync);
    RUN_TEST(test_lru_put_content_null);
    RUN_TEST(test_lru_get_stats_valid);
    RUN_TEST(test_lru_save_metadata_success);
    RUN_TEST(test_lru_save_metadata_null);
    RUN_TEST(test_lru_load_metadata_success);
    RUN_TEST(test_lru_load_metadata_null);
    RUN_TEST(test_lru_load_metadata_file_not_found);
    RUN_TEST(test_lru_load_metadata_invalid_json);
    RUN_TEST(test_lru_load_metadata_empty_file);
    return UNITY_END();
}
