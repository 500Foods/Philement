#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>
#include <src/database/dbqueue/dbqueue.h>
#include "mock_dbqueue.h"
#include "mock_database_engine.h"

extern DatabaseQueueManager *global_queue_manager;

static DatabaseQueue *g_dbq = NULL;
static DatabaseHandle *g_handle = NULL;

void setUp(void);
void tearDown(void);

void test_store_segment_null_database(void);
void test_store_segment_null_message(void);
void test_store_segment_zero_len(void);
void test_store_segment_no_db_queue(void);
void test_store_segment_query_failure(void);
void test_store_segment_query_success(void);
void test_segment_exists_null(void);
void test_segment_exists_cache_hit(void);
void test_segment_exists_db_miss(void);
void test_segment_exists_db_hit(void);
void test_store_chat_null_params(void);
void test_store_chat_no_db_queue(void);
void test_store_chat_query_failure(void);
void test_store_chat_query_success(void);
void test_free_hash_null(void);
void test_free_hash_valid(void);
void test_get_or_create_cache_null(void);
void test_get_cache_null(void);
void test_get_cache_not_found(void);
void test_cache_get_stats_null(void);
void test_cache_get_stats_no_cache(void);
void test_cache_get_stats_success(void);

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();

    g_handle = calloc(1, sizeof(DatabaseHandle));
    g_dbq = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_handle);
    TEST_ASSERT_NOT_NULL(g_dbq);
    g_dbq->persistent_connection = g_handle;
    mock_dbqueue_set_get_database_result(g_dbq);
    global_queue_manager = (DatabaseQueueManager *)0x1;
}

void tearDown(void) {
    free(g_dbq);
    free(g_handle);
    g_dbq = NULL;
    g_handle = NULL;
    global_queue_manager = NULL;
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
}

void test_store_segment_null_database(void) {
    char *result = chat_storage_store_segment(NULL, "msg", 3);
    TEST_ASSERT_NULL(result);
}

void test_store_segment_null_message(void) {
    char *result = chat_storage_store_segment("db1", NULL, 3);
    TEST_ASSERT_NULL(result);
}

void test_store_segment_zero_len(void) {
    char *result = chat_storage_store_segment("db1", "msg", 0);
    TEST_ASSERT_NULL(result);
}

void test_store_segment_no_db_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    char *result = chat_storage_store_segment("db1", "hello", 5);
    TEST_ASSERT_NULL(result);
    mock_dbqueue_set_get_database_result(g_dbq);
}

void test_store_segment_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    char *result = chat_storage_store_segment("db1", "hello", 5);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
}

void test_store_segment_query_success(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[{\"result\":\"ok\"}]");
    char *result = chat_storage_store_segment("db1", "hello", 5);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
}

void test_segment_exists_null(void) {
    TEST_ASSERT_FALSE(chat_storage_segment_exists(NULL, "hash"));
    TEST_ASSERT_FALSE(chat_storage_segment_exists("db1", NULL));
}

void test_segment_exists_cache_hit(void) {
    chat_storage_get_or_create_cache("db1_test_cache_hit");
    ChatLRUCache *cache = chat_storage_get_cache("db1_test_cache_hit");
    TEST_ASSERT_NOT_NULL(cache);

    const char *message = "test content for cache hit";
    char *hash = chat_storage_generate_hash(message, strlen(message));
    TEST_ASSERT_NOT_NULL(hash);

    chat_lru_cache_put(cache, hash, message, strlen(message), false);
    TEST_ASSERT_TRUE(chat_storage_segment_exists("db1_test_cache_hit", hash));

    free(hash);
}

void test_segment_exists_db_miss(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[]");
    TEST_ASSERT_FALSE(chat_storage_segment_exists("db1", "nonexistent_hash"));
}

void test_segment_exists_db_hit(void) {
    const char *content = "hello";
    char *hash_hex = NULL;
    uint8_t *compressed = NULL;
    size_t compressed_size = 0;

    chat_storage_compress_message(content, strlen(content), &compressed, &compressed_size);
    hash_hex = calloc(compressed_size * 2 + 1, 1);
    for (size_t i = 0; i < compressed_size; i++) {
        sprintf(hash_hex + i * 2, "%02x", compressed[i]);
    }

    char result_json[512];
    snprintf(result_json, sizeof(result_json),
        "[{\"segment_content\":\"%s\"}]", hash_hex);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data(result_json);

    TEST_ASSERT_TRUE(chat_storage_segment_exists("db1", hash_hex));

    free(hash_hex);
    free(compressed);
}

void test_store_chat_null_params(void) {
    TEST_ASSERT_NULL(chat_storage_store_chat(NULL, "convos", NULL, 0,
        "engine", "model", 0, 0, 0.0, 0, 0, "session"));
    TEST_ASSERT_NULL(chat_storage_store_chat("db1", NULL, NULL, 0,
        "engine", "model", 0, 0, 0.0, 0, 0, "session"));
    TEST_ASSERT_NULL(chat_storage_store_chat("db1", "convos", NULL, 0,
        "engine", "model", 0, 0, 0.0, 0, 0, "session"));
}

void test_store_chat_no_db_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    const char *hashes[] = {"hash1"};
    char *result = chat_storage_store_chat("db1", "convos", hashes, 1,
        "engine", "model", 0, 0, 0.0, 0, 0, "session");
    TEST_ASSERT_NULL(result);
    mock_dbqueue_set_get_database_result(g_dbq);
}

void test_store_chat_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    const char *hashes[] = {"hash1"};
    char *result = chat_storage_store_chat("db1", "convos", hashes, 1,
        "engine", "model", 100, 200, 0.05, 1, 100, "session");
    TEST_ASSERT_NULL(result);
}

void test_store_chat_query_success(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("{\"ok\":true}");
    const char *hashes[] = {"hash1", "hash2"};
    char *result = chat_storage_store_chat("db1", "convos", hashes, 2,
        "engine", "model", 100, 200, 0.05, 1, 100, "session");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("convos", result);
    free(result);
}

void test_free_hash_null(void) {
    chat_storage_free_hash(NULL);
}

void test_free_hash_valid(void) {
    char *hash = strdup("abc123");
    chat_storage_free_hash(hash);
}

void test_get_or_create_cache_null(void) {
    TEST_ASSERT_NULL(chat_storage_get_or_create_cache(NULL));
}

void test_get_cache_null(void) {
    TEST_ASSERT_NULL(chat_storage_get_cache(NULL));
}

void test_get_cache_not_found(void) {
    TEST_ASSERT_NULL(chat_storage_get_cache("nonexistent_db"));
}

void test_cache_get_stats_null(void) {
    TEST_ASSERT_FALSE(chat_storage_cache_get_stats(NULL, NULL, NULL, NULL));
}

void test_cache_get_stats_no_cache(void) {
    uint64_t hits = 0, misses = 0;
    double ratio = 0;
    TEST_ASSERT_FALSE(chat_storage_cache_get_stats("no_cache_db", &hits, &misses, &ratio));
}

void test_cache_get_stats_success(void) {
    chat_storage_get_or_create_cache("db1_stats_test");
    ChatLRUCache *cache = chat_storage_get_cache("db1_stats_test");
    TEST_ASSERT_NOT_NULL(cache);

    const char *message = "test for stats";
    char *hash = chat_storage_generate_hash(message, strlen(message));
    chat_lru_cache_put(cache, hash, message, strlen(message), false);
    chat_lru_cache_contains(cache, hash);
    chat_lru_cache_contains(cache, "missing_hash");

    uint64_t hits = 0, misses = 0;
    double ratio = 0;
    bool result = chat_storage_cache_get_stats("db1_stats_test", &hits, &misses, &ratio);
    TEST_ASSERT_TRUE(result);
    free(hash);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_store_segment_null_database);
    RUN_TEST(test_store_segment_null_message);
    RUN_TEST(test_store_segment_zero_len);
    RUN_TEST(test_store_segment_no_db_queue);
    RUN_TEST(test_store_segment_query_failure);
    RUN_TEST(test_store_segment_query_success);
    RUN_TEST(test_segment_exists_null);
    RUN_TEST(test_segment_exists_cache_hit);
    RUN_TEST(test_segment_exists_db_miss);
    RUN_TEST(test_segment_exists_db_hit);
    RUN_TEST(test_store_chat_null_params);
    RUN_TEST(test_store_chat_no_db_queue);
    RUN_TEST(test_store_chat_query_failure);
    RUN_TEST(test_store_chat_query_success);
    RUN_TEST(test_free_hash_null);
    RUN_TEST(test_free_hash_valid);
    RUN_TEST(test_get_or_create_cache_null);
    RUN_TEST(test_get_cache_null);
    RUN_TEST(test_get_cache_not_found);
    RUN_TEST(test_cache_get_stats_null);
    RUN_TEST(test_cache_get_stats_no_cache);
    RUN_TEST(test_cache_get_stats_success);
    return UNITY_END();
}
