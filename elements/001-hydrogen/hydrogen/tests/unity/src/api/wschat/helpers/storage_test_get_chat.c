/*
 * Unity Test File: chat_storage_get_chat
 * This file contains unit tests for chat_storage_get_chat()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage.h>

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
static void test_get_chat_null_database(void) {
    json_t* result = chat_storage_get_chat(NULL, 1);
    TEST_ASSERT_NULL(result);
}

/* convos_id <= 0 */
static void test_get_chat_invalid_id(void) {
    json_t* result = chat_storage_get_chat("testdb", 0);
    TEST_ASSERT_NULL(result);
}

/* no queue */
static void test_get_chat_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NULL(result);
}

/* no connection */
static void test_get_chat_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NULL(result);
}

/* query fails */
static void test_get_chat_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NULL(result);
}

/* empty array */
static void test_get_chat_no_data(void) {
    mock_database_engine_set_execute_json_data("[]");
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NULL(result);
}

/* segments field missing -> returns empty object */
static void test_get_chat_no_segments_object(void) {
    mock_database_engine_set_execute_json_data("[{\"convos_id\": 1}]");
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    json_decref(result);
}

/* segments present but not an array -> returns empty object */
static void test_get_chat_segments_not_array(void) {
    mock_database_engine_set_execute_json_data("[{\"segments\": \"nope\"}]");
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    json_decref(result);
}

/* happy path with one decompressible segment */
static void test_get_chat_success(void) {
    char* hex = build_compressed_hex("hello from chat");
    TEST_ASSERT_NOT_NULL(hex);
    char buf[2048];
    snprintf(buf, sizeof(buf),
             "[{\"segments\": [{\"segment_content\": \"%s\"}]}]", hex);
    free(hex);

    mock_database_engine_set_execute_json_data(buf);
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_array(result));
    TEST_ASSERT_EQUAL_INT(1, json_array_size(result));
    TEST_ASSERT_EQUAL_STRING("hello from chat",
                             json_string_value(json_array_get(result, 0)));
    json_decref(result);
}

/* segment entry missing content -> skipped, returns empty array */
static void test_get_chat_skips_invalid_segment(void) {
    mock_database_engine_set_execute_json_data(
        "[{\"segments\": [{\"segment_content\": 123}]}]");
    json_t* result = chat_storage_get_chat("testdb", 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_array(result));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(result));
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_chat_null_database);
    RUN_TEST(test_get_chat_invalid_id);
    RUN_TEST(test_get_chat_no_queue);
    RUN_TEST(test_get_chat_no_connection);
    RUN_TEST(test_get_chat_query_failure);
    RUN_TEST(test_get_chat_no_data);
    RUN_TEST(test_get_chat_no_segments_object);
    RUN_TEST(test_get_chat_segments_not_array);
    RUN_TEST(test_get_chat_success);
    RUN_TEST(test_get_chat_skips_invalid_segment);
    return UNITY_END();
}
