/*
 * Unity Test File: chat_health_monitor_start
 * Tests the monitor start logic in src/api/wschat/helpers/health.c, including
 * the already-running, no-liveliness, and thread-create-failure branches.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_PTHREAD
#include <unity/mocks/mock_pthread.h>

void test_chat_health_monitor_start_null(void);
void test_chat_health_monitor_start_already_running(void);
void test_chat_health_monitor_start_no_liveliness(void);
void test_chat_health_monitor_start_create_failure(void);

void setUp(void) {
    mock_pthread_reset_all();
}

void tearDown(void) {
    mock_pthread_reset_all();
}

void test_chat_health_monitor_start_null(void) {
    TEST_ASSERT_FALSE(chat_health_monitor_start(NULL));
}

void test_chat_health_monitor_start_already_running(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    c->health_monitor_running = true;
    c->health_monitor_thread = 1;  // arbitrary non-zero handle
    TEST_ASSERT_TRUE(chat_health_monitor_start(c));
    TEST_ASSERT_TRUE(c->health_monitor_running);
    chat_engine_cache_destroy(c);
}

void test_chat_health_monitor_start_no_liveliness(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    // No engines with liveliness enabled -> start is a no-op success.
    TEST_ASSERT_TRUE(chat_health_monitor_start(c));
    TEST_ASSERT_FALSE(c->health_monitor_running);
    chat_engine_cache_destroy(c);
}

void test_chat_health_monitor_start_create_failure(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    ChatEngineConfig* e = chat_engine_config_create(1, "gpt", CEC_PROVIDER_OPENAI,
        "gpt-4", "u", "k", 0, 0, false, 30, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(c, e));

    // Force pthread_create to fail.
    mock_pthread_set_create_failure(1);
    TEST_ASSERT_FALSE(chat_health_monitor_start(c));
    TEST_ASSERT_FALSE(c->health_monitor_running);

    chat_engine_cache_destroy(c);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_monitor_start_null);
    RUN_TEST(test_chat_health_monitor_start_already_running);
    RUN_TEST(test_chat_health_monitor_start_no_liveliness);
    RUN_TEST(test_chat_health_monitor_start_create_failure);
    return UNITY_END();
}
