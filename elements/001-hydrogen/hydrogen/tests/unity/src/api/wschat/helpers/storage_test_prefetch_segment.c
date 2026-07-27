/*
 * Unity Test File: chat_storage_prefetch_segment
 * This file contains unit tests for chat_storage_prefetch_segment()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage.h>
#include <src/api/wschat/helpers/lru_cache.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

static char g_tmp_root[] = "/tmp/storage_test_prefetch_segment_XXXXXX";

void setUp(void) {
    /* Create sandbox cache directory before any cache operations */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s 2>/dev/null", g_tmp_root);
    system(cmd);
    mkdir(g_tmp_root, 0755);
    setenv(CHAT_CACHE_DIR_ENV_VAR, g_tmp_root, 1);

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

    /* Clean up sandbox */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s 2>/dev/null", g_tmp_root);
    system(cmd);
    unsetenv(CHAT_CACHE_DIR_ENV_VAR);
}

/* NULL database */
static void test_prefetch_null_database(void) {
    TEST_ASSERT_FALSE(chat_storage_prefetch_segment(NULL, "h1"));
}

/* NULL hash */
static void test_prefetch_null_hash(void) {
    TEST_ASSERT_FALSE(chat_storage_prefetch_segment("testdb", NULL));
}

/* no cache available */
static void test_prefetch_no_cache(void) {
    mock_dbqueue_set_get_database_result(NULL);
    TEST_ASSERT_FALSE(chat_storage_prefetch_segment("testdb", "h1"));
}

/* already in cache */
static void test_prefetch_cache_hit(void) {
    ChatLRUCache* cache = chat_storage_get_or_create_cache("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_TRUE(chat_lru_cache_put(cache, "h1", "x", 1, false));
    TEST_ASSERT_TRUE(chat_storage_prefetch_segment("testdb", "h1"));
}

/* not in cache - falls back to segment_exists (DB miss here) */
static void test_prefetch_segment_exists_miss(void) {
    mock_database_engine_set_execute_json_data("[]");
    TEST_ASSERT_FALSE(chat_storage_prefetch_segment("testdb", "h1"));
}

/* not in cache - falls back to segment_exists (DB hit) */
static void test_prefetch_segment_exists_hit(void) {
    mock_database_engine_set_execute_json_data("[{\"segment_content\": \"00\"}]");
    TEST_ASSERT_TRUE(chat_storage_prefetch_segment("testdb", "h1"));
}

int main(void) {
    if (mkdtemp(g_tmp_root) == NULL) {
        strncpy(g_tmp_root, "/tmp/storage_test_prefetch_segment_fb", sizeof(g_tmp_root) - 1);
        mkdir(g_tmp_root, 0755);
    }
    UNITY_BEGIN();
    RUN_TEST(test_prefetch_null_database);
    RUN_TEST(test_prefetch_null_hash);
    RUN_TEST(test_prefetch_no_cache);
    RUN_TEST(test_prefetch_cache_hit);
    RUN_TEST(test_prefetch_segment_exists_miss);
    RUN_TEST(test_prefetch_segment_exists_hit);
    return UNITY_END();
}
