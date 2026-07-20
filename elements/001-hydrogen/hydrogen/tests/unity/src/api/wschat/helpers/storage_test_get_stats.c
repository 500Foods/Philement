/*
 * Unity Test File: chat_storage_get_stats
 * This file contains unit tests for chat_storage_get_stats()
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
static void test_get_stats_null_database(void) {
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats(NULL, "h1", &u, &c, &r, &a));
}

/* NULL segment_hash */
static void test_get_stats_null_hash(void) {
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", NULL, &u, &c, &r, &a));
}

/* NULL output pointers */
static void test_get_stats_null_outputs(void) {
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", "h1", NULL, NULL, NULL, NULL));
}

/* no queue */
static void test_get_stats_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", "h1", &u, &c, &r, &a));
}

/* no connection */
static void test_get_stats_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", "h1", &u, &c, &r, &a));
}

/* query fails */
static void test_get_stats_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", "h1", &u, &c, &r, &a));
}

/* empty array */
static void test_get_stats_no_data(void) {
    mock_database_engine_set_execute_json_data("[]");
    size_t u, c; double r; int a;
    TEST_ASSERT_FALSE(chat_storage_get_stats("testdb", "h1", &u, &c, &r, &a));
}

/* success path with metadata */
static void test_get_stats_success(void) {
    mock_database_engine_set_execute_json_data(
        "[{\"uncompressed_size\": 100, \"compressed_size\": 50, "
        "\"compression_ratio\": 0.5, \"access_count\": 7}]");
    size_t u = 0, c = 0; double r = 0.0; int a = 0;
    TEST_ASSERT_TRUE(chat_storage_get_stats("testdb", "h1", &u, &c, &r, &a));
    TEST_ASSERT_EQUAL_INT(100, u);
    TEST_ASSERT_EQUAL_INT(50, c);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, r);
    TEST_ASSERT_EQUAL_INT(7, a);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_stats_null_database);
    RUN_TEST(test_get_stats_null_hash);
    RUN_TEST(test_get_stats_null_outputs);
    RUN_TEST(test_get_stats_no_queue);
    RUN_TEST(test_get_stats_no_connection);
    RUN_TEST(test_get_stats_query_failure);
    RUN_TEST(test_get_stats_no_data);
    RUN_TEST(test_get_stats_success);
    return UNITY_END();
}
