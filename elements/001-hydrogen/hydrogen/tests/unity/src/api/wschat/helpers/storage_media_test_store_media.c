/*
 * Unity Test File: chat_storage_media
 * This file contains unit tests for media storage functions
 * in src/api/conduit/helpers/chat_storage_media.c
 *
 * Note: Most media functions require database access and cannot be fully
 * unit tested without complex mocking. This test file focuses on testing
 * parameter validation and error handling paths.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage_media.h>

// Function prototypes
void test_store_media_null_database(void);
void test_store_media_null_hash(void);
void test_store_media_null_data(void);
void test_store_media_zero_size(void);
void test_retrieve_media_null_database(void);
void test_retrieve_media_null_hash(void);
void test_retrieve_media_null_data_output(void);
void test_retrieve_media_null_size_output(void);
void test_resolve_media_null_database(void);
void test_resolve_media_null_content(void);
void test_resolve_media_null_output(void);
void test_resolve_media_invalid_json(void);
void test_resolve_media_json_object_not_array(void);
void test_resolve_media_text_content_only(void);
void test_resolve_media_non_image_url(void);
void test_resolve_media_non_media_url(void);
void test_resolve_media_missing_image_url_object(void);
void test_resolve_media_missing_url_in_image_url(void);
void test_resolve_media_empty_array(void);
void test_resolve_media_mixed_content(void);
void test_resolve_media_item_missing_type(void);
void test_resolve_media_non_object_item(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test store_media with NULL database */
void test_store_media_null_database(void) {
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media(NULL, "hash", data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Test store_media with NULL media_hash */
void test_store_media_null_hash(void) {
    const unsigned char data[] = {0x01, 0x02, 0x03};
    bool result = chat_storage_store_media("testdb", NULL, data, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Test store_media with NULL media_data */
void test_store_media_null_data(void) {
    bool result = chat_storage_store_media("testdb", "hash", NULL, 3, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Test store_media with zero size */
void test_store_media_zero_size(void) {
    const unsigned char data[] = {0x01};
    bool result = chat_storage_store_media("testdb", "hash", data, 0, "image/png");
    TEST_ASSERT_FALSE(result);
}

/* Test retrieve_media with NULL database */
void test_retrieve_media_null_database(void) {
    unsigned char* data = NULL;
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media(NULL, "hash", &data, &size, &mime);
    TEST_ASSERT_FALSE(result);
}

/* Test retrieve_media with NULL media_hash */
void test_retrieve_media_null_hash(void) {
    unsigned char* data = NULL;
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", NULL, &data, &size, &mime);
    TEST_ASSERT_FALSE(result);
}

/* Test retrieve_media with NULL media_data output */
void test_retrieve_media_null_data_output(void) {
    size_t size = 0;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", "hash", NULL, &size, &mime);
    TEST_ASSERT_FALSE(result);
}

/* Test retrieve_media with NULL size output */
void test_retrieve_media_null_size_output(void) {
    unsigned char* data = NULL;
    char* mime = NULL;
    bool result = chat_storage_retrieve_media("testdb", "hash", &data, NULL, &mime);
    TEST_ASSERT_FALSE(result);
}

/* Test resolve_media_in_content with NULL database */
void test_resolve_media_null_database(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"text\", \"text\": \"hello\"}]";

    bool result = chat_storage_resolve_media_in_content(NULL, content, &resolved, &error);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

/* Test resolve_media_in_content with NULL content_json */
void test_resolve_media_null_content(void) {
    char* resolved = NULL;
    char* error = NULL;

    bool result = chat_storage_resolve_media_in_content("testdb", NULL, &resolved, &error);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

/* Test resolve_media_in_content with NULL resolved_json output */
void test_resolve_media_null_output(void) {
    char* error = NULL;
    const char* content = "[{\"type\": \"text\", \"text\": \"hello\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, NULL, &error);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

/* Test resolve_media_in_content with invalid JSON */
void test_resolve_media_invalid_json(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "not valid json";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Invalid JSON array in content", error);
    free(error);
}

/* Test resolve_media_in_content with JSON object (not array) */
void test_resolve_media_json_object_not_array(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "{\"type\": \"text\"}";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Invalid JSON array in content", error);
    free(error);
}

/* Test resolve_media_in_content with text content (no media references) */
void test_resolve_media_text_content_only(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"text\", \"text\": \"hello world\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    /* Verify the content is preserved */
    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_TRUE(json_is_array(arr));
    TEST_ASSERT_EQUAL(1, json_array_size(arr));

    json_t* item = json_array_get(arr, 0);
    json_t* type = json_object_get(item, "type");
    TEST_ASSERT_EQUAL_STRING("text", json_string_value(type));

    json_decref(arr);
    free(resolved);
}

/* Test resolve_media_in_content with non-image_url content */
void test_resolve_media_non_image_url(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"file\", \"url\": \"http://example.com/file.txt\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test resolve_media_in_content with image_url but non-media URL */
void test_resolve_media_non_media_url(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": \"http://example.com/image.png\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    /* Verify the content is preserved */
    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);

    json_t* item = json_array_get(arr, 0);
    json_t* image_url = json_object_get(item, "image_url");
    json_t* url = json_object_get(image_url, "url");
    TEST_ASSERT_EQUAL_STRING("http://example.com/image.png", json_string_value(url));

    json_decref(arr);
    free(resolved);
}

/* Test resolve_media_in_content with missing image_url object */
void test_resolve_media_missing_image_url_object(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test resolve_media_in_content with missing URL in image_url */
void test_resolve_media_missing_url_in_image_url(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test resolve_media_in_content with empty array */
void test_resolve_media_empty_array(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(0, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

/* Test resolve_media_in_content with mixed content types */
void test_resolve_media_mixed_content(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"text\", \"text\": \"hello\"},"
        "{\"type\": \"file\", \"url\": \"http://example.com/file.txt\"},"
        "{\"type\": \"text\", \"text\": \"world\"}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(3, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

/* Test resolve_media_in_content with item missing type field */
void test_resolve_media_item_missing_type(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"text\": \"no type field\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test resolve_media_in_content with non-object item in array */
void test_resolve_media_non_object_item(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[\"string item\", 123, true]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(3, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

int main(void) {
    UNITY_BEGIN();

    /* Parameter validation tests */
    RUN_TEST(test_store_media_null_database);
    RUN_TEST(test_store_media_null_hash);
    RUN_TEST(test_store_media_null_data);
    RUN_TEST(test_store_media_zero_size);

    RUN_TEST(test_retrieve_media_null_database);
    RUN_TEST(test_retrieve_media_null_hash);
    RUN_TEST(test_retrieve_media_null_data_output);
    RUN_TEST(test_retrieve_media_null_size_output);

    RUN_TEST(test_resolve_media_null_database);
    RUN_TEST(test_resolve_media_null_content);
    RUN_TEST(test_resolve_media_null_output);
    RUN_TEST(test_resolve_media_invalid_json);
    RUN_TEST(test_resolve_media_json_object_not_array);

    /* Content handling tests */
    RUN_TEST(test_resolve_media_text_content_only);
    RUN_TEST(test_resolve_media_non_image_url);
    RUN_TEST(test_resolve_media_non_media_url);
    RUN_TEST(test_resolve_media_missing_image_url_object);
    RUN_TEST(test_resolve_media_missing_url_in_image_url);
    RUN_TEST(test_resolve_media_empty_array);
    RUN_TEST(test_resolve_media_mixed_content);
    RUN_TEST(test_resolve_media_item_missing_type);
    RUN_TEST(test_resolve_media_non_object_item);

    return UNITY_END();
}
