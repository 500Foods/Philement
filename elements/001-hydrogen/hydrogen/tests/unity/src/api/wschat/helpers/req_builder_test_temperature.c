/*
 * Unity Tests for Chat Request Builder — Phase 1: Temperature, Overlay, Responses
 *
 * Tests that temperature reaches the provider across all builders (OpenAI,
 * Ollama, Anthropic, Responses), the additional_params overlay merges in all
 * builders, and the Responses API routing uses use_responses_api.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>

void setUp(void) { }
void tearDown(void) { }

// Test function prototypes
void test_openai_temperature_uses_client_value(void);
void test_openai_temperature_uses_engine_default_when_omitted(void);
void test_openai_temperature_exactly_one(void);
void test_ollama_temperature_uses_client_value(void);
void test_ollama_temperature_uses_engine_default_when_omitted(void);
void test_anthropic_temperature_present_with_client_value(void);
void test_anthropic_temperature_uses_engine_default_when_omitted(void);
void test_responses_temperature_uses_client_value(void);
void test_responses_uses_input_not_messages(void);
void test_responses_uses_max_output_tokens(void);
void test_openai_overlay_merges_params(void);
void test_anthropic_overlay_merges_params(void);
void test_ollama_overlay_merges_params(void);
void test_responses_overlay_merges_params(void);
void test_dispatch_routes_responses_when_flag_set(void);
void test_dispatch_routes_openai_when_flag_clear(void);
void test_dispatch_anthropic_unaffected_by_responses_flag(void);

static ChatEngineConfig* create_test_engine(ChatEngineProvider provider, bool use_native) {
    return chat_engine_config_create(
        1, "test-engine", provider, "test-model",
        "https://api.test.com/v1/chat", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, use_native);
}

// --- OpenAI temperature ---

void test_openai_temperature_uses_client_value(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = 0.2;

    json_t* request = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.2, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_openai_temperature_uses_engine_default_when_omitted(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    engine->temperature_default = 0.5;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = -1.0;

    json_t* request = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_openai_temperature_exactly_one(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = 1.0;

    json_t* request = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Ollama temperature ---

void test_ollama_temperature_uses_client_value(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, true);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = 0.2;

    json_t* request = chat_request_build_ollama(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* options = json_object_get(request, "options");
    TEST_ASSERT_NOT_NULL(options);
    json_t* temperature = json_object_get(options, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.2, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_ollama_temperature_uses_engine_default_when_omitted(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, true);
    engine->temperature_default = 0.5;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = -1.0;

    json_t* request = chat_request_build_ollama(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* options = json_object_get(request, "options");
    TEST_ASSERT_NOT_NULL(options);
    json_t* temperature = json_object_get(options, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Anthropic temperature ---

void test_anthropic_temperature_present_with_client_value(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = 0.2;

    json_t* request = chat_request_build_anthropic(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.2, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_anthropic_temperature_uses_engine_default_when_omitted(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    engine->temperature_default = 0.5;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = -1.0;

    json_t* request = chat_request_build_anthropic(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Responses API builder ---

void test_responses_temperature_uses_client_value(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.temperature = 0.2;

    json_t* request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.2, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_uses_input_not_messages(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* input = json_object_get(request, "input");
    TEST_ASSERT_NOT_NULL(input);
    TEST_ASSERT_EQUAL(1, json_array_size(input));

    json_t* messages_field = json_object_get(request, "messages");
    TEST_ASSERT_NULL(messages_field);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_uses_max_output_tokens(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.max_tokens = 2048;

    json_t* request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* max_output = json_object_get(request, "max_output_tokens");
    TEST_ASSERT_NOT_NULL(max_output);
    TEST_ASSERT_EQUAL(2048, json_integer_value(max_output));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Overlay (additional_params) ---

void test_openai_overlay_merges_params(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* overlay = json_object();
    json_object_set_new(overlay, "reasoning_effort", json_string("high"));
    params.additional_params = overlay;

    json_t* request = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* effort = json_object_get(request, "reasoning_effort");
    TEST_ASSERT_NOT_NULL(effort);
    TEST_ASSERT_EQUAL_STRING("high", json_string_value(effort));

    json_decref(request);
    json_decref(overlay);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_anthropic_overlay_merges_params(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* overlay = json_object();
    json_object_set_new(overlay, "thinking", json_pack("{s:b}", "budget_tokens", 10000));
    params.additional_params = overlay;

    json_t* request = chat_request_build_anthropic(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* thinking = json_object_get(request, "thinking");
    TEST_ASSERT_NOT_NULL(thinking);

    json_decref(request);
    json_decref(overlay);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_ollama_overlay_merges_params(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, true);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* overlay = json_object();
    json_object_set_new(overlay, "num_ctx", json_integer(8192));
    params.additional_params = overlay;

    json_t* request = chat_request_build_ollama(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* num_ctx = json_object_get(request, "num_ctx");
    TEST_ASSERT_NOT_NULL(num_ctx);
    TEST_ASSERT_EQUAL(8192, json_integer_value(num_ctx));

    json_decref(request);
    json_decref(overlay);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_overlay_merges_params(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* overlay = json_object();
    json_object_set_new(overlay, "reasoning_effort", json_string("medium"));
    params.additional_params = overlay;

    json_t* request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* effort = json_object_get(request, "reasoning_effort");
    TEST_ASSERT_NOT_NULL(effort);
    TEST_ASSERT_EQUAL_STRING("medium", json_string_value(effort));

    json_decref(request);
    json_decref(overlay);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// --- Responses routing ---

void test_dispatch_routes_responses_when_flag_set(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    engine->use_responses_api = true;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* input = json_object_get(request, "input");
    TEST_ASSERT_NOT_NULL(input);

    json_t* max_output = json_object_get(request, "max_output_tokens");
    TEST_ASSERT_NOT_NULL(max_output);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_dispatch_routes_openai_when_flag_clear(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    engine->use_responses_api = false;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);

    json_t* max_tokens = json_object_get(request, "max_tokens");
    TEST_ASSERT_NOT_NULL(max_tokens);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_dispatch_anthropic_unaffected_by_responses_flag(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    engine->use_responses_api = true;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);

    json_t* temperature = json_object_get(request, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_emits_reasoning_effort(void);
void test_responses_omits_reasoning_when_null(void);

// --- Responses reasoning ---

void test_responses_emits_reasoning_effort(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "test-engine", CEC_PROVIDER_OPENAI, "test-model",
        "https://api.test.com/v1/responses", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->use_responses_api = true;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.reasoning = strdup("high");

    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* reasoning = json_object_get(request, "reasoning");
    TEST_ASSERT_NOT_NULL(reasoning);
    TEST_ASSERT_TRUE(json_is_object(reasoning));
    json_t* effort = json_object_get(reasoning, "effort");
    TEST_ASSERT_NOT_NULL(effort);
    TEST_ASSERT_EQUAL_STRING("high", json_string_value(effort));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
    free(params.reasoning);
}

void test_responses_omits_reasoning_when_null(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "test-engine", CEC_PROVIDER_OPENAI, "test-model",
        "https://api.test.com/v1/responses", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->use_responses_api = true;
    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t* reasoning = json_object_get(request, "reasoning");
    TEST_ASSERT_NULL(reasoning);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_openai_temperature_uses_client_value);
    RUN_TEST(test_openai_temperature_uses_engine_default_when_omitted);
    RUN_TEST(test_openai_temperature_exactly_one);
    RUN_TEST(test_ollama_temperature_uses_client_value);
    RUN_TEST(test_ollama_temperature_uses_engine_default_when_omitted);
    RUN_TEST(test_anthropic_temperature_present_with_client_value);
    RUN_TEST(test_anthropic_temperature_uses_engine_default_when_omitted);
    RUN_TEST(test_responses_temperature_uses_client_value);
    RUN_TEST(test_responses_uses_input_not_messages);
    RUN_TEST(test_responses_uses_max_output_tokens);
    RUN_TEST(test_openai_overlay_merges_params);
    RUN_TEST(test_anthropic_overlay_merges_params);
    RUN_TEST(test_ollama_overlay_merges_params);
    RUN_TEST(test_responses_overlay_merges_params);
    RUN_TEST(test_dispatch_routes_responses_when_flag_set);
    RUN_TEST(test_dispatch_routes_openai_when_flag_clear);
    RUN_TEST(test_dispatch_anthropic_unaffected_by_responses_flag);
    RUN_TEST(test_responses_emits_reasoning_effort);
    RUN_TEST(test_responses_omits_reasoning_when_null);

    return UNITY_END();
}
