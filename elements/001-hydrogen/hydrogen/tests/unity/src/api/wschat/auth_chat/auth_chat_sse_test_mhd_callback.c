/*
 * Unity Test File: rest_sse_mhd_callback
 * This file contains unit tests for rest_sse_mhd_callback() in
 * src/api/wschat/auth_chat/auth_chat_sse.c
 *
 * CHANGELOG:
 * 2026-09-05: Initial coverage for MHD SSE reader callback
 *
 * TEST_VERSION: 1.0.0
 */

#define USE_MOCK_SYSTEM

#include <src/hydrogen.h>
#include <unity.h>
#include <tests/unity/mocks/mock_system.h>

#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/proxy_multi.h>

void test_mhd_callback_null_ctx(void);
void test_mhd_callback_bad_pipe(void);
void test_mhd_callback_reads_data(void);
void test_mhd_callback_end_of_stream(void);
void test_mhd_callback_eagain(void);
void test_mhd_callback_read_error(void);
void test_mhd_callback_done_then_data(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_mhd_callback_null_ctx(void) {
    char buf[16];
    TEST_ASSERT_EQUAL(MHD_CONTENT_READER_END_WITH_ERROR,
                      rest_sse_mhd_callback(NULL, 0, buf, sizeof(buf)));
}

void test_mhd_callback_bad_pipe(void) {
    RestSseContext ctx;
    char buf[16];
    memset(&ctx, 0, sizeof(ctx));
    ctx.pipe_read = -1;
    TEST_ASSERT_EQUAL(MHD_CONTENT_READER_END_WITH_ERROR,
                      rest_sse_mhd_callback(&ctx, 0, buf, sizeof(buf)));
}

void test_mhd_callback_reads_data(void) {
    RestSseContext ctx;
    char buf[16];
    ssize_t n;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pipe_read = 3;
    ctx.pipe_write = -1;
    mock_system_set_read_data("hello", 5);
    mock_system_set_read_result(5);

    n = rest_sse_mhd_callback(&ctx, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(5, (int)n);
}

void test_mhd_callback_end_of_stream(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    char buf[16];
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->pipe_read = 3;
    ctx->pipe_write = -1;
    ctx->callback_done = true;
    mock_system_set_read_result(0);

    TEST_ASSERT_EQUAL(MHD_CONTENT_READER_END_OF_STREAM,
                      rest_sse_mhd_callback(ctx, 0, buf, sizeof(buf)));
}

void test_mhd_callback_eagain(void) {
    RestSseContext ctx;
    char buf[16];

    memset(&ctx, 0, sizeof(ctx));
    ctx.pipe_read = 3;
    ctx.pipe_write = -1;
    mock_system_set_read_eagain(1);

    TEST_ASSERT_EQUAL_INT(0, (int)rest_sse_mhd_callback(&ctx, 0, buf, sizeof(buf)));
}

void test_mhd_callback_read_error(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    char buf[16];
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->pipe_read = 3;
    ctx->pipe_write = -1;
    mock_system_set_read_should_fail(1);

    TEST_ASSERT_EQUAL(MHD_CONTENT_READER_END_WITH_ERROR,
                      rest_sse_mhd_callback(ctx, 0, buf, sizeof(buf)));
}

void test_mhd_callback_done_then_data(void) {
    RestSseContext ctx;
    char buf[16];
    ssize_t n;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pipe_read = 3;
    ctx.pipe_write = -1;
    ctx.callback_done = true;
    mock_system_set_read_data("end", 3);
    mock_system_set_read_result(3);

    n = rest_sse_mhd_callback(&ctx, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(3, (int)n);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mhd_callback_null_ctx);
    RUN_TEST(test_mhd_callback_bad_pipe);
    RUN_TEST(test_mhd_callback_reads_data);
    RUN_TEST(test_mhd_callback_end_of_stream);
    RUN_TEST(test_mhd_callback_eagain);
    RUN_TEST(test_mhd_callback_read_error);
    RUN_TEST(test_mhd_callback_done_then_data);
    return UNITY_END();
}
