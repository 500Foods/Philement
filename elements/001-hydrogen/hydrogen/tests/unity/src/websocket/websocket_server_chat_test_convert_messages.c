/*
 * Unity Test File: WebSocket Chat Message Conversion Tests
 * Tests convert_json_messages_to_chat_messages helper function.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat_internal.h>

// Test function prototypes
void test_convert_json_messages_null(void);
void test_convert_json_messages_not_array(void);
void test_convert_json_messages_empty_array(void);
void test_convert_json_messages_valid_string_content(void);
void test_convert_json_messages_array_content(void);
void test_convert_json_messages_skips_invalid_entries(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_convert_json_messages_null(void) {
    ChatMessage *result = convert_json_messages_to_chat_messages(NULL);
    TEST_ASSERT_NULL(result);
}

void test_convert_json_messages_not_array(void) {
    json_t *obj = json_object();
    ChatMessage *result = convert_json_messages_to_chat_messages(obj);
    TEST_ASSERT_NULL(result);
    json_decref(obj);
}

void test_convert_json_messages_empty_array(void) {
    json_t *arr = json_array();
    ChatMessage *result = convert_json_messages_to_chat_messages(arr);
    TEST_ASSERT_NULL(result);
    json_decref(arr);
}

void test_convert_json_messages_valid_string_content(void) {
    json_t *arr = json_array();
    json_t *msg = json_object();
    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string("hello"));
    json_array_append_new(arr, msg);

    ChatMessage *result = convert_json_messages_to_chat_messages(arr);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, result->role);
    TEST_ASSERT_EQUAL_STRING("hello", result->content);

    chat_message_list_destroy(result);
    json_decref(arr);
}

void test_convert_json_messages_array_content(void) {
    json_t *arr = json_array();
    json_t *msg = json_object();
    json_t *content_arr = json_array();
    json_t *text_obj = json_object();
    json_object_set_new(text_obj, "type", json_string("text"));
    json_object_set_new(text_obj, "text", json_string("image prompt"));
    json_array_append_new(content_arr, text_obj);

    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", content_arr);
    json_array_append_new(arr, msg);

    ChatMessage *result = convert_json_messages_to_chat_messages(arr);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, result->role);
    TEST_ASSERT_NOT_NULL(result->content);

    chat_message_list_destroy(result);
    json_decref(arr);
}

void test_convert_json_messages_skips_invalid_entries(void) {
    json_t *arr = json_array();

    // Missing role
    json_t *bad1 = json_object();
    json_object_set_new(bad1, "content", json_string("no role"));
    json_array_append_new(arr, bad1);

    // Not an object
    json_array_append_new(arr, json_string("not object"));

    // Valid message
    json_t *good = json_object();
    json_object_set_new(good, "role", json_string("assistant"));
    json_object_set_new(good, "content", json_string("valid"));
    json_array_append_new(arr, good);

    ChatMessage *result = convert_json_messages_to_chat_messages(arr);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_ASSISTANT, result->role);
    TEST_ASSERT_EQUAL_STRING("valid", result->content);

    chat_message_list_destroy(result);
    json_decref(arr);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_convert_json_messages_null);
    RUN_TEST(test_convert_json_messages_not_array);
    RUN_TEST(test_convert_json_messages_empty_array);
    RUN_TEST(test_convert_json_messages_valid_string_content);
    RUN_TEST(test_convert_json_messages_array_content);
    RUN_TEST(test_convert_json_messages_skips_invalid_entries);

    return UNITY_END();
}
