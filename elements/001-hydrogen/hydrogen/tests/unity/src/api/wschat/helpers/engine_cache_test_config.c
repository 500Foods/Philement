/*
 * Unity Test File: chat_engine_config_create / chat_engine_config_clear_key
 * This file contains unit tests for chat_engine_config_create() and
 * chat_engine_config_clear_key() in src/api/wschat/helpers/engine_cache.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/engine_cache.h>

void test_config_create_valid(void);
void test_config_create_null_strings(void);
void test_config_create_defaults(void);
void test_config_clear_key_null(void);
void test_config_clear_key_wipes(void);

void setUp(void) {
}

void tearDown(void) {
}

/* Valid creation populates fields correctly */
void test_config_create_valid(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        7, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4", "https://api.example.com",
        "secret-key", 2048, 0.7, true, 600, 20, 25, 200, MODALITY_TEXT, true);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_INT(7, engine->engine_id);
    TEST_ASSERT_EQUAL_STRING("gpt4", engine->name);
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OPENAI, engine->provider);
    TEST_ASSERT_EQUAL_STRING("gpt-4", engine->model);
    TEST_ASSERT_EQUAL_STRING("https://api.example.com", engine->api_url);
    TEST_ASSERT_EQUAL_STRING("secret-key", engine->api_key);
    TEST_ASSERT_EQUAL_INT(2048, engine->max_tokens);
    TEST_ASSERT_EQUAL_INT(true, engine->is_default);
    TEST_ASSERT_EQUAL_INT(600, engine->liveliness_seconds);
    TEST_ASSERT_EQUAL_INT(20, engine->max_images_per_message);
    TEST_ASSERT_EQUAL_INT(25, engine->max_payload_mb);
    TEST_ASSERT_EQUAL_INT(200, engine->max_concurrent_requests);
    TEST_ASSERT_EQUAL_INT(MODALITY_TEXT, engine->supported_modalities);
    TEST_ASSERT_EQUAL_INT(true, engine->use_native_api);
    TEST_ASSERT_EQUAL_INT(true, engine->is_healthy);
    chat_engine_config_destroy(engine);
}

/* NULL string params leave fields empty (covers else branches) */
void test_config_create_null_strings(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, NULL, CEC_PROVIDER_OLLAMA, NULL, NULL, NULL,
        0, 0.0, false, -5, -1, -1, -1, 0, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_INT(0, engine->name[0]);
    TEST_ASSERT_EQUAL_INT(0, engine->model[0]);
    TEST_ASSERT_EQUAL_INT(0, engine->api_url[0]);
    TEST_ASSERT_EQUAL_INT(0, engine->api_key[0]);
    /* Defaults applied for out-of-range limits */
    TEST_ASSERT_EQUAL_INT(300, engine->liveliness_seconds);
    TEST_ASSERT_EQUAL_INT(10, engine->max_images_per_message);
    TEST_ASSERT_EQUAL_INT(10, engine->max_payload_mb);
    TEST_ASSERT_EQUAL_INT(100, engine->max_concurrent_requests);
    TEST_ASSERT_EQUAL_INT(MODALITY_DEFAULT, engine->supported_modalities);
    chat_engine_config_destroy(engine);
}

/* Liveliness clamped to 3600 max */
void test_config_create_defaults(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        2, "x", CEC_PROVIDER_ANTHROPIC, "claude", "url", "k",
        4096, 1.0, false, 9999, 5, 5, 5, MODALITY_ALL, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_INT(3600, engine->liveliness_seconds);
    TEST_ASSERT_EQUAL_INT(MODALITY_ALL, engine->supported_modalities);
    chat_engine_config_destroy(engine);
}

/* NULL engine is a no-op */
void test_config_clear_key_null(void) {
    chat_engine_config_clear_key(NULL);
}

/* Key is wiped to zero after clear */
void test_config_clear_key_wipes(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        3, "n", CEC_PROVIDER_OPENAI, "m", "u", "topsecretkey",
        0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_EQUAL_STRING("topsecretkey", engine->api_key);
    chat_engine_config_clear_key(engine);
    TEST_ASSERT_EQUAL_INT(0, engine->api_key[0]);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_create_valid);
    RUN_TEST(test_config_create_null_strings);
    RUN_TEST(test_config_create_defaults);
    RUN_TEST(test_config_clear_key_null);
    RUN_TEST(test_config_clear_key_wipes);
    return UNITY_END();
}
