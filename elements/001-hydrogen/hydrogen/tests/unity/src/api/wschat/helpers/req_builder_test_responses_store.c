/*
 * Unity Tests for Chat Request Builder — CHAT_FINALE Phase 10a
 *
 * Tests the Responses API `store` data-residency knob:
 *   - Default false (opt-in to provider-side 30-day retention)
 *   - Explicit true on engine config emits true
 *   - Explicit false on engine config emits false
 *   - OpenAI/Anthropic/Ollama builders are unaffected (Responses-only field)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>

void setUp(void) { }
void tearDown(void) { }

// Test function prototypes
void test_responses_store_default_false(void);
void test_responses_store_explicit_true(void);
void test_responses_store_explicit_false(void);
void test_openai_builder_does_not_emit_store(void);
void test_anthropic_builder_does_not_emit_store(void);
void test_ollama_builder_does_not_emit_store(void);

static ChatEngineConfig* create_test_engine(ChatEngineProvider provider, bool use_native) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "test-engine", provider, "test-model",
        "https://api.test.com/v1/chat", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, use_native);
    if (engine) {
        engine->use_responses_api = true;
    }
    return engine;
}

// --- Responses store knob ---

void test_responses_store_default_false(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_FALSE(engine->store);  // CHAT_FINALE Phase 10a default

    json_t* body = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NOT_NULL(store);
    TEST_ASSERT_TRUE(json_is_boolean(store));
    TEST_ASSERT_FALSE(json_boolean_value(store));

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_store_explicit_true(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    engine->store = true;

    json_t* body = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NOT_NULL(store);
    TEST_ASSERT_TRUE(json_is_boolean(store));
    TEST_ASSERT_TRUE(json_boolean_value(store));

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_store_explicit_false(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    engine->store = false;

    json_t* body = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NOT_NULL(store);
    TEST_ASSERT_FALSE(json_boolean_value(store));

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Non-Responses builders must not emit store ---

void test_openai_builder_does_not_emit_store(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    engine->use_responses_api = false;  // Force Chat Completions builder
    engine->store = true;  // Even if set, Chat Completions must not carry it

    json_t* body = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NULL(store);

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_anthropic_builder_does_not_emit_store(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    engine->store = true;

    json_t* body = chat_request_build_anthropic(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NULL(store);

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_ollama_builder_does_not_emit_store(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NOT_NULL(engine);
    engine->store = true;

    json_t* body = chat_request_build_ollama(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(body);

    json_t* store = json_object_get(body, "store");
    TEST_ASSERT_NULL(store);

    json_decref(body);
    chat_message_destroy(messages);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_responses_store_default_false);
    RUN_TEST(test_responses_store_explicit_true);
    RUN_TEST(test_responses_store_explicit_false);
    RUN_TEST(test_openai_builder_does_not_emit_store);
    RUN_TEST(test_anthropic_builder_does_not_emit_store);
    RUN_TEST(test_ollama_builder_does_not_emit_store);

    return UNITY_END();
}