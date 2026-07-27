/*
 * Unity Test File: chat_storage_store_segment
 * This file contains unit tests for chat_storage_store_segment()
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

static char g_tmp_root[] = "/tmp/storage_test_store_segment_XXXXXX";

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
static void test_store_segment_null_database(void) {
    char* hash = chat_storage_store_segment(NULL, "hello", 5);
    TEST_ASSERT_NULL(hash);
}

/* NULL message */
static void test_store_segment_null_message(void) {
    char* hash = chat_storage_store_segment("testdb", NULL, 5);
    TEST_ASSERT_NULL(hash);
}

/* zero length */
static void test_store_segment_zero_length(void) {
    char* hash = chat_storage_store_segment("testdb", "hello", 0);
    TEST_ASSERT_NULL(hash);
}

/* database queue unavailable */
static void test_store_segment_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    char* hash = chat_storage_store_segment("testdb", "hello", 5);
    TEST_ASSERT_NULL(hash);
}

/* queue present but persistent_connection NULL */
static void test_store_segment_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    char* hash = chat_storage_store_segment("testdb", "hello", 5);
    TEST_ASSERT_NULL(hash);
}

/* query execution fails (still returns the hash per implementation) */
static void test_store_segment_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    char* hash = chat_storage_store_segment("testdb", "hello world", 11);
    /* Implementation returns hash regardless of query success */
    TEST_ASSERT_NOT_NULL(hash);
    free(hash);
}

/* full happy path */
static void test_store_segment_success(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("{\"status\":\"ok\"}");
    char* hash = chat_storage_store_segment("testdb", "hello world", 11);
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_STRING_LEN("testdb", "testdb", 6); /* sanity */
    free(hash);
}

int main(void) {
    mkdtemp(g_tmp_root);
    UNITY_BEGIN();
    RUN_TEST(test_store_segment_null_database);
    RUN_TEST(test_store_segment_null_message);
    RUN_TEST(test_store_segment_zero_length);
    RUN_TEST(test_store_segment_no_queue);
    RUN_TEST(test_store_segment_no_connection);
    RUN_TEST(test_store_segment_query_failure);
    RUN_TEST(test_store_segment_success);
    return UNITY_END();
}
