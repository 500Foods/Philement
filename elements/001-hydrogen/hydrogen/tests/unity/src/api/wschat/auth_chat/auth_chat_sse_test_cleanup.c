/*
 * Unity Test File: rest_sse_cleanup
 * This file contains unit tests for rest_sse_cleanup() in
 * src/api/wschat/auth_chat/auth_chat_sse.c
 *
 * CHANGELOG:
 * 2026-09-05: Initial coverage for SSE context teardown
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/proxy_multi.h>

#include <unistd.h>

void test_cleanup_null(void);
void test_cleanup_already_done(void);
void test_cleanup_pipes_only(void);
void test_cleanup_unlinks_stream(void);
void test_cleanup_middle_stream(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_cleanup_null(void) {
    rest_sse_cleanup(NULL);
}

void test_cleanup_already_done(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->cleanup_done = true;
    ctx->pipe_read = -1;
    ctx->pipe_write = -1;
    rest_sse_cleanup(ctx);
    free(ctx);
}

void test_cleanup_pipes_only(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    int pipefd[2];
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL_INT(0, pipe(pipefd));
    ctx->pipe_read = pipefd[0];
    ctx->pipe_write = pipefd[1];
    rest_sse_cleanup(ctx);
}

void test_cleanup_unlinks_stream(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    MultiStreamManager *mgr = calloc(1, sizeof(MultiStreamManager));
    MultiStreamContext *sc = calloc(1, sizeof(MultiStreamContext));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_NOT_NULL(sc);

    pthread_mutex_init(&mgr->streams_mutex, NULL);
    chunk_queue_init(&sc->chunk_queue);
    sc->request_id = strdup("req");
    sc->engine_name = strdup("eng");
    sc->finish_reason = strdup("stop");
    sc->request_body = strdup("{}");
    sc->headers = curl_slist_append(NULL, "Content-Type: application/json");
    mgr->active_streams = sc;
    ctx->manager = mgr;
    ctx->stream_ctx = sc;
    ctx->pipe_read = -1;
    ctx->pipe_write = -1;

    rest_sse_cleanup(ctx);
    TEST_ASSERT_NULL(mgr->active_streams);
    pthread_mutex_destroy(&mgr->streams_mutex);
    free(mgr);
}

void test_cleanup_middle_stream(void) {
    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    MultiStreamManager *mgr = calloc(1, sizeof(MultiStreamManager));
    MultiStreamContext *prev = calloc(1, sizeof(MultiStreamContext));
    MultiStreamContext *sc = calloc(1, sizeof(MultiStreamContext));
    MultiStreamContext *next = calloc(1, sizeof(MultiStreamContext));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_NOT_NULL(sc);
    TEST_ASSERT_NOT_NULL(next);

    pthread_mutex_init(&mgr->streams_mutex, NULL);
    chunk_queue_init(&sc->chunk_queue);
    sc->request_id = strdup("mid");
    sc->engine_name = strdup("eng");
    sc->prev = prev;
    sc->next = next;
    prev->next = sc;
    next->prev = sc;
    mgr->active_streams = prev;
    ctx->manager = mgr;
    ctx->stream_ctx = sc;
    ctx->pipe_read = -1;
    ctx->pipe_write = -1;

    rest_sse_cleanup(ctx);
    TEST_ASSERT_EQUAL_PTR(next, prev->next);
    TEST_ASSERT_EQUAL_PTR(prev, next->prev);
    pthread_mutex_destroy(&mgr->streams_mutex);
    free(prev);
    free(next);
    free(mgr);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cleanup_null);
    RUN_TEST(test_cleanup_already_done);
    RUN_TEST(test_cleanup_pipes_only);
    RUN_TEST(test_cleanup_unlinks_stream);
    RUN_TEST(test_cleanup_middle_stream);
    return UNITY_END();
}
