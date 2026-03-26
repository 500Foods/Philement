/*
 * Unity Test File: chat_storage_media hex operations
 * This file contains additional unit tests for hex conversion paths
 * in src/api/conduit/chat/chat_storage_media.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage_media.h>

// Function prototypes
void test_resolve_media_with_media_url_empty_hash(void);
void test_resolve_media_image_url_without_url_field(void);
void test_resolve_media_image_url_with_null_url(void);
void test_resolve_media_multiple_items_mixed_types(void);
void test_resolve_media_item_with_empty_type(void);
void test_resolve_media_image_url_with_numeric_url(void);
void test_resolve_media_item_with_type_not_string(void);
void test_resolve_media_with_only_media_url_items(void);
void test_resolve_media_with_empty_image_url_object(void);
void test_resolve_media_preserves_non_image_items(void);
void test_resolve_media_multiple_non_image_types(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test with media: URL but empty hash portion */
void test_resolve_media_with_media_url_empty_hash(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    /* Should succeed but keep original since media: has empty hash */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test image_url type without url field */
void test_resolve_media_image_url_without_url_field(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"alt\": \"no url\"}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test image_url with null url value */
void test_resolve_media_image_url_with_null_url(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test multiple items with mixed types */
void test_resolve_media_multiple_items_mixed_types(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"text\", \"text\": \"first\"},"
        "{\"type\": \"image\", \"url\": \"http://example.com/img.png\"},"
        "{\"type\": \"text\", \"text\": \"second\"},"
        "{\"type\": \"audio\", \"url\": \"http://example.com/audio.mp3\"}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(4, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

/* Test item with empty type string */
void test_resolve_media_item_with_empty_type(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"\", \"data\": \"test\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test image_url with non-string url (e.g., number) */
void test_resolve_media_image_url_with_numeric_url(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {\"url\": 12345}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test item where type is not a string */
void test_resolve_media_item_with_type_not_string(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": 123, \"data\": \"test\"}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test with only media: URL items */
void test_resolve_media_with_only_media_url_items(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:abc123\"}},"
        "{\"type\": \"image_url\", \"image_url\": {\"url\": \"media:def456\"}}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    /* Will succeed but media retrieval will fail, keeping originals */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(2, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

/* Test with empty image_url object */
void test_resolve_media_with_empty_image_url_object(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "[{\"type\": \"image_url\", \"image_url\": {}}]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);
    free(resolved);
}

/* Test that non-image items are preserved */
void test_resolve_media_preserves_non_image_items(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"text\", \"text\": \"hello\", \"extra\": \"field\"},"
        "{\"type\": \"video\", \"url\": \"http://example.com/video.mp4\", \"duration\": 120}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);

    /* Check first item preserved extra field */
    json_t* item1 = json_array_get(arr, 0);
    json_t* extra = json_object_get(item1, "extra");
    TEST_ASSERT_NOT_NULL(extra);
    TEST_ASSERT_EQUAL_STRING("field", json_string_value(extra));

    /* Check second item preserved duration field */
    json_t* item2 = json_array_get(arr, 1);
    json_t* duration = json_object_get(item2, "duration");
    TEST_ASSERT_NOT_NULL(duration);
    TEST_ASSERT_EQUAL(120, json_integer_value(duration));

    json_decref(arr);
    free(resolved);
}

/* Test with multiple non-image type items */
void test_resolve_media_multiple_non_image_types(void) {
    char* resolved = NULL;
    char* error = NULL;
    const char* content = "["
        "{\"type\": \"audio\"},"
        "{\"type\": \"video\"},"
        "{\"type\": \"document\"},"
        "{\"type\": \"file\"}"
    "]";

    bool result = chat_storage_resolve_media_in_content("testdb", content, &resolved, &error);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(resolved);

    json_error_t json_error;
    json_t* arr = json_loads(resolved, 0, &json_error);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(4, json_array_size(arr));

    json_decref(arr);
    free(resolved);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_media_with_media_url_empty_hash);
    RUN_TEST(test_resolve_media_image_url_without_url_field);
    RUN_TEST(test_resolve_media_image_url_with_null_url);
    RUN_TEST(test_resolve_media_multiple_items_mixed_types);
    RUN_TEST(test_resolve_media_item_with_empty_type);
    RUN_TEST(test_resolve_media_image_url_with_numeric_url);
    RUN_TEST(test_resolve_media_item_with_type_not_string);
    RUN_TEST(test_resolve_media_with_only_media_url_items);
    RUN_TEST(test_resolve_media_with_empty_image_url_object);
    RUN_TEST(test_resolve_media_preserves_non_image_items);
    RUN_TEST(test_resolve_media_multiple_non_image_types);

    return UNITY_END();
}
