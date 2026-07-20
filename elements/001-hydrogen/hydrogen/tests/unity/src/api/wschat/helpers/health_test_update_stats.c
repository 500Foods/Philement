/*
 * Unity Test File: chat_health_update_stats
 * Tests the rolling stats / failure-counting logic in
 * src/api/wschat/helpers/health.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

void test_chat_health_update_stats_null(void);
void test_chat_health_update_stats_success_first(void);
void test_chat_health_update_stats_success_ema(void);
void test_chat_health_update_stats_failure_threshold(void);
void test_chat_health_update_stats_reset_on_success(void);

void setUp(void) {}
void tearDown(void) {}

void test_chat_health_update_stats_null(void) {
    chat_health_update_stats(NULL, true, 100.0, 10);
}

void test_chat_health_update_stats_success_first(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    chat_health_update_stats(e, true, 100.0, 10);

    TEST_ASSERT_TRUE(e->is_healthy);
    TEST_ASSERT_EQUAL_INT(0, e->consecutive_failures);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, e->avg_response_time_ms);
    TEST_ASSERT_EQUAL_UINT64(10, e->tokens_24h);
    TEST_ASSERT_EQUAL_UINT64(1, e->conversations_24h);

    chat_engine_config_destroy(e);
}

void test_chat_health_update_stats_success_ema(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    chat_health_update_stats(e, true, 100.0, 10);  // seed average = 100
    chat_health_update_stats(e, true, 200.0, 5);   // EMA = 100*0.9 + 200*0.1 = 110

    TEST_ASSERT_EQUAL_DOUBLE(110.0, e->avg_response_time_ms);
    TEST_ASSERT_EQUAL_UINT64(15, e->tokens_24h);
    TEST_ASSERT_EQUAL_UINT64(2, e->conversations_24h);

    chat_engine_config_destroy(e);
}

void test_chat_health_update_stats_failure_threshold(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    // Two failures keep it healthy (below HEALTH_MAX_FAILURES = 3)
    chat_health_update_stats(e, false, 0.0, 0);
    chat_health_update_stats(e, false, 0.0, 0);
    TEST_ASSERT_EQUAL_INT(2, e->consecutive_failures);
    TEST_ASSERT_TRUE(e->is_healthy);

    // Third failure trips the threshold -> unhealthy
    chat_health_update_stats(e, false, 0.0, 0);
    TEST_ASSERT_EQUAL_INT(3, e->consecutive_failures);
    TEST_ASSERT_FALSE(e->is_healthy);

    chat_engine_config_destroy(e);
}

void test_chat_health_update_stats_reset_on_success(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    chat_health_update_stats(e, false, 0.0, 0);
    chat_health_update_stats(e, false, 0.0, 0);
    TEST_ASSERT_EQUAL_INT(2, e->consecutive_failures);

    chat_health_update_stats(e, true, 50.0, 1);
    TEST_ASSERT_EQUAL_INT(0, e->consecutive_failures);
    TEST_ASSERT_TRUE(e->is_healthy);
    TEST_ASSERT_EQUAL_DOUBLE(50.0, e->avg_response_time_ms);

    chat_engine_config_destroy(e);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_update_stats_null);
    RUN_TEST(test_chat_health_update_stats_success_first);
    RUN_TEST(test_chat_health_update_stats_success_ema);
    RUN_TEST(test_chat_health_update_stats_failure_threshold);
    RUN_TEST(test_chat_health_update_stats_reset_on_success);
    return UNITY_END();
}
