#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/local_mcp.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

void test_proxy_allowlisted_call(void);
void test_proxy_rejects_unknown_tool(void);
ChatEngineConfig *engine_with_local_mcp(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
}

ChatEngineConfig *engine_with_local_mcp(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    json_t *collection = json_loads(
        "{\"local_mcp\":{\"enabled\":true,\"servers\":["
        "{\"url\":\"https://mcp.example.com/mcp\",\"allowed_tools\":[\"System.Info\"]}]}}",
        0, NULL);
    chat_local_mcp_config_load(collection, &engine->local_mcp);
    json_decref(collection);
    return engine;
}

void test_proxy_allowlisted_call(void) {
    ChatEngineConfig *engine = engine_with_local_mcp();
    oidc_rp_http_test_set_response("mcp.example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response("mcp.example.com", 202, "");
    oidc_rp_http_test_set_response("mcp.example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"pong\"}],\"isError\":false}}");
    json_t *calls = json_array();
    json_t *args = json_object();
    json_t *call = chat_local_mcp_normalize_call("call_1", "System.Info", args);
    json_decref(args);
    json_array_append_new(calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, calls, "cid");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    TEST_ASSERT_EQUAL_STRING("pong",
        json_string_value(json_object_get(json_array_get(results, 0), "content")));
    json_decref(calls);
    json_decref(results);
    chat_engine_config_destroy(engine);
}

void test_proxy_rejects_unknown_tool(void) {
    ChatEngineConfig *engine = engine_with_local_mcp();
    json_t *calls = json_array();
    json_t *args = json_object();
    json_t *call = chat_local_mcp_normalize_call("call_1", "H.query", args);
    json_decref(args);
    json_array_append_new(calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, calls, "cid");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    TEST_ASSERT_NOT_NULL(strstr(
        json_string_value(json_object_get(json_array_get(results, 0), "content")),
        "allowlisted"));
    json_decref(calls);
    json_decref(results);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_proxy_allowlisted_call);
    RUN_TEST(test_proxy_rejects_unknown_tool);
    return UNITY_END();
}
