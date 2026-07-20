/*
 * Unity Test File: chat_storage_resolve_media_in_content coverage
 * This file contains additional unit tests for chat_storage_resolve_media_in_content()
 * in src/api/wschat/helpers/storage_media.c, exercising the media retrieval
 * and data-URL construction paths that were previously uncovered.
 *
 * Uses mock_dbqueue to provide a DatabaseQueue and mock_database_engine to
 * control chat_storage_retrieve_media's underlying query result.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage_media.h>

static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

static const char* SAMPLE_HEX = "89504e47";
static const char* SAMPLE_MIME = "image/png";

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();

    g_handle = calloc(1, sizeof(DatabaseHandle));
    g_dbq = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_handle);
    TEST_ASSERT_NOT_NULL(g_dbq);
    g_dbq->persistent_connection = g_handle;
    mock_dbqueue_set_get_database_result(g_dbq);

    /* Default: retrieve_media finds the media asset */
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "[{\"media_data\": \"%s\", \"mime_type\": \"%s\", \"media_size\": 4}]",
             SAMPLE_HEX, SAMPLE_MIME);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data(buf);
}

void tearDown(void) {
    free(g_dbq);
    free(g_handle);
    g_dbq = NULL;
    g_handle = NULL;
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
}

/* Single media: reference resolved into a base64 data URL */
static void test_resolve_media_single_image_resolved(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:89504e47\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    /* The data URL must use the stored mime type and base64 payload */
    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    json_t* item = json_array_get(arr, 0);
    json_t* image_url = json_object_get(item, "image_url");
    json_t* url = json_object_get(image_url, "url");
    const char* url_str = json_string_value(url);
    TEST_ASSERT_EQUAL(0, strncmp(url_str, "data:image/png;base64,", 20));

    json_decref(arr);
    free(resolved);
}

/* Resolve with no mime_type stored -> defaults to octet-stream */
static void test_resolve_media_default_mime_when_missing(void) {
    /* Override result so mime_type is absent */
    mock_database_engine_set_execute_json_data("[{\"media_data\": \"89504e47\", \"media_size\": 4}]");

    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:89504e47\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    json_t* item = json_array_get(arr, 0);
    json_t* image_url = json_object_get(item, "image_url");
    json_t* url = json_object_get(image_url, "url");
    const char* url_str = json_string_value(url);
    TEST_ASSERT_EQUAL(0, strncmp(url_str, "data:application/octet-stream;base64,", 33));

    json_decref(arr);
    free(resolved);
}

/* Mixed content: one text item plus one media reference */
static void test_resolve_media_mixed_with_media(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"text\", \"text\": \"hi\"},"
        "{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:89504e47\"}}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_EQUAL_INT(2, json_array_size(arr));
    json_t* second = json_array_get(arr, 1);
    json_t* image_url = json_object_get(second, "image_url");
    json_t* url = json_object_get(image_url, "url");
    TEST_ASSERT_EQUAL(0, strncmp(json_string_value(url), "data:", 5));

    json_decref(arr);
    free(resolved);
}

/* Media hash not found -> original reference preserved */
static void test_resolve_media_not_found_preserved(void) {
    mock_database_engine_set_execute_json_data("[]");

    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:deadbeef\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    json_t* item = json_array_get(arr, 0);
    json_t* image_url = json_object_get(item, "image_url");
    json_t* url = json_object_get(image_url, "url");
    TEST_ASSERT_EQUAL_STRING("media:deadbeef", json_string_value(url));

    json_decref(arr);
    free(resolved);
}

/* Two media references both resolved into data URLs */
static void test_resolve_media_multiple_images_resolved(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:89504e47\"}},"
        "{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:89504e47\"}}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_EQUAL_INT(2, json_array_size(arr));
    for (size_t i = 0; i < 2; i++) {
        json_t* item = json_array_get(arr, i);
        json_t* image_url = json_object_get(item, "image_url");
        json_t* url = json_object_get(image_url, "url");
        TEST_ASSERT_EQUAL(0, strncmp(json_string_value(url), "data:image/png;base64,", 20));
    }

    json_decref(arr);
    free(resolved);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_resolve_media_single_image_resolved);
    RUN_TEST(test_resolve_media_default_mime_when_missing);
    RUN_TEST(test_resolve_media_mixed_with_media);
    RUN_TEST(test_resolve_media_not_found_preserved);
    RUN_TEST(test_resolve_media_multiple_images_resolved);
    return UNITY_END();
}
