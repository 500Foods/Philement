/*
 * Unity Test File: chat_storage_store_chat
 * This file contains unit tests for chat_storage_store_chat()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

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
static void test_store_chat_null_database(void) {
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat(NULL, "conv1", hashes, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* NULL convos_ref */
static void test_store_chat_null_convos_ref(void) {
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat("testdb", NULL, hashes, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* NULL hashes */
static void test_store_chat_null_hashes(void) {
    char* ref = chat_storage_store_chat("testdb", "conv1", NULL, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* zero count */
static void test_store_chat_zero_count(void) {
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat("testdb", "conv1", hashes, 0, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* no queue */
static void test_store_chat_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat("testdb", "conv1", hashes, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* no connection */
static void test_store_chat_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat("testdb", "conv1", hashes, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* query fails */
static void test_store_chat_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    const char* hashes[] = {"h1"};
    char* ref = chat_storage_store_chat("testdb", "conv1", hashes, 1, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NULL(ref);
}

/* success path returns strdup'd convos_ref */
static void test_store_chat_success(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("{\"status\":\"ok\"}");
    const char* hashes[] = {"h1", "h2"};
    char* ref = chat_storage_store_chat("testdb", "conv1", hashes, 2, "gpt", "4", 1, 2, 0.5, 9, 100, "sess");
    TEST_ASSERT_NOT_NULL(ref);
    TEST_ASSERT_EQUAL_STRING("conv1", ref);
    free(ref);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_store_chat_null_database);
    RUN_TEST(test_store_chat_null_convos_ref);
    RUN_TEST(test_store_chat_null_hashes);
    RUN_TEST(test_store_chat_zero_count);
    RUN_TEST(test_store_chat_no_queue);
    RUN_TEST(test_store_chat_no_connection);
    RUN_TEST(test_store_chat_query_failure);
    RUN_TEST(test_store_chat_success);
    return UNITY_END();
}
