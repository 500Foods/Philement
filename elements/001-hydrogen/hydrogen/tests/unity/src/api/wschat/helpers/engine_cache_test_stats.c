/*
 * Unity Test File: chat_engine_cache stats, refresh, providers and health
 * This file contains unit tests for the statistics/refresh/provider/health
 * functions in src/api/wschat/helpers/engine_cache.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

void test_stats_null_args(void);
void test_stats_populated(void);
void test_needs_refresh(void);
void test_should_refresh(void);
void test_get_last_refresh(void);
void test_provider_from_string(void);
void test_provider_to_string(void);
void test_config_get_status(void);
void test_config_update_health(void);
void test_config_mark_health_checked(void);
void test_clear_removes_entries(void);
void test_refresh_null(void);

static ChatEngineCache* g_cache = NULL;

void setUp(void) {
    mock_system_reset_all();
    g_cache = chat_engine_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(g_cache);
}

void tearDown(void) {
    if (g_cache) {
        chat_engine_cache_destroy(g_cache);
        g_cache = NULL;
    }
    mock_system_reset_all();
}

/* NULL args to stats are safe no-ops */
void test_stats_null_args(void) {
    chat_engine_cache_get_stats(NULL, (char*)"x", 10);
    chat_engine_cache_get_stats(g_cache, NULL, 10);
    chat_engine_cache_get_stats(g_cache, (char*)"x", 0);
}

/* Populated cache produces a stats string */
void test_stats_populated(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "alpha", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, true, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e));

    char buffer[256];
    chat_engine_cache_get_stats(g_cache, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(strstr(buffer, "Chat engines: 1") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "default: 1") != NULL);
}

/* needs_refresh honors interval and last_refresh */
void test_needs_refresh(void) {
    TEST_ASSERT_FALSE(chat_engine_cache_needs_refresh(NULL, 3600));
    g_cache->last_refresh = time(NULL);
    TEST_ASSERT_FALSE(chat_engine_cache_needs_refresh(g_cache, 3600));
    g_cache->last_refresh = time(NULL) - 7200;
    TEST_ASSERT_TRUE(chat_engine_cache_needs_refresh(g_cache, 3600));
    /* Negative interval falls back to the default 3600 */
    g_cache->last_refresh = time(NULL) - 7200;
    TEST_ASSERT_TRUE(chat_engine_cache_needs_refresh(g_cache, -1));
}

/* should_refresh uses >= and default 3600 */
void test_should_refresh(void) {
    TEST_ASSERT_FALSE(chat_engine_cache_should_refresh(NULL, 3600));
    g_cache->last_refresh = time(NULL) - 3600;
    TEST_ASSERT_TRUE(chat_engine_cache_should_refresh(g_cache, 3600));
    g_cache->last_refresh = time(NULL);
    TEST_ASSERT_FALSE(chat_engine_cache_should_refresh(g_cache, -5));
}

/* get_last_refresh returns stored value */
void test_get_last_refresh(void) {
    TEST_ASSERT_EQUAL_INT(0, chat_engine_cache_get_last_refresh(NULL));
    g_cache->last_refresh = (time_t)12345;
    TEST_ASSERT_EQUAL_INT(12345, chat_engine_cache_get_last_refresh(g_cache));
}

/* provider string parsing */
void test_provider_from_string(void) {
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string(NULL));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("openai"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_ANTHROPIC, chat_engine_provider_from_string("anthropic"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OLLAMA, chat_engine_provider_from_string("ollama"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("xai"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("gradient"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string("bogus"));
}

/* provider enum to string */
void test_provider_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("openai", chat_engine_provider_to_string(CEC_PROVIDER_OPENAI));
    TEST_ASSERT_EQUAL_STRING("anthropic", chat_engine_provider_to_string(CEC_PROVIDER_ANTHROPIC));
    TEST_ASSERT_EQUAL_STRING("ollama", chat_engine_provider_to_string(CEC_PROVIDER_OLLAMA));
    TEST_ASSERT_EQUAL_STRING("unknown", chat_engine_provider_to_string(CEC_PROVIDER_UNKNOWN));
}

/* status strings */
void test_config_get_status(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_EQUAL_STRING("unknown", chat_engine_config_get_status(NULL));
    TEST_ASSERT_EQUAL_STRING("healthy", chat_engine_config_get_status(e));

    e->is_healthy = false;
    TEST_ASSERT_EQUAL_STRING("unavailable", chat_engine_config_get_status(e));

    e->is_healthy = true;
    e->consecutive_failures = 1;
    TEST_ASSERT_EQUAL_STRING("degraded", chat_engine_config_get_status(e));

    chat_engine_config_destroy(e);
}

/* update_health success path with rolling average */
void test_config_update_health(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    chat_engine_config_update_health(e, true, 100.0);
    TEST_ASSERT_EQUAL_INT(true, e->is_healthy);
    TEST_ASSERT_EQUAL_INT(0, e->consecutive_failures);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, e->avg_response_time_ms);

    chat_engine_config_update_health(e, true, 200.0);
    TEST_ASSERT_EQUAL_DOUBLE(110.0, e->avg_response_time_ms);

    chat_engine_config_update_health(e, false, 0.0);
    TEST_ASSERT_EQUAL_INT(1, e->consecutive_failures);
    TEST_ASSERT_EQUAL_INT(true, e->is_healthy);

    chat_engine_config_update_health(e, false, 0.0);
    chat_engine_config_update_health(e, false, 0.0);
    TEST_ASSERT_EQUAL_INT(3, e->consecutive_failures);
    TEST_ASSERT_EQUAL_INT(false, e->is_healthy);

    chat_engine_config_update_health(NULL, true, 0.0);
    chat_engine_config_destroy(e);
}

/* mark_health_checked toggles healthy/failures */
void test_config_mark_health_checked(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);

    chat_engine_config_mark_health_checked(e, true);
    TEST_ASSERT_EQUAL_INT(true, e->is_healthy);
    TEST_ASSERT_EQUAL_INT(0, e->consecutive_failures);

    chat_engine_config_mark_health_checked(e, false);
    chat_engine_config_mark_health_checked(e, false);
    chat_engine_config_mark_health_checked(e, false);
    TEST_ASSERT_EQUAL_INT(false, e->is_healthy);

    chat_engine_config_mark_health_checked(e, true);
    TEST_ASSERT_EQUAL_INT(true, e->is_healthy);

    chat_engine_config_mark_health_checked(NULL, true);
    chat_engine_config_destroy(e);
}

/* clear removes all entries and resets count */
void test_clear_removes_entries(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e));
    TEST_ASSERT_EQUAL_size_t(1, g_cache->engine_count);
    chat_engine_cache_clear(g_cache);
    TEST_ASSERT_EQUAL_size_t(0, g_cache->engine_count);
    TEST_ASSERT_EQUAL_PTR(NULL, chat_engine_cache_lookup_by_name(g_cache, "n"));
}

/* refresh with NULL or no database name fails */
void test_refresh_null(void) {
    TEST_ASSERT_FALSE(chat_engine_cache_refresh(NULL));
    ChatEngineCache* c = chat_engine_cache_create(NULL);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_FALSE(chat_engine_cache_refresh(c));
    chat_engine_cache_destroy(c);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stats_null_args);
    RUN_TEST(test_stats_populated);
    RUN_TEST(test_needs_refresh);
    RUN_TEST(test_should_refresh);
    RUN_TEST(test_get_last_refresh);
    RUN_TEST(test_provider_from_string);
    RUN_TEST(test_provider_to_string);
    RUN_TEST(test_config_get_status);
    RUN_TEST(test_config_update_health);
    RUN_TEST(test_config_mark_health_checked);
    RUN_TEST(test_clear_removes_entries);
    RUN_TEST(test_refresh_null);
    return UNITY_END();
}
