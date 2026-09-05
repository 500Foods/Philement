#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/req_builder.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <string.h>

void test_request_params_default(void);
void test_message_create_basic(void);
void test_message_create_null_content(void);
void test_message_destroy_null(void);
void test_message_list_destroy_null(void);
void test_message_list_append_empty(void);
void test_message_list_append_one(void);
void test_message_role_to_string(void);
void test_message_role_from_string(void);
void test_message_role_from_string_null(void);
void test_request_build_openai_basic(void);
void test_request_build_openai_null_engine(void);
void test_request_build_openai_with_stream(void);
void test_request_build_openai_with_reasoning(void);
void test_request_build_anthropic_basic(void);
void test_request_build_ollama_basic(void);
void test_request_build_responses_basic(void);
void test_request_build_generic_openai(void);
void test_request_build_generic_anthropic(void);
void test_request_build_generic_ollama(void);
void test_request_build_generic_unknown_provider(void);
void test_build_messages_array_empty(void);
void test_build_messages_array_with_messages(void);
void test_to_json_string_compact(void);
void test_to_json_string_pretty(void);
void test_correlation_id_generate(void);
void test_correlation_id_generate_short_buffer(void);
void test_correlation_id_generate_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_request_params_default(void) {
    ChatRequestParams params = chat_request_params_default();
    TEST_ASSERT_NULL(params.model);
    TEST_ASSERT_EQUAL_DOUBLE(-1.0, params.temperature);
    TEST_ASSERT_EQUAL_INT(-1, params.max_tokens);
    TEST_ASSERT_FALSE(params.stream);
    TEST_ASSERT_NULL(params.reasoning);
    TEST_ASSERT_NULL(params.additional_params);
    TEST_ASSERT_FALSE(params.hosted_mcp_enabled);
    TEST_ASSERT_NULL(params.hosted_mcp_sub);
    TEST_ASSERT_NULL(params.local_mcp_tools);
}

void test_message_create_basic(void) {
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, msg->role);
    TEST_ASSERT_EQUAL_STRING("Hello", msg->content);
    TEST_ASSERT_NULL(msg->name);
    TEST_ASSERT_NULL(msg->next);
    chat_message_destroy(msg);
}

void test_message_create_null_content(void) {
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, NULL, NULL);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NULL(msg->content);
    chat_message_destroy(msg);
}

void test_message_destroy_null(void) {
    chat_message_destroy(NULL);
}

void test_message_list_destroy_null(void) {
    chat_message_list_destroy(NULL);
}

void test_message_list_append_empty(void) {
    ChatMessage *head = NULL;
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "first", NULL);
    head = chat_message_list_append(head, msg);
    TEST_ASSERT_EQUAL_PTR(msg, head);
    TEST_ASSERT_NULL(head->next);
    chat_message_list_destroy(head);
}

void test_message_list_append_one(void) {
    ChatMessage *msg1 = chat_message_create(CHAT_ROLE_USER, "first", NULL);
    ChatMessage *msg2 = chat_message_create(CHAT_ROLE_ASSISTANT, "second", NULL);
    ChatMessage *head = chat_message_list_append(msg1, msg2);
    TEST_ASSERT_EQUAL_PTR(msg1, head);
    TEST_ASSERT_EQUAL_PTR(msg2, head->next);
    chat_message_list_destroy(head);
}

void test_message_role_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("system", chat_message_role_to_string(CHAT_ROLE_SYSTEM));
    TEST_ASSERT_EQUAL_STRING("user", chat_message_role_to_string(CHAT_ROLE_USER));
    TEST_ASSERT_EQUAL_STRING("assistant", chat_message_role_to_string(CHAT_ROLE_ASSISTANT));
    TEST_ASSERT_EQUAL_STRING("tool", chat_message_role_to_string(CHAT_ROLE_TOOL));
    TEST_ASSERT_EQUAL_STRING("user", chat_message_role_to_string(CHAT_ROLE_UNKNOWN));
}

void test_message_role_from_string(void) {
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_SYSTEM, chat_message_role_from_string("system"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, chat_message_role_from_string("user"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_ASSISTANT, chat_message_role_from_string("assistant"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_TOOL, chat_message_role_from_string("tool"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_UNKNOWN, chat_message_role_from_string("unknown"));
}

void test_message_role_from_string_null(void) {
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, chat_message_role_from_string(NULL));
}

void test_request_build_openai_basic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build_openai(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_EQUAL_STRING("gpt-4", json_string_value(json_object_get(req, "model")));
    TEST_ASSERT_TRUE(json_array_size(json_object_get(req, "messages")) > 0);

    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_openai_null_engine(void) {
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    TEST_ASSERT_NULL(chat_request_build_openai(NULL, msg, &params));
    chat_message_destroy(msg);
}

void test_request_build_openai_with_stream(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.stream = true;

    json_t *req = chat_request_build_openai(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(req, "stream")));

    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_openai_with_reasoning(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.reasoning = strdup("high");

    /* Standard chat completions API does not support reasoning field */
    json_t *req = chat_request_build_openai(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_NULL(json_object_get(req, "reasoning"));

    free(params.reasoning);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_anthropic_basic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3",
        "https://api.anthropic.com", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build_anthropic(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_ollama_basic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "llama", CEC_PROVIDER_OLLAMA, "llama2",
        "http://localhost:11434", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build_ollama(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_responses_basic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    engine->use_responses_api = true;
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build_responses(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_generic_openai(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4",
        "https://api.openai.com/v1/chat", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_generic_anthropic(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3",
        "https://api.anthropic.com", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_generic_ollama(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "llama", CEC_PROVIDER_OLLAMA, "llama2",
        "http://localhost:11434", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_request_build_generic_unknown_provider(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "unknown", CEC_PROVIDER_UNKNOWN, "model",
        "http://localhost:8080", "key",
        4096, 0.7, true, 300, 10, 10, 100,
        MODALITY_DEFAULT, false);
    ChatMessage *msg = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t *req = chat_request_build(engine, msg, &params);
    TEST_ASSERT_NOT_NULL(req);
    json_decref(req);
    chat_message_destroy(msg);
    chat_engine_config_destroy(engine);
}

void test_build_messages_array_empty(void) {
    json_t *arr = chat_request_build_messages_array(NULL);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_size_t(0, json_array_size(arr));
    json_decref(arr);
}

void test_build_messages_array_with_messages(void) {
    ChatMessage *msg1 = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatMessage *msg2 = chat_message_create(CHAT_ROLE_ASSISTANT, "Hello", NULL);
    msg1->next = msg2;

    json_t *arr = chat_request_build_messages_array(msg1);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_size_t(2, json_array_size(arr));

    json_decref(arr);
    chat_message_list_destroy(msg1);
}

void test_to_json_string_compact(void) {
    json_t *obj = json_object();
    json_object_set_new(obj, "key", json_string("value"));
    char *str = chat_request_to_json_string(obj, true);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_NOT_NULL(strstr(str, "key"));
    TEST_ASSERT_NULL(strstr(str, "\n"));
    free(str);
    json_decref(obj);
}

void test_to_json_string_pretty(void) {
    json_t *obj = json_object();
    json_object_set_new(obj, "key", json_string("value"));
    char *str = chat_request_to_json_string(obj, false);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_NOT_NULL(strchr(str, '\n'));
    free(str);
    json_decref(obj);
}

void test_correlation_id_generate(void) {
    char buffer[64];
    chat_correlation_id_generate(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_size_t(36, strlen(buffer));
    /* Check format: 8-4-4-4-12 with dashes at positions 8, 13, 18, 23 */
    TEST_ASSERT_EQUAL_INT('-', buffer[8]);
    TEST_ASSERT_EQUAL_INT('-', buffer[13]);
    TEST_ASSERT_EQUAL_INT('-', buffer[18]);
    TEST_ASSERT_EQUAL_INT('-', buffer[23]);
    /* Position 14 should be '4' (UUID v4) */
    TEST_ASSERT_EQUAL_INT('4', buffer[14]);
}

void test_correlation_id_generate_short_buffer(void) {
    char buffer[10];
    chat_correlation_id_generate(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("", buffer);
}

void test_correlation_id_generate_null(void) {
    chat_correlation_id_generate(NULL, 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_request_params_default);
    RUN_TEST(test_message_create_basic);
    RUN_TEST(test_message_create_null_content);
    RUN_TEST(test_message_destroy_null);
    RUN_TEST(test_message_list_destroy_null);
    RUN_TEST(test_message_list_append_empty);
    RUN_TEST(test_message_list_append_one);
    RUN_TEST(test_message_role_to_string);
    RUN_TEST(test_message_role_from_string);
    RUN_TEST(test_message_role_from_string_null);
    RUN_TEST(test_request_build_openai_basic);
    RUN_TEST(test_request_build_openai_null_engine);
    RUN_TEST(test_request_build_openai_with_stream);
    RUN_TEST(test_request_build_openai_with_reasoning);
    RUN_TEST(test_request_build_anthropic_basic);
    RUN_TEST(test_request_build_ollama_basic);
    RUN_TEST(test_request_build_responses_basic);
    RUN_TEST(test_request_build_generic_openai);
    RUN_TEST(test_request_build_generic_anthropic);
    RUN_TEST(test_request_build_generic_ollama);
    RUN_TEST(test_request_build_generic_unknown_provider);
    RUN_TEST(test_build_messages_array_empty);
    RUN_TEST(test_build_messages_array_with_messages);
    RUN_TEST(test_to_json_string_compact);
    RUN_TEST(test_to_json_string_pretty);
    RUN_TEST(test_correlation_id_generate);
    RUN_TEST(test_correlation_id_generate_short_buffer);
    RUN_TEST(test_correlation_id_generate_null);
    return UNITY_END();
}
