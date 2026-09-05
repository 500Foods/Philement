#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/engine_cache.h>

void test_engine_config_create_basic(void);
void test_engine_config_create_null_name(void);
void test_engine_config_create_null_model(void);
void test_engine_config_destroy_null(void);
void test_engine_config_update_health_success(void);
void test_engine_config_update_health_failure(void);
void test_engine_config_get_status_healthy(void);
void test_engine_config_get_status_degraded(void);
void test_engine_config_get_status_unavailable(void);
void test_engine_config_get_status_null(void);
void test_engine_provider_from_string(void);
void test_engine_provider_from_string_unknown(void);
void test_engine_provider_to_string(void);
void test_engine_cache_create_destroy(void);
void test_engine_cache_add_engine_null_params(void);
void test_engine_cache_add_and_lookup(void);
void test_engine_cache_lookup_null_params(void);
void test_engine_cache_get_default_null(void);
void test_engine_cache_get_default_no_engines(void);
void test_engine_cache_get_engine_count_null(void);
void test_engine_cache_get_engine_count_after_add(void);
void test_engine_cache_add_engine_locked_null(void);
void test_engine_cache_get_all_null_params(void);
void test_engine_cache_get_all_valid(void);

void setUp(void) {}
void tearDown(void) {}

void test_engine_config_create_basic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat", "sk-test-key",
        4096, 0.7, true, 300, 10, 50, 10,
        MODALITY_TEXT | MODALITY_IMAGE, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_INT(1, engine->engine_id);
    TEST_ASSERT_EQUAL_STRING("gpt4", engine->name);
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, engine->provider);
    TEST_ASSERT_EQUAL_STRING("gpt-4-turbo", engine->model);
    TEST_ASSERT_EQUAL_STRING("https://api.openai.com/v1/chat", engine->api_url);
    TEST_ASSERT_EQUAL_INT(4096, engine->max_tokens);
    TEST_ASSERT_EQUAL_DOUBLE(0.7, engine->temperature_default);
    TEST_ASSERT_TRUE(engine->is_default);
    TEST_ASSERT_EQUAL_INT(300, engine->liveliness_seconds);
    TEST_ASSERT_TRUE(engine->is_healthy);
    TEST_ASSERT_EQUAL_INT(MODALITY_TEXT | MODALITY_IMAGE, engine->supported_modalities);
    chat_engine_config_destroy(engine);
}

void test_engine_config_create_null_name(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        2, NULL, CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_STRING("", engine->name);
    chat_engine_config_destroy(engine);
}

void test_engine_config_create_null_model(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        3, "test", CEC_PROVIDER_ANTHROPIC, NULL,
        "https://api.anthropic.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL_STRING("", engine->model);
    chat_engine_config_destroy(engine);
}

void test_engine_config_destroy_null(void) {
    chat_engine_config_destroy(NULL);
}

void test_engine_config_update_health_success(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    TEST_ASSERT_NOT_NULL(engine);

    chat_engine_config_update_health(engine, true, 150.0);
    TEST_ASSERT_TRUE(engine->is_healthy);
    TEST_ASSERT_EQUAL_INT(0, engine->consecutive_failures);
    TEST_ASSERT_EQUAL_DOUBLE(150.0, engine->avg_response_time_ms);

    chat_engine_config_destroy(engine);
}

void test_engine_config_update_health_failure(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    TEST_ASSERT_NOT_NULL(engine);

    chat_engine_config_update_health(engine, false, 0.0);
    TEST_ASSERT_EQUAL_INT(1, engine->consecutive_failures);
    TEST_ASSERT_TRUE(engine->is_healthy);  /* Still healthy after first failure */

    chat_engine_config_update_health(engine, false, 0.0);
    chat_engine_config_update_health(engine, false, 0.0);
    TEST_ASSERT_FALSE(engine->is_healthy);  /* Unhealthy after 3 consecutive failures */

    chat_engine_config_destroy(engine);
}

void test_engine_config_get_status_healthy(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    TEST_ASSERT_EQUAL_STRING("healthy", chat_engine_config_get_status(engine));
    chat_engine_config_destroy(engine);
}

void test_engine_config_get_status_degraded(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    chat_engine_config_update_health(engine, false, 0.0);
    TEST_ASSERT_EQUAL_STRING("degraded", chat_engine_config_get_status(engine));
    chat_engine_config_destroy(engine);
}

void test_engine_config_get_status_unavailable(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, false, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    chat_engine_config_update_health(engine, false, 0.0);
    chat_engine_config_update_health(engine, false, 0.0);
    chat_engine_config_update_health(engine, false, 0.0);
    TEST_ASSERT_EQUAL_STRING("unavailable", chat_engine_config_get_status(engine));
    chat_engine_config_destroy(engine);
}

void test_engine_config_get_status_null(void) {
    TEST_ASSERT_EQUAL_STRING("unknown", chat_engine_config_get_status(NULL));
}

void test_engine_provider_from_string(void) {
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("openai"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_ANTHROPIC, chat_engine_provider_from_string("anthropic"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OLLAMA, chat_engine_provider_from_string("ollama"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("XAI"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("gradient"));
}

void test_engine_provider_from_string_unknown(void) {
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string("unknown"));
    TEST_ASSERT_EQUAL_INT(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string(NULL));
}

void test_engine_provider_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("openai", chat_engine_provider_to_string(CEC_PROVIDER_OPENAI));
    TEST_ASSERT_EQUAL_STRING("anthropic", chat_engine_provider_to_string(CEC_PROVIDER_ANTHROPIC));
    TEST_ASSERT_EQUAL_STRING("ollama", chat_engine_provider_to_string(CEC_PROVIDER_OLLAMA));
    TEST_ASSERT_EQUAL_STRING("unknown", chat_engine_provider_to_string(CEC_PROVIDER_UNKNOWN));
}

void test_engine_cache_create_destroy(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_NOT_NULL(cache->database_name);
    TEST_ASSERT_EQUAL_STRING("testdb", cache->database_name);
    TEST_ASSERT_EQUAL_size_t(0, cache->engine_count);
    TEST_ASSERT_NOT_NULL(cache->engines);
}

void test_engine_cache_add_engine_null_params(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(cache);

    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);

    TEST_ASSERT_FALSE(chat_engine_cache_add_engine(NULL, engine));
    TEST_ASSERT_FALSE(chat_engine_cache_add_engine(cache, NULL));

    chat_engine_config_destroy(engine);
}

void test_engine_cache_add_and_lookup(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb2");
    TEST_ASSERT_NOT_NULL(cache);

    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);

    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(cache, engine));
    TEST_ASSERT_EQUAL_size_t(1, chat_engine_cache_get_engine_count(cache));

    ChatEngineConfig *found = chat_engine_cache_lookup_by_name(cache, "gpt4");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(1, found->engine_id);

    ChatEngineConfig *not_found = chat_engine_cache_lookup_by_name(cache, "nonexistent");
    TEST_ASSERT_NULL(not_found);

    chat_engine_config_destroy(engine);
}

void test_engine_cache_lookup_null_params(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb3");
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_name(NULL, "name"));
    TEST_ASSERT_NULL(chat_engine_cache_lookup_by_name(cache, NULL));
}

void test_engine_cache_get_default_null(void) {
    TEST_ASSERT_NULL(chat_engine_cache_get_default(NULL));
}

void test_engine_cache_get_default_no_engines(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb4");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_NULL(chat_engine_cache_get_default(cache));
}

void test_engine_cache_get_engine_count_null(void) {
    TEST_ASSERT_EQUAL_size_t(0, chat_engine_cache_get_engine_count(NULL));
}

void test_engine_cache_get_engine_count_after_add(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb5");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL_size_t(0, chat_engine_cache_get_engine_count(cache));

    ChatEngineConfig *e1 = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4", "url", "key",
        4096, 0.7, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    ChatEngineConfig *e2 = chat_engine_config_create(
        2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3", "url", "key",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(cache, e1);
    chat_engine_cache_add_engine(cache, e2);
    TEST_ASSERT_EQUAL_size_t(2, chat_engine_cache_get_engine_count(cache));

    /* Default should be e2 (is_default=true) */
    ChatEngineConfig *def = chat_engine_cache_get_default(cache);
    TEST_ASSERT_NOT_NULL(def);
    TEST_ASSERT_EQUAL_STRING("claude", def->name);

    chat_engine_config_destroy(e1);
    chat_engine_config_destroy(e2);
}

void test_engine_cache_add_engine_locked_null(void) {
    TEST_ASSERT_FALSE(chat_engine_cache_add_engine_locked(NULL, NULL));
}

void test_engine_cache_get_all_null_params(void) {
    size_t count = 0;
    TEST_ASSERT_NULL(chat_engine_cache_get_all(NULL, NULL));
    TEST_ASSERT_NULL(chat_engine_cache_get_all(NULL, &count));
}

void test_engine_cache_get_all_valid(void) {
    ChatEngineCache *cache = chat_engine_cache_create("testdb6");
    TEST_ASSERT_NOT_NULL(cache);

    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4", "url", "key",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    chat_engine_cache_add_engine(cache, engine);

    size_t count = 0;
    ChatEngineConfig **all = chat_engine_cache_get_all(cache, &count);
    TEST_ASSERT_NOT_NULL(all);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING("gpt4", all[0]->name);
    free(all);

    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_config_create_basic);
    RUN_TEST(test_engine_config_create_null_name);
    RUN_TEST(test_engine_config_create_null_model);
    RUN_TEST(test_engine_config_destroy_null);
    RUN_TEST(test_engine_config_update_health_success);
    RUN_TEST(test_engine_config_update_health_failure);
    RUN_TEST(test_engine_config_get_status_healthy);
    RUN_TEST(test_engine_config_get_status_degraded);
    RUN_TEST(test_engine_config_get_status_unavailable);
    RUN_TEST(test_engine_config_get_status_null);
    RUN_TEST(test_engine_provider_from_string);
    RUN_TEST(test_engine_provider_from_string_unknown);
    RUN_TEST(test_engine_provider_to_string);
    RUN_TEST(test_engine_cache_create_destroy);
    RUN_TEST(test_engine_cache_add_engine_null_params);
    RUN_TEST(test_engine_cache_add_and_lookup);
    RUN_TEST(test_engine_cache_lookup_null_params);
    RUN_TEST(test_engine_cache_get_default_null);
    RUN_TEST(test_engine_cache_get_default_no_engines);
    RUN_TEST(test_engine_cache_get_engine_count_null);
    RUN_TEST(test_engine_cache_get_engine_count_after_add);
    RUN_TEST(test_engine_cache_add_engine_locked_null);
    RUN_TEST(test_engine_cache_get_all_null_params);
    RUN_TEST(test_engine_cache_get_all_valid);
    return UNITY_END();
}
