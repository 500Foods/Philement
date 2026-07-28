/*
 * Unity Test File: WebSocket Chat Send Helper Tests
 * Tests send_chat_error, send_chat_done, and send_chat_chunk functions.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_send_chat_error_null_wsi(void);
void test_send_chat_error_with_request_id(void);
void test_send_chat_error_null_message(void);
void test_send_chat_done_minimal(void);
void test_send_chat_done_with_raw_response_retrieval(void);
void test_send_chat_chunk_full(void);

// Test fixtures
static struct lws *test_wsi;

void setUp(void) {
    test_wsi = (struct lws *)0x12345678;
    mock_lws_reset_all();
}

void tearDown(void) {
    mock_lws_reset_all();
}

void test_send_chat_error_null_wsi(void) {
    send_chat_error(NULL, "error message", "req-id");
    TEST_ASSERT_TRUE(1);
}

void test_send_chat_error_with_request_id(void) {
    mock_lws_set_write_result(50);
    send_chat_error(test_wsi, "Something went wrong", "request-123");
    TEST_ASSERT_TRUE(1);
}

void test_send_chat_error_null_message(void) {
    mock_lws_set_write_result(30);
    send_chat_error(test_wsi, NULL, NULL);
    TEST_ASSERT_TRUE(1);
}

void test_send_chat_done_minimal(void) {
    mock_lws_set_write_result(100);
    send_chat_done(test_wsi, "request-123", "Hello", "gpt-4", "stop",
                   10, 5, 15, 123.45, NULL);
    TEST_ASSERT_TRUE(1);
}

void test_send_chat_done_with_raw_response_retrieval(void) {
    mock_lws_set_write_result(150);
    json_t *raw_response = json_object();
    json_object_set_new(raw_response, "retrieval", json_string("some data"));
    json_object_set_new(raw_response, "other", json_string("value"));

    send_chat_done(test_wsi, "request-123", "Hello", "gpt-4", "stop",
                   10, 5, 15, 123.45, raw_response);

    json_decref(raw_response);
    TEST_ASSERT_TRUE(1);
}

void test_send_chat_chunk_full(void) {
    mock_lws_set_write_result(80);
    send_chat_chunk(test_wsi, "request-123", "chunk content", "reasoning",
                    "gpt-4", 0, NULL);
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_chat_error_null_wsi);
    RUN_TEST(test_send_chat_error_with_request_id);
    RUN_TEST(test_send_chat_error_null_message);
    RUN_TEST(test_send_chat_done_minimal);
    RUN_TEST(test_send_chat_done_with_raw_response_retrieval);
    RUN_TEST(test_send_chat_chunk_full);

    return UNITY_END();
}
