#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/lru_cache.h>
#include <string.h>

void test_hash_string_basic(void);
void test_hash_string_empty(void);
void test_hash_string_consistency(void);
void test_hash_string_djb2_property(void);
void test_get_base_dir_env_set(void);
void test_get_base_dir_env_unset(void);
void test_get_dir_null_database(void);
void test_get_dir_basic(void);
void test_get_metadata_path_null(void);
void test_get_segment_path_null(void);
void test_get_segment_path_short_hash(void);
void test_get_segment_path_valid(void);
void test_free_entry_null(void);
void test_free_entry_valid(void);
void test_remove_entry_null(void);
void test_add_front_null(void);
void test_add_front_single(void);
void test_add_front_multiple(void);
void test_add_front_moves_to_head(void);
void test_add_to_empty_cache_sets_tail(void);
void test_remove_entry_updates_tail(void);
void test_remove_entry_updates_head(void);
void test_contains_null(void);
void test_put_null_params(void);
void test_get_stats_null(void);
void test_request_sync_null(void);
void test_flush_null(void);
void test_flush_no_dirty(void);
void test_evict_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_hash_string_basic(void) {
    size_t h = chat_lru_cache_hash_string("test");
    TEST_ASSERT(h < LRU_CACHE_HASH_TABLE_SIZE);
}

void test_hash_string_empty(void) {
    size_t h = chat_lru_cache_hash_string("");
    TEST_ASSERT(h < LRU_CACHE_HASH_TABLE_SIZE);
}

void test_hash_string_consistency(void) {
    size_t h1 = chat_lru_cache_hash_string("consistent");
    size_t h2 = chat_lru_cache_hash_string("consistent");
    TEST_ASSERT_EQUAL_size_t(h1, h2);
}

void test_hash_string_djb2_property(void) {
    /* DJB2 starts at 5381; verify we get a non-zero, bounded value */
    size_t h = chat_lru_cache_hash_string("hello world");
    TEST_ASSERT(h > 0);
    TEST_ASSERT(h < LRU_CACHE_HASH_TABLE_SIZE);
}

void test_get_base_dir_env_set(void) {
    setenv("CHAT_CACHE_DIR", "/tmp/test_cache_dir", 1);
    const char *dir = chat_lru_cache_get_base_dir();
    TEST_ASSERT_EQUAL_STRING("/tmp/test_cache_dir", dir);
    unsetenv("CHAT_CACHE_DIR");
}

void test_get_base_dir_env_unset(void) {
    unsetenv("CHAT_CACHE_DIR");
    const char *dir = chat_lru_cache_get_base_dir();
    TEST_ASSERT_EQUAL_STRING("cache", dir);
}

void test_get_dir_null_database(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_dir(NULL));
}

void test_get_dir_basic(void) {
    char *dir = chat_lru_cache_get_dir("mydb");
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_EQUAL_STRING("cache", dir);
    free(dir);
}

void test_get_metadata_path_null(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_metadata_path(NULL));
}

void test_get_segment_path_null(void) {
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path(NULL, "abc123"));
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path("cache", NULL));
}

void test_get_segment_path_short_hash(void) {
    /* Hash shorter than LRU_CACHE_PREFIX_LEN (2) should return NULL */
    TEST_ASSERT_NULL(chat_lru_cache_get_segment_path("cache", "a"));
}

void test_get_segment_path_valid(void) {
    char *path = chat_lru_cache_get_segment_path("cache", "ab1234cd");
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT(strstr(path, "ab/ab1234cd.json") != NULL);
    free(path);
}

void test_free_entry_null(void) {
    chat_lru_cache_free_entry(NULL);
}

void test_free_entry_valid(void) {
    ChatLRUCacheEntry *entry = calloc(1, sizeof(ChatLRUCacheEntry));
    TEST_ASSERT_NOT_NULL(entry);
    entry->content = strdup("some content");
    TEST_ASSERT_NOT_NULL(entry->content);
    entry->content_size = 12;
    chat_lru_cache_free_entry(entry);
}

void test_remove_entry_null(void) {
    chat_lru_cache_remove_entry(NULL, NULL);
}

void test_add_front_null(void) {
    chat_lru_cache_add_front(NULL, NULL);
}

void test_add_front_single(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry entry;
    memset(&entry, 0, sizeof(entry));

    chat_lru_cache_add_front(&cache, &entry);
    TEST_ASSERT_EQUAL_PTR(&entry, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&entry, cache.lru_tail);
}

void test_add_front_multiple(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry e1, e2, e3;
    memset(&e1, 0, sizeof(e1));
    memset(&e2, 0, sizeof(e2));
    memset(&e3, 0, sizeof(e3));

    chat_lru_cache_add_front(&cache, &e1);
    chat_lru_cache_add_front(&cache, &e2);
    chat_lru_cache_add_front(&cache, &e3);
    /* e3 should be head (most recent), e1 should be tail */
    TEST_ASSERT_EQUAL_PTR(&e3, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&e1, cache.lru_tail);
}

void test_add_front_moves_to_head(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry e1, e2;
    memset(&e1, 0, sizeof(e1));
    memset(&e2, 0, sizeof(e2));

    chat_lru_cache_add_front(&cache, &e1);
    chat_lru_cache_add_front(&cache, &e2);
    /* e2 is head. Now re-add e1 to front */
    chat_lru_cache_remove_entry(&cache, &e1);
    chat_lru_cache_add_front(&cache, &e1);
    TEST_ASSERT_EQUAL_PTR(&e1, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&e2, cache.lru_tail);
}

void test_add_to_empty_cache_sets_tail(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry entry;
    memset(&entry, 0, sizeof(entry));

    chat_lru_cache_add_front(&cache, &entry);
    TEST_ASSERT_EQUAL_PTR(&entry, cache.lru_head);
    TEST_ASSERT_EQUAL_PTR(&entry, cache.lru_tail);
    TEST_ASSERT_NULL(cache.lru_head->lru_next);
    TEST_ASSERT_NULL(cache.lru_head->lru_prev);
}

void test_remove_entry_updates_tail(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry e1, e2, e3;
    memset(&e1, 0, sizeof(e1));
    memset(&e2, 0, sizeof(e2));
    memset(&e3, 0, sizeof(e3));

    chat_lru_cache_add_front(&cache, &e1);
    chat_lru_cache_add_front(&cache, &e2);
    chat_lru_cache_add_front(&cache, &e3);

    /* Remove e1 (the tail) */
    chat_lru_cache_remove_entry(&cache, &e1);
    TEST_ASSERT_EQUAL_PTR(&e2, cache.lru_tail);
}

void test_remove_entry_updates_head(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    ChatLRUCacheEntry e1, e2, e3;
    memset(&e1, 0, sizeof(e1));
    memset(&e2, 0, sizeof(e2));
    memset(&e3, 0, sizeof(e3));

    chat_lru_cache_add_front(&cache, &e1);
    chat_lru_cache_add_front(&cache, &e2);
    chat_lru_cache_add_front(&cache, &e3);

    /* Remove e3 (the head) */
    chat_lru_cache_remove_entry(&cache, &e3);
    TEST_ASSERT_EQUAL_PTR(&e2, cache.lru_head);
    TEST_ASSERT_NULL(cache.lru_head->lru_prev);
}

void test_contains_null(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_contains(NULL, "hash"));
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    cache.mutex_initialized = true;
    TEST_ASSERT_FALSE(chat_lru_cache_contains(&cache, NULL));
}

void test_put_null_params(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    cache.mutex_initialized = true;
    cache.max_size_bytes = 1024;
    cache.hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));

    TEST_ASSERT_FALSE(chat_lru_cache_put(NULL, "hash", "data", 4, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(&cache, NULL, "data", 4, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(&cache, "hash", NULL, 4, false));
    TEST_ASSERT_FALSE(chat_lru_cache_put(&cache, "hash", "data", 0, false));

    free(cache.hash_table);
}

void test_get_stats_null(void) {
    ChatLRUCacheStats stats;
    TEST_ASSERT_FALSE(chat_lru_cache_get_stats(NULL, &stats));
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    cache.mutex_initialized = true;
    TEST_ASSERT_FALSE(chat_lru_cache_get_stats(&cache, NULL));
}

void test_request_sync_null(void) {
    chat_lru_cache_request_sync(NULL);
}

void test_flush_null(void) {
    TEST_ASSERT_EQUAL_INT(-1, chat_lru_cache_flush(NULL));
}

void test_flush_no_dirty(void) {
    ChatLRUCache cache;
    memset(&cache, 0, sizeof(cache));
    cache.mutex_initialized = true;

    int result = chat_lru_cache_flush(&cache);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_evict_null(void) {
    TEST_ASSERT_FALSE(chat_lru_cache_evict_lru_entries(NULL, 100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hash_string_basic);
    RUN_TEST(test_hash_string_empty);
    RUN_TEST(test_hash_string_consistency);
    RUN_TEST(test_hash_string_djb2_property);
    RUN_TEST(test_get_base_dir_env_set);
    RUN_TEST(test_get_base_dir_env_unset);
    RUN_TEST(test_get_dir_null_database);
    RUN_TEST(test_get_dir_basic);
    RUN_TEST(test_get_metadata_path_null);
    RUN_TEST(test_get_segment_path_null);
    RUN_TEST(test_get_segment_path_short_hash);
    RUN_TEST(test_get_segment_path_valid);
    RUN_TEST(test_free_entry_null);
    RUN_TEST(test_free_entry_valid);
    RUN_TEST(test_remove_entry_null);
    RUN_TEST(test_add_front_null);
    RUN_TEST(test_add_front_single);
    RUN_TEST(test_add_front_multiple);
    RUN_TEST(test_add_front_moves_to_head);
    RUN_TEST(test_add_to_empty_cache_sets_tail);
    RUN_TEST(test_remove_entry_updates_tail);
    RUN_TEST(test_remove_entry_updates_head);
    RUN_TEST(test_contains_null);
    RUN_TEST(test_put_null_params);
    RUN_TEST(test_get_stats_null);
    RUN_TEST(test_request_sync_null);
    RUN_TEST(test_flush_null);
    RUN_TEST(test_flush_no_dirty);
    RUN_TEST(test_evict_null);
    return UNITY_END();
}
