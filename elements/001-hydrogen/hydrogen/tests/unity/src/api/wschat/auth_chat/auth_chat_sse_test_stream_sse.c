/*
 * Unity Test File: auth_chat_stream_sse
 * This file contains unit tests for auth_chat_stream_sse() in
 * src/api/wschat/auth_chat/auth_chat_sse.c
 *
 * CHANGELOG:
 * 2026-09-05: Initial coverage for REST SSE start/error/success paths
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/engine_cache.h>

#include <unistd.h>

void test_stream_sse_null_args(void);
void test_stream_sse_manager_unavailable(void);
void test_stream_sse_calloc_failure(void);
void test_stream_sse_start_failure(void);
void test_stream_sse_response_failure(void);
void test_stream_sse_success(void);

static ChatEngineConfig *make_engine(void) {
    return chat_engine_config_create(
        1, "gpt-4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "http://127.0.0.1:1/v1/chat/completions", "key", 1000, 0.7, false,
        0, 4, 8, 8, MODALITY_TEXT, false);
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    MultiStreamManager *mgr = chat_proxy_get_multi_manager();
    mgr->initialized = false;
    mgr->multi_handle = NULL;
    mgr->active_streams = NULL;
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void test_stream_sse_null_args(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(MHD_NO, auth_chat_stream_sse(NULL, engine, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL(MHD_NO, auth_chat_stream_sse(conn, NULL, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL(MHD_NO, auth_chat_stream_sse(conn, engine, NULL, NULL, NULL));
    chat_engine_config_destroy(engine);
}

void test_stream_sse_manager_unavailable(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(MHD_YES, auth_chat_stream_sse(conn, engine, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE, mock_mhd_get_last_status_code());
    chat_engine_config_destroy(engine);
}

void test_stream_sse_calloc_failure(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    MultiStreamManager *mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(engine);
    mgr->initialized = true;
    mock_system_set_calloc_failure(1);
    TEST_ASSERT_EQUAL(MHD_YES, auth_chat_stream_sse(conn, engine, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_mhd_get_last_status_code());
    mgr->initialized = false;
    chat_engine_config_destroy(engine);
}

void test_stream_sse_start_failure(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    MultiStreamManager *mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(engine);
    mgr->initialized = true;
    mgr->multi_handle = NULL;
    TEST_ASSERT_EQUAL(MHD_YES, auth_chat_stream_sse(conn, engine, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_mhd_get_last_status_code());
    mgr->initialized = false;
    chat_engine_config_destroy(engine);
}

void test_stream_sse_response_failure(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    MultiStreamManager *mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(engine);
    pthread_mutex_init(&mgr->streams_mutex, NULL);
    mgr->multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(mgr->multi_handle);
    mgr->initialized = true;
    mgr->active_streams = NULL;
    mock_mhd_set_create_callback_response_should_fail(true);

    TEST_ASSERT_EQUAL(MHD_YES, auth_chat_stream_sse(conn, engine, "{}", NULL, NULL));
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_mhd_get_last_status_code());

    curl_multi_cleanup(mgr->multi_handle);
    pthread_mutex_destroy(&mgr->streams_mutex);
    mgr->multi_handle = NULL;
    mgr->initialized = false;
    chat_engine_config_destroy(engine);
}

void test_stream_sse_success(void) {
    ChatEngineConfig *engine = make_engine();
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    MultiStreamManager *mgr = chat_proxy_get_multi_manager();
    RestSseContext *ctx;
    TEST_ASSERT_NOT_NULL(engine);

    pthread_mutex_init(&mgr->streams_mutex, NULL);
    mgr->multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(mgr->multi_handle);
    mgr->initialized = true;
    mgr->active_streams = NULL;
    mgr->shutdown_requested = false;
    mgr->worker_thread_started = false;

    TEST_ASSERT_EQUAL(MHD_YES, auth_chat_stream_sse(conn, engine, "{\"stream\":true}", NULL, NULL));
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_TRUE(mock_mhd_header_was_added("Content-Type", "text/event-stream"));

    ctx = (RestSseContext *)mock_mhd_get_callback_cls();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NOT_NULL(ctx->stream_ctx);
    ctx->stream_ctx->stream_completed = true;
    while (!ctx->callback_done) {
        usleep(1000);
    }
    pthread_join(ctx->callback_thread, NULL);
    ctx->callback_thread = 0;
    chat_proxy_multi_stream_stop(mgr, ctx->stream_ctx);
    rest_sse_cleanup(ctx);

    curl_multi_cleanup(mgr->multi_handle);
    pthread_mutex_destroy(&mgr->streams_mutex);
    mgr->multi_handle = NULL;
    mgr->initialized = false;
    mgr->active_streams = NULL;
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stream_sse_null_args);
    RUN_TEST(test_stream_sse_manager_unavailable);
    RUN_TEST(test_stream_sse_calloc_failure);
    RUN_TEST(test_stream_sse_start_failure);
    RUN_TEST(test_stream_sse_response_failure);
    RUN_TEST(test_stream_sse_success);
    return UNITY_END();
}
