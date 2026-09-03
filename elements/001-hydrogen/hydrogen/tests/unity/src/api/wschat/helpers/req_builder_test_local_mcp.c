#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>

void test_openai_injects_function_tools(void);
void test_responses_injects_function_tools(void);
void test_anthropic_injects_tools(void);
void test_skipped_when_null(void);
ChatEngineConfig *make_engine(ChatEngineProvider provider, bool responses);
json_t *mcp_tools(void);

void setUp(void) {}
void tearDown(void) {}

ChatEngineConfig *make_engine(ChatEngineProvider provider, bool responses) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", provider, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    if (engine) {
        engine->use_responses_api = responses;
    }
    return engine;
}

json_t *mcp_tools(void) {
    json_t *tools = json_array();
    json_t *tool = json_object();
    json_object_set_new(tool, "name", json_string("System.Info"));
    json_object_set_new(tool, "description", json_string("info"));
    json_object_set_new(tool, "inputSchema", json_object());
    json_array_append_new(tools, tool);
    return tools;
}

void test_openai_injects_function_tools(void) {
    ChatEngineConfig *engine = make_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.local_mcp_tools = mcp_tools();
    json_t *req = chat_request_build_openai(engine, messages, &params);
    json_t *tools = json_object_get(req, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_STRING("function",
        json_string_value(json_object_get(json_array_get(tools, 0), "type")));
    json_decref(params.local_mcp_tools);
    json_decref(req);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_responses_injects_function_tools(void) {
    ChatEngineConfig *engine = make_engine(CEC_PROVIDER_OPENAI, true);
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.local_mcp_tools = mcp_tools();
    json_t *req = chat_request_build_responses(engine, messages, &params);
    json_t *tools = json_object_get(req, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_STRING("function",
        json_string_value(json_object_get(json_array_get(tools, 0), "type")));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(tools, 0), "name")));
    json_decref(params.local_mcp_tools);
    json_decref(req);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_anthropic_injects_tools(void) {
    ChatEngineConfig *engine = make_engine(CEC_PROVIDER_ANTHROPIC, false);
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.local_mcp_tools = mcp_tools();
    json_t *req = chat_request_build_anthropic(engine, messages, &params);
    json_t *tools = json_object_get(req, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(tools, 0), "name")));
    TEST_ASSERT_NOT_NULL(json_object_get(json_array_get(tools, 0), "input_schema"));
    json_decref(params.local_mcp_tools);
    json_decref(req);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_skipped_when_null(void) {
    ChatEngineConfig *engine = make_engine(CEC_PROVIDER_OPENAI, false);
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    json_t *req = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NULL(json_object_get(req, "tools"));
    json_decref(req);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_openai_injects_function_tools);
    RUN_TEST(test_responses_injects_function_tools);
    RUN_TEST(test_anthropic_injects_tools);
    RUN_TEST(test_skipped_when_null);
    return UNITY_END();
}
