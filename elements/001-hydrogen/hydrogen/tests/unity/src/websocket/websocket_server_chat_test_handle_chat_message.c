/*
 * Unity Test File: WebSocket Chat Message Handler Tests
 * Tests handle_chat_message error paths
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat.h>
#include <src/websocket/websocket_server_internal.h>

// Mock libwebsockets for testing
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_handle_chat_message_null_wsi(void);
void test_handle_chat_message_null_session(void);
void test_handle_chat_message_null_request(void);
void test_handle_chat_message_missing_payload(void);
void test_handle_chat_message_invalid_payload_type(void);
void test_handle_chat_message_valid_payload_missing_messages(void);
void test_handle_chat_message_no_jwt_and_no_database(void);
void test_handle_chat_message_database_not_found(void);

// Test fixtures
static WebSocketSessionData test_session;

void setUp(void) {
    memset(&test_session, 0, sizeof(test_session));
    mock_lws_reset_all();
}

void tearDown(void) {
    if (test_session.chat_database) {
        free(test_session.chat_database);
        test_session.chat_database = NULL;
    }
    mock_lws_reset_all();
}

void test_handle_chat_message_null_wsi(void) {
    json_t *request = json_object();

    int result = handle_chat_message(NULL, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();

    int result = handle_chat_message(test_wsi, NULL, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_null_request(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;

    int result = handle_chat_message(test_wsi, &test_session, NULL);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_missing_payload(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();

    int result = handle_chat_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_invalid_payload_type(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();

    json_object_set_new(request, "payload", json_string("not an object"));

    int result = handle_chat_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_valid_payload_missing_messages(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();
    json_t *payload = json_object();

    json_object_set_new(request, "payload", payload);

    int result = handle_chat_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_no_jwt_and_no_database(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();

    json_array_append_new(messages, json_object());
    json_object_set_new(payload, "messages", messages);
    json_object_set_new(request, "payload", payload);

    int result = handle_chat_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_database_not_found(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.chat_database = strdup("test_database");

    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();

    json_array_append_new(messages, json_object());
    json_object_set_new(payload, "messages", messages);
    json_object_set_new(request, "payload", payload);

    int result = handle_chat_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_chat_message_null_wsi);
    RUN_TEST(test_handle_chat_message_null_session);
    RUN_TEST(test_handle_chat_message_null_request);
    RUN_TEST(test_handle_chat_message_missing_payload);
    RUN_TEST(test_handle_chat_message_invalid_payload_type);
    RUN_TEST(test_handle_chat_message_valid_payload_missing_messages);
    RUN_TEST(test_handle_chat_message_no_jwt_and_no_database);
    RUN_TEST(test_handle_chat_message_database_not_found);

    return UNITY_END();
}
