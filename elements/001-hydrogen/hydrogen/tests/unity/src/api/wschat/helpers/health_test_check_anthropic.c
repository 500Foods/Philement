/*
 * Unity Test File: chat_health_check_anthropic
 * Tests the Anthropic-specific health check path in
 * src/api/wschat/helpers/health.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_CHAT_PROXY
#include <unity/mocks/mock_chat_proxy.h>

void test_chat_health_check_anthropic_null(void);
void test_chat_health_check_anthropic_empty_url(void);
void test_chat_health_check_anthropic_null_request(void);
void test_chat_health_check_anthropic_200(void);
void test_chat_health_check_anthropic_400(void);
void test_chat_health_check_anthropic_500(void);

void setUp(void) {
    mock_chat_proxy_reset_all();
}

void tearDown(void) {
    mock_chat_proxy_reset_all();
}

void test_chat_health_check_anthropic_null(void) {
    TEST_ASSERT_FALSE(chat_health_check_anthropic(NULL));
}

void test_chat_health_check_anthropic_empty_url(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_FALSE(chat_health_check_anthropic(e));
    TEST_ASSERT_EQUAL_INT(0, mock_chat_proxy_call_count());
    chat_engine_config_destroy(e);
}

void test_chat_health_check_anthropic_null_request(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "https://api.anthropic.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_result_null(true);
    TEST_ASSERT_FALSE(chat_health_check_anthropic(e));
    TEST_ASSERT_EQUAL_INT(1, mock_chat_proxy_call_count());
    chat_engine_config_destroy(e);
}

void test_chat_health_check_anthropic_200(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "https://api.anthropic.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(200);
    TEST_ASSERT_TRUE(chat_health_check_anthropic(e));
    TEST_ASSERT_EQUAL_INT(1, mock_chat_proxy_call_count());
    TEST_ASSERT_EQUAL_STRING("claude-3", mock_chat_proxy_last_engine()->model);
    chat_engine_config_destroy(e);
}

void test_chat_health_check_anthropic_400(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "https://api.anthropic.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(400);
    TEST_ASSERT_TRUE(chat_health_check_anthropic(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_check_anthropic_500(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "claude", CEC_PROVIDER_ANTHROPIC,
        "claude-3", "https://api.anthropic.com", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    mock_chat_proxy_set_http_status(500);
    TEST_ASSERT_FALSE(chat_health_check_anthropic(e));
    chat_engine_config_destroy(e);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_check_anthropic_null);
    RUN_TEST(test_chat_health_check_anthropic_empty_url);
    RUN_TEST(test_chat_health_check_anthropic_null_request);
    RUN_TEST(test_chat_health_check_anthropic_200);
    RUN_TEST(test_chat_health_check_anthropic_400);
    RUN_TEST(test_chat_health_check_anthropic_500);
    return UNITY_END();
}
