/*
 * Unity Test File: local_mcp_test_error_paths
 * Error path tests for local_mcp.c helper functions.
 *
 * TEST_VERSION: 1.0.0
 */
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/local_mcp.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/mcp/mcp_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include "mock_auth_chat_deps.h"

void test_list_tools_null_engine(void);
void test_list_tools_disabled(void);
void test_list_tools_no_servers(void);
void test_list_tools_initialize_failure(void);
void test_list_tools_list_parse_failure(void);
void test_list_tools_happy_path(void);
void test_list_tools_with_allowed_filter(void);
void test_list_tools_empty_tools_returns_null(void);

void test_proxy_tool_calls_null_engine(void);
void test_proxy_tool_calls_null_tool_calls(void);
void test_proxy_tool_calls_server_not_found(void);
void test_proxy_tool_calls_initialize_fails(void);
void test_proxy_tool_calls_call_fails(void);
void test_proxy_tool_calls_happy_path(void);

void test_append_tool_results_null_args(void);
void test_append_tool_results_openai(void);
void test_append_tool_results_anthropic(void);
void test_append_tool_results_responses(void);
void test_append_tool_results_invalid_json(void);

void test_complete_request_round_trip(void);
void test_complete_request_local_mcp_max_rounds(void);
void test_complete_request_null_correlation_id(void);

void test_stream_next_body_happy_path(void);
void test_stream_next_body_disabled_mcp(void);
void test_stream_next_body_max_rounds(void);
void test_stream_next_body_no_tool_calls(void);
void test_stream_next_body_no_cid(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
    mock_auth_chat_deps_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    mock_auth_chat_deps_reset_all();
    mock_system_reset_all();
}

static ChatEngineConfig *make_engine_with_server(const char *url,
                                                 const char *auth,
                                                 const char *allowed_tools[],
                                                 size_t allowed_count) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = true;
    if (url) {
        ChatLocalMcpServer srv = {0};
        srv.url = strdup(url);
        if (auth) {
            srv.authorization = strdup(auth);
        }
        if (allowed_tools && allowed_count > 0) {
            srv.allowed_tools = calloc(allowed_count, sizeof(char *));
            for (size_t i = 0; i < allowed_count; i++) {
                srv.allowed_tools[i] = strdup(allowed_tools[i]);
            }
            srv.allowed_tool_count = allowed_count;
        }
        engine->local_mcp.servers = calloc(1, sizeof(ChatLocalMcpServer));
        engine->local_mcp.servers[0] = srv;
        engine->local_mcp.server_count = 1;
    }
    return engine;
}

static void free_engine(ChatEngineConfig *engine) {
    if (!engine) return;
    chat_local_mcp_config_cleanup(&engine->local_mcp);
    chat_engine_config_destroy(engine);
}

void test_list_tools_null_engine(void) {
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(NULL, "cid"));
}

void test_list_tools_disabled(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = false;
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(engine, "cid"));
    chat_engine_config_destroy(engine);
}

void test_list_tools_no_servers(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = true;
    engine->local_mcp.server_count = 0;
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(engine, "cid"));
    chat_engine_config_destroy(engine);
}

void test_list_tools_initialize_failure(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 500, "server error");
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(engine, "cid"));
    free_engine(engine);
}

void test_list_tools_list_parse_failure(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200, "not json");
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(engine, "cid"));
    free_engine(engine);
}

void test_list_tools_happy_path(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"tools\":["
        "{\"name\":\"System.Info\",\"description\":\"info\",\"inputSchema\":{\"type\":\"object\"}"
        "}]}}");
    json_t *tools = chat_local_mcp_list_tools(engine, "cid");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(tools, 0), "name")));
    json_decref(tools);
    free_engine(engine);
}

void test_list_tools_with_allowed_filter(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"tools\":["
        "{\"name\":\"System.Info\",\"inputSchema\":{\"type\":\"object\"}},"
        "{\"name\":\"System.Terminals\",\"inputSchema\":{\"type\":\"object\"}}]}}");
    json_t *tools = chat_local_mcp_list_tools(engine, "cid");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(tools, 0), "name")));
    json_decref(tools);
    free_engine(engine);
}

void test_list_tools_empty_tools_returns_null(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"tools\":[]}}");
    TEST_ASSERT_NULL(chat_local_mcp_list_tools(engine, "cid"));
    free_engine(engine);
}

void test_proxy_tool_calls_null_engine(void) {
    json_t *tool_calls = json_array();
    TEST_ASSERT_NOT_NULL(chat_local_mcp_proxy_tool_calls(NULL, tool_calls, "cid"));
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(
        chat_local_mcp_proxy_tool_calls(NULL, tool_calls, "cid")));
    json_decref(tool_calls);
}

void test_proxy_tool_calls_null_tool_calls(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, NULL, "cid");
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(results));
    json_decref(results);
    free_engine(engine);
}

void test_proxy_tool_calls_server_not_found(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_string("call_1"));
    json_object_set_new(call, "name", json_string("Unknown.Tool"));
    json_object_set_new(call, "arguments", json_object());
    json_array_append_new(tool_calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, tool_calls, "cid");
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"tool not allowlisted\"}",
        json_string_value(json_object_get(json_array_get(results, 0), "content")));
    json_decref(tool_calls);
    json_decref(results);
    free_engine(engine);
}

void test_proxy_tool_calls_initialize_fails(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 500, "server error");
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_string("call_1"));
    json_object_set_new(call, "name", json_string("System.Info"));
    json_object_set_new(call, "arguments", json_object());
    json_array_append_new(tool_calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, tool_calls, "cid");
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    json_decref(tool_calls);
    json_decref(results);
    free_engine(engine);
}

void test_proxy_tool_calls_call_fails(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200, "not json");
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_string("call_1"));
    json_object_set_new(call, "name", json_string("System.Info"));
    json_object_set_new(call, "arguments", json_object());
    json_array_append_new(tool_calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, tool_calls, "cid");
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    const char *content = json_string_value(json_object_get(json_array_get(results, 0), "content"));
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "invalid mcp json") != NULL ||
                     strstr(content, "tools/call failed") != NULL);
    json_decref(tool_calls);
    json_decref(results);
    free_engine(engine);
}

void test_proxy_tool_calls_happy_path(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"Hello World\"}]}}");
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_string("call_1"));
    json_object_set_new(call, "name", json_string("System.Info"));
    json_object_set_new(call, "arguments", json_object());
    json_array_append_new(tool_calls, call);
    json_t *results = chat_local_mcp_proxy_tool_calls(engine, tool_calls, "cid");
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(results));
    TEST_ASSERT_EQUAL_STRING("Hello World",
        json_string_value(json_object_get(json_array_get(results, 0), "content")));
    json_decref(tool_calls);
    json_decref(results);
    free_engine(engine);
}

void test_append_tool_results_null_args(void) {
    TEST_ASSERT_NULL(chat_local_mcp_append_tool_results(NULL, NULL, NULL,
        CEC_PROVIDER_OPENAI, false));
}

void test_append_tool_results_invalid_json(void) {
    json_t *tool_calls = json_array();
    json_t *results = json_array();
    TEST_ASSERT_NULL(chat_local_mcp_append_tool_results("not json",
        tool_calls, results, CEC_PROVIDER_OPENAI, false));
    json_decref(tool_calls);
    json_decref(results);
}

void test_append_tool_results_openai(void) {
    json_t *tool_calls = json_array();
    json_t *tc = json_object();
    json_object_set_new(tc, "id", json_string("call_1"));
    json_object_set_new(tc, "name", json_string("System.Info"));
    json_object_set_new(tc, "arguments", json_object());
    json_array_append_new(tool_calls, tc);

    json_t *results = json_array();
    json_t *row = json_object();
    json_object_set_new(row, "id", json_string("call_1"));
    json_object_set_new(row, "content", json_string("Hello World"));
    json_array_append_new(results, row);

    char *out = chat_local_mcp_append_tool_results(
        "{\"model\":\"x\",\"messages\":[]}", tool_calls, results,
        CEC_PROVIDER_OPENAI, false);
    TEST_ASSERT_NOT_NULL(out);
    json_t *parsed = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(parsed);
    json_t *messages = json_object_get(parsed, "messages");
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_EQUAL_UINT(2, json_array_size(messages));
    json_decref(parsed);
    free(out);
}

void test_append_tool_results_anthropic(void) {
    json_t *tool_calls = json_array();
    json_t *tc = json_object();
    json_object_set_new(tc, "id", json_string("call_1"));
    json_object_set_new(tc, "name", json_string("System.Info"));
    json_object_set_new(tc, "arguments", json_object());
    json_array_append_new(tool_calls, tc);

    json_t *results = json_array();
    json_t *row = json_object();
    json_object_set_new(row, "id", json_string("call_1"));
    json_object_set_new(row, "content", json_string("Hello World"));
    json_array_append_new(results, row);

    char *out = chat_local_mcp_append_tool_results(
        "{\"model\":\"x\",\"messages\":[]}", tool_calls, results,
        CEC_PROVIDER_ANTHROPIC, false);
    TEST_ASSERT_NOT_NULL(out);
    json_t *parsed = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(parsed);
    json_t *messages = json_object_get(parsed, "messages");
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_EQUAL_UINT(2, json_array_size(messages));
    json_decref(parsed);
    free(out);
}

void test_append_tool_results_responses(void) {
    json_t *tool_calls = json_array();
    json_t *tc = json_object();
    json_object_set_new(tc, "id", json_string("call_1"));
    json_object_set_new(tc, "name", json_string("System.Info"));
    json_object_set_new(tc, "arguments", json_object());
    json_array_append_new(tool_calls, tc);

    json_t *results = json_array();
    json_t *row = json_object();
    json_object_set_new(row, "id", json_string("call_1"));
    json_object_set_new(row, "content", json_string("Hello World"));
    json_array_append_new(results, row);

    char *out = chat_local_mcp_append_tool_results(
        "{\"model\":\"x\",\"input\":[]}", tool_calls, results,
        CEC_PROVIDER_OPENAI, true);
    TEST_ASSERT_NOT_NULL(out);
    json_t *parsed = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(parsed);
    json_t *input = json_object_get(parsed, "input");
    TEST_ASSERT_NOT_NULL(input);
    TEST_ASSERT_EQUAL_UINT(2, json_array_size(input));
    json_decref(parsed);
    free(out);
}

void test_complete_request_null_correlation_id(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = false;
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"choices\":[{\"message\":{\"content\":\"hello\"}}]}");
    struct ChatProxyResult *result = chat_local_mcp_complete_request(engine,
        "{\"model\":\"x\"}", NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->response_body);
    free(result->response_body);
    free(result->error_message);
    free(result);
    chat_engine_config_destroy(engine);
}

void test_complete_request_round_trip(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(
        "{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"call_1\","
        "\"function\":{\"name\":\"System.Info\",\"arguments\":\"{}\"}}]}}]}");
    for (int round = 0; round < CHAT_LOCAL_MCP_MAX_ROUNDS; round++) {
        oidc_rp_http_test_set_response(NULL, 200,
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
        oidc_rp_http_test_set_response(NULL, 200, "");
        oidc_rp_http_test_set_response(NULL, 200,
            "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"result\"}]}}");
    }
    struct ChatProxyResult *result = chat_local_mcp_complete_request(engine,
        "{\"model\":\"x\",\"messages\":[]}", "cid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->response_body);
    TEST_ASSERT_EQUAL_INT(CHAT_LOCAL_MCP_MAX_ROUNDS + 1, mock_auth_chat_deps_proxy_call_count());
    free(result->response_body);
    free(result->error_message);
    free(result);
    free_engine(engine);
}

void test_complete_request_local_mcp_max_rounds(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(
        "{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"call_1\","
        "\"function\":{\"name\":\"System.Info\",\"arguments\":\"{}\"}}]}}]}");
    for (int round = 0; round < CHAT_LOCAL_MCP_MAX_ROUNDS; round++) {
        oidc_rp_http_test_set_response(NULL, 200,
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
        oidc_rp_http_test_set_response(NULL, 200, "");
        oidc_rp_http_test_set_response(NULL, 200,
            "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"result\"}]}}");
    }
    struct ChatProxyResult *result = chat_local_mcp_complete_request(engine,
        "{\"model\":\"x\",\"messages\":[]}", "cid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_LOCAL_MCP_MAX_ROUNDS + 1, mock_auth_chat_deps_proxy_call_count());
    free(result->response_body);
    free(result->error_message);
    free(result);
    free_engine(engine);
}

void test_stream_next_body_happy_path(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"result\"}]}}");

    MultiStreamContext ctx = {0};
    ctx.engine = engine;
    ctx.request_body = strdup("{\"model\":\"x\",\"messages\":[]}");
    ctx.local_mcp_cid = strdup("stream_cid");
    ctx.tool_call_acc = NULL;
    ctx.local_mcp_round = 0;

    json_t *acc = json_array();
    json_t *delta = json_object();
    json_object_set_new(delta, "id", json_string("call_1"));
    json_object_set_new(delta, "name", json_string("System.Info"));
    json_object_set_new(delta, "arguments", json_object());
    json_array_append_new(acc, delta);
    ctx.tool_call_acc = acc;

    char *next_body = chat_local_mcp_stream_next_body(&ctx);
    TEST_ASSERT_NOT_NULL(next_body);
    json_t *parsed = json_loads(next_body, 0, NULL);
    TEST_ASSERT_NOT_NULL(parsed);
    json_decref(parsed);
    free(next_body);
    free((void *)ctx.request_body);
    free(ctx.local_mcp_cid);
    json_decref(ctx.tool_call_acc);
    chat_engine_config_destroy(engine);
}

void test_stream_next_body_disabled_mcp(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = false;
    MultiStreamContext ctx = {0};
    ctx.engine = engine;
    ctx.request_body = strdup("{}");
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(&ctx));
    free((void *)ctx.request_body);
    chat_engine_config_destroy(engine);
}

void test_stream_next_body_max_rounds(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    MultiStreamContext ctx = {0};
    ctx.engine = engine;
    ctx.request_body = strdup("{}");
    ctx.local_mcp_round = CHAT_LOCAL_MCP_MAX_ROUNDS;
    ctx.tool_call_acc = json_array();
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(&ctx));
    free((void *)ctx.request_body);
    json_decref(ctx.tool_call_acc);
    chat_engine_config_destroy(engine);
}

void test_stream_next_body_no_tool_calls(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    MultiStreamContext ctx = {0};
    ctx.engine = engine;
    ctx.request_body = strdup("{}");
    ctx.local_mcp_cid = strdup("cid");
    ctx.local_mcp_round = 0;
    ctx.tool_call_acc = NULL;
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(&ctx));
    free((void *)ctx.request_body);
    free(ctx.local_mcp_cid);
    chat_engine_config_destroy(engine);
}

void test_stream_next_body_no_cid(void) {
    const char *allowed[] = {"System.Info"};
    ChatEngineConfig *engine = make_engine_with_server(
        "https://mcp.example.com/mcp", "Bearer token", allowed, 1);
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response(NULL, 200, "");
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"result\"}]}}");

    MultiStreamContext ctx = {0};
    ctx.engine = engine;
    ctx.request_body = strdup("{\"model\":\"x\",\"messages\":[]}");
    ctx.local_mcp_cid = NULL;
    ctx.local_mcp_round = 0;
    ctx.tool_call_acc = json_array();
    json_t *delta = json_object();
    json_object_set_new(delta, "id", json_string("call_1"));
    json_object_set_new(delta, "name", json_string("System.Info"));
    json_object_set_new(delta, "arguments", json_object());
    json_array_append_new(ctx.tool_call_acc, delta);

    char *next_body = chat_local_mcp_stream_next_body(&ctx);
    TEST_ASSERT_NOT_NULL(next_body);
    free(next_body);
    free((void *)ctx.request_body);
    json_decref(ctx.tool_call_acc);
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_list_tools_null_engine);
    RUN_TEST(test_list_tools_disabled);
    RUN_TEST(test_list_tools_no_servers);
    RUN_TEST(test_list_tools_initialize_failure);
    RUN_TEST(test_list_tools_list_parse_failure);
    RUN_TEST(test_list_tools_happy_path);
    RUN_TEST(test_list_tools_with_allowed_filter);
    RUN_TEST(test_list_tools_empty_tools_returns_null);

    RUN_TEST(test_proxy_tool_calls_null_engine);
    RUN_TEST(test_proxy_tool_calls_null_tool_calls);
    RUN_TEST(test_proxy_tool_calls_server_not_found);
    RUN_TEST(test_proxy_tool_calls_initialize_fails);
    RUN_TEST(test_proxy_tool_calls_call_fails);
    RUN_TEST(test_proxy_tool_calls_happy_path);

    RUN_TEST(test_append_tool_results_null_args);
    RUN_TEST(test_append_tool_results_invalid_json);
    RUN_TEST(test_append_tool_results_openai);
    RUN_TEST(test_append_tool_results_anthropic);
    RUN_TEST(test_append_tool_results_responses);

    RUN_TEST(test_complete_request_round_trip);
    RUN_TEST(test_complete_request_local_mcp_max_rounds);
    RUN_TEST(test_complete_request_null_correlation_id);

    RUN_TEST(test_stream_next_body_happy_path);
    RUN_TEST(test_stream_next_body_disabled_mcp);
    RUN_TEST(test_stream_next_body_max_rounds);
    RUN_TEST(test_stream_next_body_no_tool_calls);
    RUN_TEST(test_stream_next_body_no_cid);

    return UNITY_END();
}
