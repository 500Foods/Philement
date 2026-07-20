/*
 * Unity Test File: chat_storage_retrieve_segment
 * This file contains unit tests for chat_storage_retrieve_segment()
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

/* Build a compressed-hex string for `content` using the real compressor */
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
static void test_retrieve_segment_null_database(void) {
    char* content = chat_storage_retrieve_segment(NULL, "abc");
    TEST_ASSERT_NULL(content);
}

/* NULL segment_hash */
static void test_retrieve_segment_null_hash(void) {
    char* content = chat_storage_retrieve_segment("testdb", NULL);
    TEST_ASSERT_NULL(content);
}

/* cache hit returns content and does not touch DB */
static void test_retrieve_segment_cache_hit(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "hash1", "cached content", 14, false));

    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("cached content", content);
    free(content);
}

/* no queue available */
static void test_retrieve_segment_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* no persistent connection */
static void test_retrieve_segment_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* query fails */
static void test_retrieve_segment_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* query returns empty array (no data) */
static void test_retrieve_segment_no_data(void) {
    mock_database_engine_set_execute_json_data("[]");
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* invalid content (not a string) */
static void test_retrieve_segment_invalid_content(void) {
    mock_database_engine_set_execute_json_data("[{\"segment_content\": 123}]");
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* decompress failure (garbage hex that does not decompress) */
static void test_retrieve_segment_decompress_failure(void) {
    /* valid hex but not valid brotli => decompress fails */
    mock_database_engine_set_execute_json_data("[{\"segment_content\": \"deadbeef\"}]");
    char* content = chat_storage_retrieve_segment("testdb", "hash1");
    TEST_ASSERT_NULL(content);
}

/* happy path from DB, populates cache */
static void test_retrieve_segment_success(void) {
    char* hex = build_compressed_hex("this is a stored segment");
    TEST_ASSERT_NOT_NULL(hex);
    char buf[1024];
    snprintf(buf, sizeof(buf), "[{\"segment_content\": \"%s\"}]", hex);
    free(hex);

    mock_database_engine_set_execute_json_data(buf);
    char* content = chat_storage_retrieve_segment("testdb", "hashdb");
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("this is a stored segment", content);

    /* Should now be cached */
    ChatLRUCache* cache = chat_storage_get_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_contains(cache, "hashdb"));
    free(content);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_retrieve_segment_null_database);
    RUN_TEST(test_retrieve_segment_null_hash);
    RUN_TEST(test_retrieve_segment_cache_hit);
    RUN_TEST(test_retrieve_segment_no_queue);
    RUN_TEST(test_retrieve_segment_no_connection);
    RUN_TEST(test_retrieve_segment_query_failure);
    RUN_TEST(test_retrieve_segment_no_data);
    RUN_TEST(test_retrieve_segment_invalid_content);
    RUN_TEST(test_retrieve_segment_decompress_failure);
    RUN_TEST(test_retrieve_segment_success);
    return UNITY_END();
}
