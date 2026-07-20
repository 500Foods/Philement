/*
 * Unity Test File: chat_storage_retrieve_segments_batch
 * This file contains unit tests for chat_storage_retrieve_segments_batch()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

static char* build_compressed_hex(const char* content) {
    uint8_t* comp = NULL;
    size_t comp_size = 0;
    if (!chat_storage_compress_message(content, strlen(content), &comp, &comp_size)) {
        return NULL;
    }
    char* hex = calloc(comp_size * 2 + 1, 1);
    if (hex) {
        for (size_t i = 0; i < comp_size; i++) {
            sprintf(hex + i * 2, "%02x", comp[i]);
        }
    }
    free(comp);
    return hex;
}

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
    chat_storage_cache_shutdown("testdb");

    g_handle = calloc(1, sizeof(DatabaseHandle));
    g_dbq = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_handle);
    TEST_ASSERT_NOT_NULL(g_dbq);
    g_dbq->persistent_connection = g_handle;
    mock_dbqueue_set_get_database_result(g_dbq);
}

void tearDown(void) {
    free(g_dbq);
    free(g_handle);
    g_dbq = NULL;
    g_handle = NULL;
    chat_storage_cache_shutdown("testdb");
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
}

/* NULL database */
static void test_batch_null_database(void) {
    const char* hashes[] = {"h1"};
    json_t* result = chat_storage_retrieve_segments_batch(NULL, hashes, 1);
    TEST_ASSERT_NULL(result);
}

/* NULL hashes */
static void test_batch_null_hashes(void) {
    json_t* result = chat_storage_retrieve_segments_batch("testdb", NULL, 1);
    TEST_ASSERT_NULL(result);
}

/* zero count */
static void test_batch_zero_count(void) {
    const char* hashes[] = {"h1"};
    json_t* result = chat_storage_retrieve_segments_batch("testdb", hashes, 0);
    TEST_ASSERT_NULL(result);
}

/* all found in cache */
static void test_batch_all_cache_hits(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "h1", "one", 3, false));
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "h2", "two", 3, false));

    const char* hashes[] = {"h1", "h2"};
    json_t* result = chat_storage_retrieve_segments_batch("testdb", hashes, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_array(result));
    TEST_ASSERT_EQUAL_INT(2, json_array_size(result));

    /* verify source tags */
    json_t* seg0 = json_array_get(result, 0);
    TEST_ASSERT_EQUAL_STRING("cache", json_string_value(json_object_get(seg0, "source")));
    json_decref(result);
}

/* skip NULL entries in the hash array */
static void test_batch_skips_null_entries(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "h1", "one", 3, false));

    const char* hashes[] = {NULL, "h1", NULL};
    json_t* result = chat_storage_retrieve_segments_batch("testdb", hashes, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(result));
    json_decref(result);
}

/* some from cache, some from DB, some missing */
static void test_batch_mixed_cache_and_db(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "cached", "cachedcontent", 13, false));

    char* hex = build_compressed_hex("dbcontent");
    TEST_ASSERT_NOT_NULL(hex);
    char buf[2048];
    /* Only "dbhash" is returned by the (mock) DB */
    snprintf(buf, sizeof(buf),
             "[{\"segment_hash\": \"dbhash\", \"segment_content\": \"%s\"}]", hex);
    free(hex);
    mock_database_engine_set_execute_json_data(buf);

    const char* hashes[] = {"cached", "dbhash", "missing"};
    json_t* result = chat_storage_retrieve_segments_batch("testdb", hashes, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(2, json_array_size(result));

    /* dbhash should now also be cached */
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "dbhash"));
    json_decref(result);
}

/* DB fetch when queue/connection unavailable returns partial cache-only results */
static void test_batch_db_unavailable(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "cached", "cachedcontent", 13, false));

    mock_dbqueue_set_get_database_result(NULL);

    const char* hashes[] = {"cached", "dbhash"};
    json_t* result = chat_storage_retrieve_segments_batch("testdb", hashes, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(result));
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_batch_null_database);
    RUN_TEST(test_batch_null_hashes);
    RUN_TEST(test_batch_zero_count);
    RUN_TEST(test_batch_all_cache_hits);
    RUN_TEST(test_batch_skips_null_entries);
    RUN_TEST(test_batch_mixed_cache_and_db);
    RUN_TEST(test_batch_db_unavailable);
    return UNITY_END();
}
