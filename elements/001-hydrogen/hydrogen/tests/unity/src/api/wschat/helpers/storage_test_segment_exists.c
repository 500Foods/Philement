/*
 * Unity Test File: chat_storage_segment_exists
 * This file contains unit tests for chat_storage_segment_exists()
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
static void test_segment_exists_null_database(void) {
    TEST_ASSERT_FALSE(chat_storage_segment_exists(NULL, "abc"));
}

/* NULL hash */
static void test_segment_exists_null_hash(void) {
    TEST_ASSERT_FALSE(chat_storage_segment_exists("testdb", NULL));
}

/* cache hit */
static void test_segment_exists_cache_hit(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "hash1", "x", 1, false));
    TEST_ASSERT_TRUE(chat_storage_segment_exists("testdb", "hash1"));
}

/* no queue */
static void test_segment_exists_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    TEST_ASSERT_FALSE(chat_storage_segment_exists("testdb", "hash1"));
}

/* no connection */
static void test_segment_exists_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    TEST_ASSERT_FALSE(chat_storage_segment_exists("testdb", "hash1"));
}

/* query fails */
static void test_segment_exists_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    TEST_ASSERT_FALSE(chat_storage_segment_exists("testdb", "hash1"));
}

/* empty array */
static void test_segment_exists_no_data(void) {
    mock_database_engine_set_execute_json_data("[]");
    TEST_ASSERT_FALSE(chat_storage_segment_exists("testdb", "hash1"));
}

/* success path - populates cache from DB */
static void test_segment_exists_success(void) {
    char* hex = build_compressed_hex("segment payload");
    TEST_ASSERT_NOT_NULL(hex);
    char buf[1024];
    snprintf(buf, sizeof(buf), "[{\"segment_content\": \"%s\"}]", hex);
    free(hex);

    mock_database_engine_set_execute_json_data(buf);
    TEST_ASSERT_TRUE(chat_storage_segment_exists("testdb", "hashdb"));

    ChatLRUCache* cache = chat_storage_get_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "hashdb"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_segment_exists_null_database);
    RUN_TEST(test_segment_exists_null_hash);
    RUN_TEST(test_segment_exists_cache_hit);
    RUN_TEST(test_segment_exists_no_queue);
    RUN_TEST(test_segment_exists_no_connection);
    RUN_TEST(test_segment_exists_query_failure);
    RUN_TEST(test_segment_exists_no_data);
    RUN_TEST(test_segment_exists_success);
    return UNITY_END();
}
