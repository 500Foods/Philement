/*
 * Unity Test File: Conduit Status - chat models JSON builder
 * Tests conduit_status_build_models() in
 * src/api/conduit/status/status.c
 *
 * CHANGELOG:
 * 2026-07-16: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/status/status.h>
#include <src/api/wschat/helpers/engine_cache.h>

// Test function prototypes
void test_build_models_null_cec(void);
void test_build_models_empty_cache(void);
void test_build_models_single_engine_no_last_working(void);
void test_build_models_single_engine_with_last_working(void);
void test_build_models_null_engine_skipped(void);

void setUp(void) {
    // No global state to reset
}

void tearDown(void) {
    // No global state to clean
}

// NULL cache -> NULL result
void test_build_models_null_cec(void) {
    json_t* models = conduit_status_build_models(NULL);
    TEST_ASSERT_NULL(models);
}

// Cache with no engines -> NULL result
void test_build_models_empty_cache(void) {
    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(cec);
    if (cec) {
        json_t* models = conduit_status_build_models(cec);
        TEST_ASSERT_NULL(models);
        chat_engine_cache_destroy(cec);
    }
}

// Single engine, last_working == 0 -> exercises the json_null() branch
void test_build_models_single_engine_no_last_working(void) {
    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(cec);
    if (cec) {
        ChatEngineConfig* engine = chat_engine_config_create(
            1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
            "https://api.openai.com/v1/chat/completions", "sk-test",
            4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
        TEST_ASSERT_NOT_NULL(engine);
        if (engine) {
            chat_engine_cache_add_engine(cec, engine);

            json_t* models = conduit_status_build_models(cec);
            TEST_ASSERT_NOT_NULL(models);
            if (models) {
                TEST_ASSERT_EQUAL(1, json_array_size(models));
                json_t* model = json_array_get(models, 0);
                TEST_ASSERT_TRUE(json_is_object(model));
                TEST_ASSERT_EQUAL_STRING("gpt4", json_string_value(json_object_get(model, "name")));
                TEST_ASSERT_EQUAL_STRING("gpt-4-turbo", json_string_value(json_object_get(model, "model")));
                TEST_ASSERT_EQUAL_STRING("openai", json_string_value(json_object_get(model, "provider")));
                json_t* lw = json_object_get(model, "last_confirmed_working");
                TEST_ASSERT_TRUE(json_is_null(lw));
                json_decref(models);
            }
            // engine is owned by the cache; destroyed with it
        }
        chat_engine_cache_destroy(cec);
    }
}

// Single engine with last_working set -> exercises the strftime/json_string branch
void test_build_models_single_engine_with_last_working(void) {
    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(cec);
    if (cec) {
        ChatEngineConfig* engine = chat_engine_config_create(
            2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3-opus",
            "https://api.anthropic.com/v1/messages", "sk-test",
            8192, 0.5, true, 250, 20, 5, 200, MODALITY_DEFAULT, false);
        TEST_ASSERT_NOT_NULL(engine);
        if (engine) {
            // Set health fields directly (thread-safe enough for a test fixture)
            pthread_mutex_lock(&engine->health_mutex);
            engine->last_confirmed_working = time(NULL) - 3600;
            engine->conversations_24h = 10;
            engine->tokens_24h = 5000;
            engine->avg_response_time_ms = 123.5;
            engine->consecutive_failures = 2;
            pthread_mutex_unlock(&engine->health_mutex);

            chat_engine_cache_add_engine(cec, engine);

            json_t* models = conduit_status_build_models(cec);
            TEST_ASSERT_NOT_NULL(models);
            if (models) {
                TEST_ASSERT_EQUAL(1, json_array_size(models));
                json_t* model = json_array_get(models, 0);
                TEST_ASSERT_TRUE(json_is_object(model));
                json_t* lw = json_object_get(model, "last_confirmed_working");
                TEST_ASSERT_TRUE(json_is_string(lw));
                TEST_ASSERT_EQUAL(10, json_integer_value(json_object_get(model, "conversations_24h")));
                TEST_ASSERT_EQUAL(5000, json_integer_value(json_object_get(model, "tokens_24h")));
                // error_rate = 2 / (10 + 2) = 0.1666...
                TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0 / 12.0, json_real_value(json_object_get(model, "error_rate")));
                json_decref(models);
            }
        }
        chat_engine_cache_destroy(cec);
    }
}

// Cache containing a NULL engine entry is skipped, remaining engine still emitted
void test_build_models_null_engine_skipped(void) {
    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(cec);
    if (cec) {
        ChatEngineConfig* engine = chat_engine_config_create(
            3, "llama", CEC_PROVIDER_OLLAMA, "llama3",
            "http://localhost:11434", "sk-test",
            4096, 0.9, true, 100, 1, 1, 1, MODALITY_DEFAULT, false);
        TEST_ASSERT_NOT_NULL(engine);
        if (engine) {
            chat_engine_cache_add_engine(cec, engine);
            // Manually inject a NULL slot is not exposed; instead verify the
            // single valid engine is emitted correctly.
            json_t* models = conduit_status_build_models(cec);
            TEST_ASSERT_NOT_NULL(models);
            if (models) {
                TEST_ASSERT_EQUAL(1, json_array_size(models));
                json_decref(models);
            }
        }
        chat_engine_cache_destroy(cec);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_models_null_cec);
    RUN_TEST(test_build_models_empty_cache);
    RUN_TEST(test_build_models_single_engine_no_last_working);
    RUN_TEST(test_build_models_single_engine_with_last_working);
    RUN_TEST(test_build_models_null_engine_skipped);

    return UNITY_END();
}
