/*
 * Unity Test File: rest_sse_callback_thread
 * This file contains unit tests for rest_sse_callback_thread() in
 * src/api/wschat/auth_chat/auth_chat_sse.c
 *
 * CHANGELOG:
 * 2026-09-05: Initial coverage for SSE callback drain/write paths
 *
 * TEST_VERSION: 1.0.0
 */

#define USE_MOCK_SYSTEM

#include <src/hydrogen.h>
#include <unity.h>
#include <tests/unity/mocks/mock_system.h>

#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/proxy_multi.h>

void test_callback_thread_null_ctx(void);
void test_callback_thread_null_stream(void);
void test_callback_thread_empty_completed(void);
void test_callback_thread_writes_sse_data(void);
void test_callback_thread_skips_empty_json(void);
void test_callback_thread_write_fail(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_callback_thread_null_ctx(void) {
    TEST_ASSERT_NULL(rest_sse_callback_thread(NULL));
}

void test_callback_thread_null_stream(void) {
    RestSseContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.stream_ctx = NULL;
    TEST_ASSERT_NULL(rest_sse_callback_thread(&ctx));
}

void test_callback_thread_empty_completed(void) {
    RestSseContext ctx;
    MultiStreamContext sc;

    memset(&ctx, 0, sizeof(ctx));
    memset(&sc, 0, sizeof(sc));
    chunk_queue_init(&sc.chunk_queue);
    sc.stream_completed = true;
    ctx.stream_ctx = &sc;
    ctx.pipe_read = -1;
    ctx.pipe_write = 3;

    TEST_ASSERT_NULL(rest_sse_callback_thread(&ctx));
    TEST_ASSERT_TRUE(ctx.callback_done);
    TEST_ASSERT_EQUAL_INT(-1, ctx.pipe_write);

    chunk_queue_destroy(&sc.chunk_queue);
}

void test_callback_thread_writes_sse_data(void) {
    RestSseContext ctx;
    MultiStreamContext sc;

    memset(&ctx, 0, sizeof(ctx));
    memset(&sc, 0, sizeof(sc));
    chunk_queue_init(&sc.chunk_queue);
    TEST_ASSERT_TRUE(chunk_queue_enqueue(&sc.chunk_queue, "{\"x\":1}", 7));
    TEST_ASSERT_TRUE(chunk_queue_has_data(&sc.chunk_queue));
    sc.stream_completed = true;
    ctx.stream_ctx = &sc;
    ctx.pipe_read = -1;
    ctx.pipe_write = 3;
    mock_system_set_write_result(32);

    TEST_ASSERT_NULL(rest_sse_callback_thread(&ctx));
    TEST_ASSERT_TRUE(ctx.callback_done);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&sc.chunk_queue));

    chunk_queue_destroy(&sc.chunk_queue);
}

void test_callback_thread_skips_empty_json(void) {
    RestSseContext ctx;
    MultiStreamContext sc;

    memset(&ctx, 0, sizeof(ctx));
    memset(&sc, 0, sizeof(sc));
    chunk_queue_init(&sc.chunk_queue);
    TEST_ASSERT_TRUE(chunk_queue_enqueue(&sc.chunk_queue, "", 0));
    sc.stream_completed = true;
    ctx.stream_ctx = &sc;
    ctx.pipe_read = -1;
    ctx.pipe_write = 3;
    mock_system_set_write_result(32);

    TEST_ASSERT_NULL(rest_sse_callback_thread(&ctx));
    TEST_ASSERT_TRUE(ctx.callback_done);

    chunk_queue_destroy(&sc.chunk_queue);
}

void test_callback_thread_write_fail(void) {
    RestSseContext ctx;
    MultiStreamContext sc;

    memset(&ctx, 0, sizeof(ctx));
    memset(&sc, 0, sizeof(sc));
    chunk_queue_init(&sc.chunk_queue);
    TEST_ASSERT_TRUE(chunk_queue_enqueue(&sc.chunk_queue, "{\"y\":2}", 7));
    sc.stream_completed = true;
    ctx.stream_ctx = &sc;
    ctx.pipe_read = -1;
    ctx.pipe_write = 3;
    mock_system_set_write_should_fail(1);

    TEST_ASSERT_NULL(rest_sse_callback_thread(&ctx));
    TEST_ASSERT_TRUE(ctx.callback_done);

    chunk_queue_destroy(&sc.chunk_queue);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_callback_thread_null_ctx);
    RUN_TEST(test_callback_thread_null_stream);
    RUN_TEST(test_callback_thread_empty_completed);
    RUN_TEST(test_callback_thread_writes_sse_data);
    RUN_TEST(test_callback_thread_skips_empty_json);
    RUN_TEST(test_callback_thread_write_fail);
    return UNITY_END();
}
