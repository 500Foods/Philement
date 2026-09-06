/*
 * Unity Test File: rest_sse_free_cls
 * This file contains unit tests for rest_sse_free_cls() in
 * src/api/wschat/auth_chat/auth_chat_sse.c
 *
 * CHANGELOG:
 * 2026-09-05: Initial coverage for MHD free callback
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/auth_chat/auth_chat.h>

void test_free_cls_null(void);
void test_free_cls_heap_ctx(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_free_cls_null(void) {
    rest_sse_free_cls(NULL);
}

void test_free_cls_heap_ctx(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->pipe_read = -1;
    ctx->pipe_write = -1;
    rest_sse_free_cls(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_free_cls_null);
    RUN_TEST(test_free_cls_heap_ctx);
    return UNITY_END();
}
