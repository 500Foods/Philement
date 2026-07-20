/*
 * Unity Test File: chat_storage_retrieve_media
 * This file contains unit tests for chat_storage_retrieve_media()
 * in src/api/wschat/helpers/storage_media.c, exercising the database
 * interaction and JSON parsing paths that were previously uncovered.
 *
 * Uses mock_dbqueue to provide a DatabaseQueue and mock_database_engine
 * to control the query result returned via chat_storage_execute_query.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage_media.h>
#include <src/api/wschat/helpers/storage_hex.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

/* Build a JSON array row with a valid hex media_data payload */
static char* build_retrieve_result(const char* hex, const char* mime, size_t size) {
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "[{\"media_data\": \"%s\", \"mime_type\": \"%s\", \"media_size\": %zu}]",
             hex, mime, size);
    return strdup(buf);
}

/* Hex encoding of the bytes {0x89, 0x50, 0x4e, 0x47} ("PNG" magic) */
static const char* SAMPLE_HEX = "89504e47";
static const unsigned char SAMPLE_BYTES[] = {0x89, 0x50, 0x4e, 0x47};

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
static void test_retrieve_media_no_queue(void) {
    mock_dbqueue_set_get_database_result(NULL);
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Queue present but no persistent connection */
static void test_retrieve_media_no_connection(void) {
    g_dbq->persistent_connection = NULL;
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Query fails (engine returns failure) */
static void test_retrieve_media_query_failure(void) {
    mock_database_engine_set_execute_result(false);
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Query succeeds but returns NULL result_json */
static void test_retrieve_media_null_result(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data(NULL);
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Result JSON is not an array */
static void test_retrieve_media_result_not_array(void) {
    mock_database_engine_set_execute_json_data("{\"not\": \"array\"}");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Result array is empty */
static void test_retrieve_media_empty_array(void) {
    mock_database_engine_set_execute_json_data("[]");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* First row is not an object */
static void test_retrieve_media_row_not_object(void) {
    mock_database_engine_set_execute_json_data("[123]");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Row missing media_data field */
static void test_retrieve_media_missing_media_data(void) {
    mock_database_engine_set_execute_json_data("[{\"mime_type\": \"image/png\", \"media_size\": 4}]");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* media_data is not a string */
static void test_retrieve_media_media_data_not_string(void) {
    mock_database_engine_set_execute_json_data("[{\"media_data\": 123, \"media_size\": 4}]");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* media_data hex is invalid (cannot convert) */
static void test_retrieve_media_invalid_hex(void) {
    mock_database_engine_set_execute_json_data("[{\"media_data\": \"zzzz\", \"media_size\": 4}]");
    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Happy path: media_data, mime_type and media_size all populated */
static void test_retrieve_media_success(void) {
    char* result_json = build_retrieve_result(SAMPLE_HEX, "image/png", sizeof(SAMPLE_BYTES));
    TEST_ASSERT_NOT_NULL(result_json);
    mock_database_engine_set_execute_json_data(result_json);
    free(result_json);

    unsigned char* data = NULL;
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, &mime);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(sizeof(SAMPLE_BYTES), size);
    TEST_ASSERT_EQUAL_MEMORY(SAMPLE_BYTES, data, sizeof(SAMPLE_BYTES));
    TEST_ASSERT_NOT_NULL(mime);
    TEST_ASSERT_EQUAL_STRING("image/png", mime);

    free(data);
    free(mime);
}

/* Success but caller does not request mime_type (must be freed internally) */
static void test_retrieve_media_success_no_mime_requested(void) {
    char* result_json = build_retrieve_result(SAMPLE_HEX, "image/png", sizeof(SAMPLE_BYTES));
    TEST_ASSERT_NOT_NULL(result_json);
    mock_database_engine_set_execute_json_data(result_json);
    free(result_json);

    unsigned char* data = NULL;
    size_t size = 0;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(sizeof(SAMPLE_BYTES), size);
    free(data);
}

/* Row present but mime_type field missing -> mime stays NULL */
static void test_retrieve_media_missing_mime(void) {
    char* result_json = build_retrieve_result(SAMPLE_HEX, "", sizeof(SAMPLE_BYTES));
    /* Rewrite mime_type to be absent */
    free(result_json);
    result_json = strdup("[{\"media_data\": \"89504e47\", \"media_size\": 4}]");
    mock_database_engine_set_execute_json_data(result_json);
    free(result_json);

    unsigned char* data = NULL;
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, &mime);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_NULL(mime);
    free(data);
}

/* Row present but media_size missing -> size stays 0 */
static void test_retrieve_media_missing_size(void) {
    char* result_json = strdup("[{\"media_data\": \"89504e47\", \"mime_type\": \"image/png\"}]");
    mock_database_engine_set_execute_json_data(result_json);
    free(result_json);

    unsigned char* data = NULL;
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, &size, &mime);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(0, size);
    TEST_ASSERT_NOT_NULL(mime);
    free(data);
    free(mime);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_retrieve_media_no_queue);
    RUN_TEST(test_retrieve_media_no_connection);
    RUN_TEST(test_retrieve_media_query_failure);
    RUN_TEST(test_retrieve_media_null_result);
    RUN_TEST(test_retrieve_media_result_not_array);
    RUN_TEST(test_retrieve_media_empty_array);
    RUN_TEST(test_retrieve_media_row_not_object);
    RUN_TEST(test_retrieve_media_missing_media_data);
    RUN_TEST(test_retrieve_media_media_data_not_string);
    RUN_TEST(test_retrieve_media_invalid_hex);
    RUN_TEST(test_retrieve_media_success);
    RUN_TEST(test_retrieve_media_success_no_mime_requested);
    RUN_TEST(test_retrieve_media_missing_mime);
    RUN_TEST(test_retrieve_media_missing_size);
    return UNITY_END();
}
