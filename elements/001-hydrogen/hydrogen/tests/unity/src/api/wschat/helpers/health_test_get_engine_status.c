/*
 * Unity Test File: chat_health_get_engine_status
 * Tests the thread-safe per-engine status derivation in
 * src/api/wschat/helpers/health.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

void test_chat_health_get_engine_status_null(void);
void test_chat_health_get_engine_status_healthy(void);
void test_chat_health_get_engine_status_unavailable(void);
void test_chat_health_get_engine_status_degraded(void);

void setUp(void) {}
void tearDown(void) {}

void test_chat_health_get_engine_status_null(void) {
    TEST_ASSERT_EQUAL_INT(HEALTH_UNKNOWN, chat_health_get_engine_status(NULL));
}

void test_chat_health_get_engine_status_healthy(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    e->is_healthy = true;
    e->consecutive_failures = 0;
    TEST_ASSERT_EQUAL_INT(HEALTH_HEALTHY, chat_health_get_engine_status(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_get_engine_status_unavailable(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    e->is_healthy = false;
    e->consecutive_failures = 0;
    TEST_ASSERT_EQUAL_INT(HEALTH_UNAVAILABLE, chat_health_get_engine_status(e));
    chat_engine_config_destroy(e);
}

void test_chat_health_get_engine_status_degraded(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    e->is_healthy = true;
    e->consecutive_failures = 1;
    TEST_ASSERT_EQUAL_INT(HEALTH_DEGRADED, chat_health_get_engine_status(e));
    chat_engine_config_destroy(e);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_get_engine_status_null);
    RUN_TEST(test_chat_health_get_engine_status_healthy);
    RUN_TEST(test_chat_health_get_engine_status_unavailable);
    RUN_TEST(test_chat_health_get_engine_status_degraded);
    return UNITY_END();
}
