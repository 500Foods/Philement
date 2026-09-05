#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/engine_cache.h>

void test_stream_start_null_params(void);
void test_stream_start_uninitialized_manager(void);
void test_restart_easy_null_params(void);
void test_restart_easy_uninitialized_manager(void);
void test_stream_stop_null_params(void);
void test_request_writable_null(void);
void test_drain_queue_empty(void);
void test_drain_queue_null_connection_valid(void);

void setUp(void) {}
void tearDown(void) {}

void test_stream_start_null_params(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    TEST_ASSERT_NULL(chat_proxy_multi_stream_start(NULL, engine, "{}", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    TEST_ASSERT_NULL(chat_proxy_multi_stream_start(&mgr, NULL, "{}", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    TEST_ASSERT_NULL(chat_proxy_multi_stream_start(&mgr, engine, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));

    chat_engine_config_destroy(engine);
}

void test_stream_start_uninitialized_manager(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    TEST_ASSERT_NULL(chat_proxy_multi_stream_start(&mgr, engine, "{}", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));

    chat_engine_config_destroy(engine);
}

void test_restart_easy_null_params(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(NULL, NULL, "{}"));
}

void test_restart_easy_uninitialized_manager(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    ctx.engine = engine;

    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(&mgr, &ctx, "{}"));

    chat_engine_config_destroy(engine);
}

void test_stream_stop_null_params(void) {
    chat_proxy_multi_stream_stop(NULL, NULL);
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    chat_proxy_multi_stream_stop(&mgr, NULL);
}

void test_request_writable_null(void) {
    chat_proxy_multi_request_writable(NULL);
}

void test_drain_queue_empty(void) {
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    chunk_queue_init(&ctx.chunk_queue);
    TEST_ASSERT_EQUAL_INT(0, chat_proxy_multi_drain_queue(&ctx));
    chunk_queue_destroy(&ctx.chunk_queue);
}

void test_drain_queue_null_connection_valid(void) {
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    chunk_queue_init(&ctx.chunk_queue);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"data\":1}", 10);
    ctx.connection_valid = NULL;
    ctx.wsi = (struct lws *)0x1;

    int written = chat_proxy_multi_drain_queue(&ctx);
    TEST_ASSERT_EQUAL_INT(1, written);

    chunk_queue_destroy(&ctx.chunk_queue);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stream_start_null_params);
    RUN_TEST(test_stream_start_uninitialized_manager);
    RUN_TEST(test_restart_easy_null_params);
    RUN_TEST(test_restart_easy_uninitialized_manager);
    RUN_TEST(test_stream_stop_null_params);
    RUN_TEST(test_request_writable_null);
    RUN_TEST(test_drain_queue_empty);
    RUN_TEST(test_drain_queue_null_connection_valid);
    return UNITY_END();
}
