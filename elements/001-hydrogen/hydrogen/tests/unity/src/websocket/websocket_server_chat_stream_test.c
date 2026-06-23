/*
 * Unity Test File: WebSocket Chat Stream Callback Tests
 * Tests stream_chunk_callback function.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_stream_chunk_callback_null_chunk(void);
void test_stream_chunk_callback_null_context(void);
void test_stream_chunk_callback_null_wsi(void);
void test_stream_chunk_callback_connection_invalid(void);
void test_stream_chunk_callback_intermediate_chunk(void);
void test_stream_chunk_callback_done_chunk(void);
void test_stream_chunk_callback_done_chunk_no_finish_reason(void);
void test_stream_chunk_callback_updates_model(void);

// Test fixtures
static StreamContext *test_ctx = NULL;
static bool connection_valid;
static bool stream_active;

void setUp(void) {
    test_ctx = (StreamContext*)calloc(1, sizeof(StreamContext));
    TEST_ASSERT_NOT_NULL(test_ctx);
    test_ctx->wsi = (struct lws *)0x12345678;
    connection_valid = true;
    stream_active = true;
    test_ctx->connection_valid = &connection_valid;
    test_ctx->stream_active = &stream_active;
    clock_gettime(CLOCK_MONOTONIC, &test_ctx->start_time);
    mock_lws_reset_all();
}

void tearDown(void) {
    if (test_ctx) {
        free(test_ctx->request_id);
        free(test_ctx->model);
        free(test_ctx->finish_reason);
        free(test_ctx);
        test_ctx = NULL;
    }
    mock_lws_reset_all();
}

void test_stream_chunk_callback_null_chunk(void) {
    stream_chunk_callback(NULL, test_ctx);
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_null_context(void) {
    ChatStreamChunk chunk = {0};
    stream_chunk_callback(&chunk, NULL);
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_null_wsi(void) {
    ChatStreamChunk chunk = {0};
    test_ctx->wsi = NULL;
    stream_chunk_callback(&chunk, test_ctx);
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_connection_invalid(void) {
    ChatStreamChunk chunk = {0};
    connection_valid = false;
    stream_chunk_callback(&chunk, test_ctx);
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_intermediate_chunk(void) {
    ChatStreamChunk chunk = {0};
    char content[] = "hello";
    char reasoning[] = "thinking";
    char model[] = "gpt-4";
    chunk.is_done = false;
    chunk.content = content;
    chunk.reasoning_content = reasoning;
    chunk.model = model;

    mock_lws_set_write_result(50);

    stream_chunk_callback(&chunk, test_ctx);

    TEST_ASSERT_TRUE(test_ctx->first_chunk_logged);
    TEST_ASSERT_EQUAL_INT(1, test_ctx->chunk_index);
}

void test_stream_chunk_callback_done_chunk(void) {
    ChatStreamChunk chunk = {0};
    char finish_reason[] = "length";
    chunk.is_done = true;
    chunk.finish_reason = finish_reason;

    mock_lws_set_write_result(40);

    stream_chunk_callback(&chunk, test_ctx);

    // Context is freed in callback on done, so don't access it
    test_ctx = NULL;
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_done_chunk_no_finish_reason(void) {
    ChatStreamChunk chunk = {0};
    chunk.is_done = true;
    chunk.finish_reason = NULL;

    mock_lws_set_write_result(40);

    stream_chunk_callback(&chunk, test_ctx);

    test_ctx = NULL;
    TEST_ASSERT_TRUE(1);
}

void test_stream_chunk_callback_updates_model(void) {
    ChatStreamChunk chunk = {0};
    char content[] = "content";
    char model[] = "updated-model";
    chunk.is_done = false;
    chunk.content = content;
    chunk.model = model;

    mock_lws_set_write_result(50);

    stream_chunk_callback(&chunk, test_ctx);

    TEST_ASSERT_EQUAL_STRING("updated-model", test_ctx->model);
    TEST_ASSERT_EQUAL_INT(1, test_ctx->chunk_index);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_stream_chunk_callback_null_chunk);
    RUN_TEST(test_stream_chunk_callback_null_context);
    RUN_TEST(test_stream_chunk_callback_null_wsi);
    RUN_TEST(test_stream_chunk_callback_connection_invalid);
    RUN_TEST(test_stream_chunk_callback_intermediate_chunk);
    RUN_TEST(test_stream_chunk_callback_done_chunk);
    RUN_TEST(test_stream_chunk_callback_done_chunk_no_finish_reason);
    RUN_TEST(test_stream_chunk_callback_updates_model);

    return UNITY_END();
}
