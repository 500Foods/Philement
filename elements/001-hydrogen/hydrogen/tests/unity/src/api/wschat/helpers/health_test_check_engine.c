/*
 * Unity Test File: chat_health_check_engine
 * Tests the per-engine health check dispatcher in
 * src/api/wschat/helpers/health.c, exercising every provider branch and the
 * unknown-provider fallback.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_CHAT_PROXY
#include <unity/mocks/mock_chat_proxy.h>

void test_chat_health_check_engine_null(void);
void test_chat_health_check_engine_openai(void);
void test_chat_health_check_engine_anthropic(void);
void test_chat_health_check_engine_ollama(void);
void test_chat_health_check_engine_unknown(void);
void test_chat_health_check_engine_updates_health(void);

void setUp(void) {
    mock_chat_proxy_reset_all();
}

void tearDown(void) {
    mock_chat_proxy_reset_all();
}

void test_chat_health_check_engine_null(void) {
    TEST_ASSERT_FALSE(chat_health_check_engine(NULL));
}

void test_chat_health_check_engine_openai(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "https://api.openai.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_engine(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_check_engine_anthropic(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "https://api.anthropic.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_engine(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_check_engine_ollama(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "ollama", CEC_PROVIDER_OLLAMA,
        "llama3", "http://localhost:11434", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_engine(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_check_engine_unknown(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "mystery", CEC_PROVIDER_UNKNOWN,
        "model", "https://example.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    // Unknown provider never calls the proxy and always reports unhealthy.
    TEST_ASSERT_FALSE(chat_health_check_engine(e));
    TEST_ASSERT_EQUAL_INT(0, mock_chat_proxy_call_count());
    chat_engine_config_destroy(e);
}

void test_chat_health_check_engine_updates_health(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "https://api.openai.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    // Successful check should mark the engine healthy.
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_engine(e));
    TEST_ASSERT_TRUE(e->is_healthy);
    TEST_ASSERT_EQUAL_INT(0, e->consecutive_failures);

    // Three failing checks are required before mark_health_checked flips the
    // engine to unhealthy (mirrors chat_engine_config_mark_health_checked).
    mock_chat_proxy_set_http_status(500);
    TEST_ASSERT_FALSE(chat_health_check_engine(e));
    TEST_ASSERT_FALSE(chat_health_check_engine(e));
    TEST_ASSERT_FALSE(chat_health_check_engine(e));
    TEST_ASSERT_EQUAL_INT(3, e->consecutive_failures);
    TEST_ASSERT_FALSE(e->is_healthy);

    // A successful check recovers the engine.
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_engine(e));
    TEST_ASSERT_TRUE(e->is_healthy);

    chat_engine_config_destroy(e);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_check_engine_null);
    RUN_TEST(test_chat_health_check_engine_openai);
    RUN_TEST(test_chat_health_check_engine_anthropic);
    RUN_TEST(test_chat_health_check_engine_ollama);
    RUN_TEST(test_chat_health_check_engine_unknown);
    RUN_TEST(test_chat_health_check_engine_updates_health);
    return UNITY_END();
}
