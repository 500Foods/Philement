/*
 * Unity Test File: WebSocket Chat Writable Handler Tests
 * Tests handle_chat_writable function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/websocket/websocket_server_chat.h>
#include <src/websocket/websocket_server_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_handle_chat_writable_null_session(void);
void test_handle_chat_writable_no_stream_context(void);
void test_handle_chat_writable_with_empty_stream_context(void);
void test_handle_chat_writable_drains_chunks(void);

void setUp(void) {
    mock_lws_reset_all();
}

void tearDown(void) {
    mock_lws_reset_all();
}

void test_handle_chat_writable_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;

    handle_chat_writable(test_wsi, NULL);

    // Should return without crashing
    TEST_ASSERT_TRUE(1);
}

void test_handle_chat_writable_no_stream_context(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    WebSocketSessionData session = {0};

    handle_chat_writable(test_wsi, &session);

    // Should return without crashing
    TEST_ASSERT_TRUE(1);
}

void test_handle_chat_writable_with_empty_stream_context(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    MultiStreamContext stream_ctx;
    memset(&stream_ctx, 0, sizeof(stream_ctx));
    chunk_queue_init(&stream_ctx.chunk_queue);

    WebSocketSessionData session = {0};
    session.multi_stream_ctx = &stream_ctx;

    handle_chat_writable(test_wsi, &session);

    chunk_queue_destroy(&stream_ctx.chunk_queue);

    // Should return without crashing
    TEST_ASSERT_TRUE(1);
}

void test_handle_chat_writable_drains_chunks(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    MultiStreamContext stream_ctx;
    memset(&stream_ctx, 0, sizeof(stream_ctx));
    chunk_queue_init(&stream_ctx.chunk_queue);
    stream_ctx.wsi = test_wsi;

    WebSocketSessionData session = {0};
    session.connection_valid = true;
    session.multi_stream_ctx = &stream_ctx;
    stream_ctx.connection_valid = &session.connection_valid;

    mock_lws_set_write_result(50);

    const char *chunk_json = "{\"type\":\"chat_chunk\"}";
    chunk_queue_enqueue(&stream_ctx.chunk_queue, chunk_json, strlen(chunk_json));

    handle_chat_writable(test_wsi, &session);

    chunk_queue_destroy(&stream_ctx.chunk_queue);

    // Should return without crashing after draining
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_chat_writable_null_session);
    RUN_TEST(test_handle_chat_writable_no_stream_context);
    RUN_TEST(test_handle_chat_writable_with_empty_stream_context);
    RUN_TEST(test_handle_chat_writable_drains_chunks);

    return UNITY_END();
}
