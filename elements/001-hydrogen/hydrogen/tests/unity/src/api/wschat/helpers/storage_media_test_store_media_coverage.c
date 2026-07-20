/*
 * Unity Test File: chat_storage_store_media coverage
 * This file contains additional unit tests for chat_storage_store_media()
 * in src/api/wschat/helpers/storage_media.c, exercising the database
 * interaction and hex-encoding error paths that were previously uncovered.
 *
 * Uses mock_dbqueue to provide a DatabaseQueue and mock_database_engine
 * to control the query result returned via chat_storage_execute_query.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/api/wschat/helpers/storage_media.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();

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
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
}

/* No database queue available */
static void test_store_media_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", "hash", data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Queue present but no persistent connection */
static void test_store_media_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", "hash", data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/*
 * Hex conversion failure. chat_storage_binary_to_hex allocates len*2+1 bytes;
 * forcing malloc to fail via the global system mock makes it return NULL.
 */
static void test_store_media_hex_allocation_failure(void) {
    mock_system_set_malloc_failure(1);
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", "hash", data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Query execution fails */
static void test_store_media_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", "hash", data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Happy path: query succeeds, NULL mime_type provided */
static void test_store_media_success(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("{\"status\": \"ok\"}");
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", "hash", data, 3, NULL);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_store_media_no_queue);
    RUN_TEST(test_store_media_no_connection);
    RUN_TEST(test_store_media_hex_allocation_failure);
    RUN_TEST(test_store_media_query_failure);
    RUN_TEST(test_store_media_success);
    return UNITY_END();
}
