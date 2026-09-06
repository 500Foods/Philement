/*
 * Unity Test File: chat_proxy_multi_stream_start / restart_easy
 *
 * CHANGELOG:
 * 2026-09-05: Cover stream_start success, Anthropic headers, restart_easy
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/engine_cache.h>

void test_stream_start_and_stop(void);
void test_stream_start_anthropic_and_restart(void);
void test_restart_easy_guards(void);

void setUp(void) {
}

void tearDown(void) {
}

static void fill_engine(ChatEngineConfig *engine, ChatEngineProvider provider) {
    memset(engine, 0, sizeof(*engine));
    snprintf(engine->name, sizeof(engine->name), "stream-eng");
    snprintf(engine->api_url, sizeof(engine->api_url), "http://127.0.0.1:1/");
    snprintf(engine->api_key, sizeof(engine->api_key), "k");
    engine->provider = provider;
}

void test_stream_start_and_stop(void) {
    MultiStreamManager mgr;
    ChatEngineConfig engine;
    volatile bool conn_valid = true;
    volatile bool stream_active = false;
    MultiStreamContext *stream;

    memset(&mgr, 0, sizeof(mgr));
    pthread_mutex_init(&mgr.streams_mutex, NULL);
    mgr.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(mgr.multi_handle);
    mgr.initialized = true;
    fill_engine(&engine, CEC_PROVIDER_OPENAI);

    stream = chat_proxy_multi_stream_start(
        &mgr, &engine, "{\"stream\":true}", NULL, NULL,
        &conn_valid, &stream_active, NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(stream);
    TEST_ASSERT_TRUE(stream_active);
    TEST_ASSERT_TRUE(stream->is_rest);

    chat_proxy_multi_stream_stop(&mgr, stream);
    TEST_ASSERT_FALSE(stream_active);
    chat_proxy_multi_cleanup(&mgr);
}

void test_stream_start_anthropic_and_restart(void) {
    MultiStreamManager mgr;
    ChatEngineConfig engine;
    volatile bool conn_valid = true;
    volatile bool stream_active = false;
    MultiStreamContext *stream;

    memset(&mgr, 0, sizeof(mgr));
    pthread_mutex_init(&mgr.streams_mutex, NULL);
    mgr.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(mgr.multi_handle);
    mgr.initialized = true;
    fill_engine(&engine, CEC_PROVIDER_ANTHROPIC);

    stream = chat_proxy_multi_stream_start(
        &mgr, &engine, "{\"stream\":true}", NULL, NULL,
        &conn_valid, &stream_active, NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(stream);
    chat_proxy_multi_stream_stop(&mgr, stream);
    TEST_ASSERT_TRUE(chat_proxy_multi_restart_easy(&mgr, stream, "{\"stream\":true,\"n\":2}"));
    chat_proxy_multi_stream_stop(&mgr, stream);
    chat_proxy_multi_cleanup(&mgr);
}

void test_restart_easy_guards(void) {
    MultiStreamManager mgr;
    MultiStreamContext ctx;
    memset(&mgr, 0, sizeof(mgr));
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(NULL, &ctx, "{}"));
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(&mgr, NULL, "{}"));
    mgr.initialized = true;
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(&mgr, &ctx, NULL));
    TEST_ASSERT_FALSE(chat_proxy_multi_restart_easy(&mgr, &ctx, "{}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stream_start_and_stop);
    RUN_TEST(test_stream_start_anthropic_and_restart);
    RUN_TEST(test_restart_easy_guards);
    return UNITY_END();
}
