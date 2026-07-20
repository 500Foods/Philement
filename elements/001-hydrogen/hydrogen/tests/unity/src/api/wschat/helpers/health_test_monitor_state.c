/*
 * Unity Test File: chat_health_monitor_is_running / chat_health_monitor_stop
 * Tests the monitor running-state accessors in src/api/wschat/helpers/health.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

#include <src/api/wschat/helpers/health.h>
#include <src/api/wschat/helpers/engine_cache.h>

void test_chat_health_monitor_is_running_null(void);
void test_chat_health_monitor_is_running_false(void);
void test_chat_health_monitor_is_running_true(void);
void test_chat_health_monitor_stop_null(void);
void test_chat_health_monitor_stop_already_stopped(void);
void test_chat_health_monitor_stop_running(void);

void setUp(void) {}
void tearDown(void) {}

void test_chat_health_monitor_is_running_null(void) {
    TEST_ASSERT_FALSE(chat_health_monitor_is_running(NULL));
}

void test_chat_health_monitor_is_running_false(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    TEST_ASSERT_FALSE(chat_health_monitor_is_running(c));
    chat_engine_cache_destroy(c);
}

void test_chat_health_monitor_is_running_true(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    c->health_monitor_running = true;
    TEST_ASSERT_TRUE(chat_health_monitor_is_running(c));
    chat_engine_cache_destroy(c);
}

void test_chat_health_monitor_stop_null(void) {
    chat_health_monitor_stop(NULL);
}

void test_chat_health_monitor_stop_already_stopped(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    c->health_monitor_running = false;
    c->health_monitor_thread = 0;
    chat_health_monitor_stop(c);
    TEST_ASSERT_FALSE(c->health_monitor_running);
    chat_engine_cache_destroy(c);
}

void test_chat_health_monitor_stop_running(void) {
    ChatEngineCache* c = chat_engine_cache_create("db");
    c->health_monitor_running = true;
    c->health_monitor_thread = 0;
    chat_health_monitor_stop(c);
    TEST_ASSERT_FALSE(c->health_monitor_running);
    chat_engine_cache_destroy(c);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_monitor_is_running_null);
    RUN_TEST(test_chat_health_monitor_is_running_false);
    RUN_TEST(test_chat_health_monitor_is_running_true);
    RUN_TEST(test_chat_health_monitor_stop_null);
    RUN_TEST(test_chat_health_monitor_stop_already_stopped);
    RUN_TEST(test_chat_health_monitor_stop_running);
    return UNITY_END();
}
