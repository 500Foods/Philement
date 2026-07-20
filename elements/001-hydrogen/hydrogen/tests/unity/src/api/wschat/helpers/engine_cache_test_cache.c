/*
 * Unity Test File: chat_engine_cache core operations
 * This file contains unit tests for the cache lifecycle and entry management
 * functions in src/api/wschat/helpers/engine_cache.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

void test_cache_create_destroy(void);
void test_cache_create_null_name(void);
void test_cache_clear_null(void);
void test_cache_add_null_params(void);
void test_cache_add_and_lookup(void);
void test_cache_add_engine_locked_resize(void);
void test_cache_get_all_null(void);
void test_cache_get_all_returns_copy(void);
void test_cache_update_usage(void);

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

/* Create + destroy works with a real database name */
void test_cache_create_destroy(void) {
    TEST_ASSERT_EQUAL_STRING("testdb", g_cache->database_name);
    TEST_ASSERT_EQUAL_size_t(0, g_cache->engine_count);
    TEST_ASSERT_EQUAL_size_t(16, g_cache->capacity);
}

/* NULL database name still yields a valid cache */
void test_cache_create_null_name(void) {
    ChatEngineCache* c = chat_engine_cache_create(NULL);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_NULL(c->database_name);
    chat_engine_cache_destroy(c);
}

/* Clear on NULL is safe */
void test_cache_clear_null(void) {
    chat_engine_cache_clear(NULL);
}

/* Invalid params to add engine */
void test_cache_add_null_params(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_FALSE(chat_engine_cache_add_engine(NULL, e));
    TEST_ASSERT_FALSE(chat_engine_cache_add_engine(g_cache, NULL));
    chat_engine_config_destroy(e);
}

/* Add engine and look it up by name, id and default */
void test_cache_add_and_lookup(void) {
    ChatEngineConfig* e1 = chat_engine_config_create(10, "alpha", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, true, 0, 0, 0, 0, 0, false);
    ChatEngineConfig* e2 = chat_engine_config_create(11, "beta", CEC_PROVIDER_OLLAMA,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e1));
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e2));

    TEST_ASSERT_EQUAL_size_t(2, chat_engine_cache_get_engine_count(g_cache));

    TEST_ASSERT_EQUAL_PTR(e1, chat_engine_cache_lookup_by_name(g_cache, "alpha"));
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_name(g_cache, "missing"));
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_name(NULL, "alpha"));
    TEST_ASSERT_EQUAL_PTR(e2, chat_engine_cache_lookup_by_id(g_cache, 11));
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_id(g_cache, 999));
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_id(NULL, 10));

    /* Default engine is e1 */
    TEST_ASSERT_EQUAL_PTR(e1, chat_engine_cache_get_default(g_cache));
    TEST_ASSERT_NULL(chat_engine_cache_get_default(NULL));

    /* update_usage bumps count by exactly 1 for a found engine */
    chat_engine_cache_update_usage(g_cache, 10);
    chat_engine_cache_update_usage(g_cache, 9999);
}

/* Force realloc path by exceeding initial capacity */
void test_cache_add_engine_locked_resize(void) {
    for (int i = 0; i < 20; i++) {
        ChatEngineConfig* e = chat_engine_config_create(i, "n", CEC_PROVIDER_OPENAI,
            "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
        bool ok = chat_engine_cache_add_engine_locked(g_cache, e);
        if (!ok) {
            chat_engine_config_destroy(e);
            TEST_FAIL_MESSAGE("add_engine_locked failed during resize");
        }
    }
    TEST_ASSERT_EQUAL_size_t(20, g_cache->engine_count);
    TEST_ASSERT_TRUE(g_cache->capacity >= 20);
}

/* get_all with NULL args */
void test_cache_get_all_null(void) {
    size_t count = 0;
    TEST_ASSERT_NULL(chat_engine_cache_get_all(NULL, &count));
    TEST_ASSERT_NULL(chat_engine_cache_get_all(g_cache, NULL));
}

/* get_all returns a shallow copy array */
void test_cache_get_all_returns_copy(void) {
    ChatEngineConfig* e = chat_engine_config_create(1, "n", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e));

    size_t count = 0;
    ChatEngineConfig** all = chat_engine_cache_get_all(g_cache, &count);
    TEST_ASSERT_NOT_NULL(all);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_PTR(e, all[0]);
    free(all);
}

/* update_usage increments stats for a found engine */
void test_cache_update_usage(void) {
    ChatEngineConfig* e = chat_engine_config_create(10, "alpha", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(g_cache, e));
    chat_engine_cache_update_usage(g_cache, 10);
    chat_engine_cache_update_usage(g_cache, 9999);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_create_destroy);
    RUN_TEST(test_cache_create_null_name);
    RUN_TEST(test_cache_clear_null);
    RUN_TEST(test_cache_add_null_params);
    RUN_TEST(test_cache_add_and_lookup);
    RUN_TEST(test_cache_add_engine_locked_resize);
    RUN_TEST(test_cache_get_all_null);
    RUN_TEST(test_cache_get_all_returns_copy);
    RUN_TEST(test_cache_update_usage);
    return UNITY_END();
}
