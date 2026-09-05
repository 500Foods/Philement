#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/local_mcp.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include "mock_auth_chat_deps.h"

void test_normalize_call_valid_with_args(void);
void test_normalize_call_null_id(void);
void test_normalize_call_empty_name(void);
void test_normalize_call_null_name(void);
void test_normalize_call_null_arguments(void);
void test_find_server_null(void);
void test_find_server_not_found(void);
void test_find_server_found(void);
void test_tool_result_text_null_result(void);
void test_tool_result_text_no_content(void);
void test_tool_result_text_content_no_text(void);
void test_tool_result_text_no_content_array(void);
void test_tool_result_text_iserror_no_content(void);
void test_append_request_local_mcp_tools_null(void);
void test_append_request_local_mcp_tools_no_tools(void);
void test_append_request_local_mcp_tools_empty_array(void);
void test_append_request_local_mcp_openai(void);
void test_append_request_local_mcp_anthropic(void);
void test_append_request_local_mcp_responses(void);
void test_append_request_local_mcp_existing_tools_openai(void);
void test_extract_openai_no_choices(void);
void test_extract_openai_delta_message(void);
void test_extract_openai_non_string_args(void);
void test_extract_openai_missing_id(void);
void test_extract_anthropic_no_content(void);
void test_extract_anthropic_non_tool_use(void);
void test_extract_anthropic_missing_name(void);
void test_extract_responses_no_output(void);
void test_extract_responses_via_response_key(void);
void test_extract_responses_non_function_call(void);
void test_extract_responses_missing_call_id(void);
void test_extract_tool_calls_json_null(void);
void test_extract_tool_calls_null_body(void);
void test_accumulate_null_acc(void);
void test_accumulate_non_object_delta(void);
void test_accumulate_name_no_function(void);
void test_accumulate_function_empty_name(void);
void test_accumulate_function_non_string_args(void);
void test_accumulate_no_index_uses_position(void);
void test_accumulate_id_override(void);
void test_accumulate_non_string_id(void);
void test_finalize_null_acc(void);
void test_finalize_non_string_args(void);
void test_finalize_empty_args(void);
void test_finalize_no_args(void);
void test_finalize_empty_returns_null(void);
void test_finalize_null_name(void);
void test_finalize_null_id(void);
void test_complete_request_null_engine(void);
void test_complete_request_null_body(void);
void test_complete_request_disabled(void);
void test_complete_request_no_tool_calls(void);
void test_stream_next_body_null_ctx(void);
void test_stream_next_body_null_engine(void);
void test_stream_next_body_null_body(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
}

ChatEngineConfig *engine_for_utils(void);

ChatEngineConfig *engine_for_utils(void) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "test", CEC_PROVIDER_OPENAI, "model",
        "https://example.com/v1", "sk",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    engine->local_mcp.enabled = true;
    return engine;
}

void test_normalize_call_valid_with_args(void) {
    json_t *args = json_object();
    json_object_set_new(args, "key", json_string("val"));
    json_t *call = chat_local_mcp_normalize_call("call_1", "System.Info", args);
    TEST_ASSERT_NOT_NULL(call);
    TEST_ASSERT_EQUAL_STRING("call_1", json_string_value(json_object_get(call, "id")));
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_object_get(call, "name")));
    json_t *arg_obj = json_object_get(call, "arguments");
    TEST_ASSERT_NOT_NULL(arg_obj);
    TEST_ASSERT_EQUAL_STRING("val", json_string_value(json_object_get(arg_obj, "key")));
    json_decref(args);
    json_decref(call);
}

void test_normalize_call_null_id(void) {
    json_t *call = chat_local_mcp_normalize_call(NULL, "System.Info", NULL);
    TEST_ASSERT_NOT_NULL(call);
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(call, "id")));
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_object_get(call, "name")));
    TEST_ASSERT_TRUE(json_is_object(json_object_get(call, "arguments")));
    json_decref(call);
}

void test_normalize_call_empty_name(void) {
    TEST_ASSERT_NULL(chat_local_mcp_normalize_call("call_1", "", NULL));
}

void test_normalize_call_null_name(void) {
    TEST_ASSERT_NULL(chat_local_mcp_normalize_call("call_1", NULL, NULL));
}

void test_normalize_call_null_arguments(void) {
    json_t *call = chat_local_mcp_normalize_call("call_1", "System.Info", NULL);
    TEST_ASSERT_NOT_NULL(call);
    TEST_ASSERT_TRUE(json_is_object(json_object_get(call, "arguments")));
    json_decref(call);
}

void test_find_server_null(void) {
    TEST_ASSERT_NULL(chat_local_mcp_find_server(NULL, "foo"));
    TEST_ASSERT_NULL(chat_local_mcp_find_server(engine_for_utils(), NULL));
    ChatEngineConfig *engine = engine_for_utils();
    TEST_ASSERT_NULL(chat_local_mcp_find_server(engine, "NonExistent.Tool"));
    chat_engine_config_destroy(engine);
}

void test_find_server_not_found(void) {
    ChatEngineConfig *engine = engine_for_utils();
    json_t *collection = json_loads(
        "{\"local_mcp\":{\"enabled\":true,\"servers\":["
        "{\"url\":\"https://mcp.example.com/mcp\",\"allowed_tools\":[\"System.Info\"]}]}}", 0, NULL);
    chat_local_mcp_config_load(collection, &engine->local_mcp);
    json_decref(collection);
    TEST_ASSERT_NULL(chat_local_mcp_find_server(engine, "Unknown.Tool"));
    chat_local_mcp_config_cleanup(&engine->local_mcp);
    chat_engine_config_destroy(engine);
}

void test_find_server_found(void) {
    ChatEngineConfig *engine = engine_for_utils();
    json_t *collection = json_loads(
        "{\"local_mcp\":{\"enabled\":true,\"servers\":["
        "{\"url\":\"https://mcp.example.com/mcp\",\"allowed_tools\":[\"System.Info\"]}]}}", 0, NULL);
    chat_local_mcp_config_load(collection, &engine->local_mcp);
    json_decref(collection);
    const ChatLocalMcpServer *server = chat_local_mcp_find_server(engine, "System.Info");
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_EQUAL_STRING("https://mcp.example.com/mcp", server->url);
    chat_local_mcp_config_cleanup(&engine->local_mcp);
    chat_engine_config_destroy(engine);
}

void test_tool_result_text_null_result(void) {
    char *text = chat_local_mcp_tool_result_text(NULL);
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"empty tool result\"}", text);
    free(text);
}

void test_tool_result_text_no_content(void) {
    json_t *result = json_object();
    json_object_set_new(result, "isError", json_true());
    char *text = chat_local_mcp_tool_result_text(result);
    TEST_ASSERT_EQUAL_STRING("{\"isError\":true}", text);
    free(text);
    json_decref(result);
}

void test_tool_result_text_content_no_text(void) {
    json_t *result = json_object();
    json_t *content = json_array();
    json_t *block = json_object();
    json_object_set_new(block, "type", json_string("text"));
    json_object_set_new(block, "text", json_integer(42));
    json_array_append_new(content, block);
    json_object_set_new(result, "content", content);
    json_object_set_new(result, "isError", json_true());
    char *text = chat_local_mcp_tool_result_text(result);
    TEST_ASSERT_NOT_NULL(text);
    free(text);
    json_decref(result);
}

void test_tool_result_text_no_content_array(void) {
    json_t *result = json_object();
    json_object_set_new(result, "data", json_string("value"));
    char *text = chat_local_mcp_tool_result_text(result);
    TEST_ASSERT_NOT_NULL(text);
    free(text);
    json_decref(result);
}

void test_tool_result_text_iserror_no_content(void) {
    json_t *result = json_object();
    json_object_set_new(result, "isError", json_true());
    char *text = chat_local_mcp_tool_result_text(result);
    TEST_ASSERT_EQUAL_STRING("{\"isError\":true}", text);
    free(text);
    json_decref(result);
}

void test_append_request_local_mcp_tools_null(void) {
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string("x"));
    chat_request_append_local_mcp_tools(NULL, NULL, CEC_PROVIDER_OPENAI, false);
    chat_request_append_local_mcp_tools(root, NULL, CEC_PROVIDER_OPENAI, false);
    chat_request_append_local_mcp_tools(root, json_array(), CEC_PROVIDER_OPENAI, false);
    json_decref(root);
}

void test_append_request_local_mcp_tools_no_tools(void) {
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string("x"));
    json_t *tools = json_array();
    chat_request_append_local_mcp_tools(root, tools, CEC_PROVIDER_OPENAI, false);
    free(json_dumps(root, JSON_COMPACT));
    json_decref(root);
    json_decref(tools);
}

void test_append_request_local_mcp_tools_empty_array(void) {
    json_t *root = json_object();
    json_t *mcp_tools = json_array();
    chat_request_append_local_mcp_tools(root, mcp_tools, CEC_PROVIDER_OPENAI, false);
    TEST_ASSERT_NULL(json_object_get(root, "tools"));
    json_decref(root);
    json_decref(mcp_tools);
}

void test_append_request_local_mcp_openai(void) {
    json_t *root = json_object();
    json_t *mcp_tool = json_object();
    json_object_set_new(mcp_tool, "type", json_string("function"));
    json_object_set_new(mcp_tool, "function", json_object());
    json_object_set_new(mcp_tool, "name", json_string("Test.Tool"));
    json_object_set_new(mcp_tool, "description", json_string("A test tool"));
    json_object_set_new(mcp_tool, "inputSchema", json_object());
    json_t *mcp_tools = json_array();
    json_array_append_new(mcp_tools, mcp_tool);
    chat_request_append_local_mcp_tools(root, mcp_tools, CEC_PROVIDER_OPENAI, false);
    json_t *tools = json_object_get(root, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    json_decref(root);
    json_decref(mcp_tools);
}

void test_append_request_local_mcp_anthropic(void) {
    json_t *root = json_object();
    json_t *mcp_tool = json_object();
    json_object_set_new(mcp_tool, "type", json_string("function"));
    json_object_set_new(mcp_tool, "name", json_string("Test.Tool"));
    json_object_set_new(mcp_tool, "inputSchema", json_object());
    json_t *mcp_tools = json_array();
    json_array_append_new(mcp_tools, mcp_tool);
    chat_request_append_local_mcp_tools(root, mcp_tools, CEC_PROVIDER_ANTHROPIC, false);
    json_t *tools = json_object_get(root, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    json_decref(root);
    json_decref(mcp_tools);
}

void test_append_request_local_mcp_responses(void) {
    json_t *root = json_object();
    json_t *mcp_tool = json_object();
    json_object_set_new(mcp_tool, "type", json_string("function"));
    json_object_set_new(mcp_tool, "name", json_string("Test.Tool"));
    json_object_set_new(mcp_tool, "inputSchema", json_object());
    json_t *mcp_tools = json_array();
    json_array_append_new(mcp_tools, mcp_tool);
    chat_request_append_local_mcp_tools(root, mcp_tools, CEC_PROVIDER_OPENAI, true);
    json_t *tools = json_object_get(root, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    json_decref(root);
    json_decref(mcp_tools);
}

void test_append_request_local_mcp_existing_tools_openai(void) {
    json_t *root = json_object();
    json_t *existing = json_array();
    json_object_set_new(root, "tools", existing);
    json_t *mcp_tool = json_object();
    json_object_set_new(mcp_tool, "type", json_string("function"));
    json_object_set_new(mcp_tool, "name", json_string("Test.Tool"));
    json_object_set_new(mcp_tool, "inputSchema", json_object());
    json_t *mcp_tools = json_array();
    json_array_append_new(mcp_tools, mcp_tool);
    chat_request_append_local_mcp_tools(root, mcp_tools, CEC_PROVIDER_OPENAI, false);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(existing));
    json_decref(root);
    json_decref(mcp_tools);
}

void test_extract_openai_no_choices(void) {
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string("x"));
    json_t *calls = chat_local_mcp_extract_openai(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_openai_delta_message(void) {
    json_t *root = json_object();
    json_t *choices = json_array();
    json_t *choice = json_object();
    json_t *delta = json_object();
    json_object_set_new(delta, "tool_calls", json_array());
    json_object_set_new(choice, "delta", delta);
    json_array_append_new(choices, choice);
    json_object_set_new(root, "choices", choices);
    json_t *calls = chat_local_mcp_extract_openai(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_openai_non_string_args(void) {
    json_t *root = json_object();
    json_t *choices = json_array();
    json_t *choice = json_object();
    json_t *message = json_object();
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_string("call_1"));
    json_t *fn = json_object();
    json_object_set_new(fn, "name", json_string("System.Info"));
    json_object_set_new(fn, "arguments", json_integer(42));
    json_object_set_new(call, "function", fn);
    json_array_append_new(tool_calls, call);
    json_object_set_new(message, "tool_calls", tool_calls);
    json_object_set_new(choice, "message", message);
    json_array_append_new(choices, choice);
    json_object_set_new(root, "choices", choices);
    json_t *calls = chat_local_mcp_extract_openai(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(calls));
    json_t *normalized = json_array_get(calls, 0);
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(normalized, "name")));
    json_t *args = json_object_get(normalized, "arguments");
    TEST_ASSERT_TRUE(json_is_object(args) || json_is_integer(args));
    json_decref(calls);
    json_decref(root);
}

void test_extract_openai_missing_id(void) {
    json_t *root = json_object();
    json_t *choices = json_array();
    json_t *choice = json_object();
    json_t *message = json_object();
    json_t *tool_calls = json_array();
    json_t *call = json_object();
    json_object_set_new(call, "id", json_integer(123));
    json_t *fn = json_object();
    json_object_set_new(fn, "name", json_string("System.Info"));
    json_object_set_new(fn, "arguments", json_string("{}"));
    json_object_set_new(call, "function", fn);
    json_array_append_new(tool_calls, call);
    json_object_set_new(message, "tool_calls", tool_calls);
    json_object_set_new(choice, "message", message);
    json_array_append_new(choices, choice);
    json_object_set_new(root, "choices", choices);
    json_t *calls = chat_local_mcp_extract_openai(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(calls));
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(json_array_get(calls, 0), "id")));
    json_decref(calls);
    json_decref(root);
}

void test_extract_anthropic_no_content(void) {
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string("x"));
    json_t *calls = chat_local_mcp_extract_anthropic(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_anthropic_non_tool_use(void) {
    json_t *root = json_object();
    json_t *content = json_array();
    json_t *block = json_object();
    json_object_set_new(block, "type", json_string("text"));
    json_object_set_new(block, "text", json_string("hello"));
    json_array_append_new(content, block);
    json_object_set_new(root, "content", content);
    json_t *calls = chat_local_mcp_extract_anthropic(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_anthropic_missing_name(void) {
    json_t *root = json_object();
    json_t *content = json_array();
    json_t *block = json_object();
    json_object_set_new(block, "type", json_string("tool_use"));
    json_object_set_new(block, "id", json_string("tu_1"));
    json_array_append_new(content, block);
    json_object_set_new(root, "content", content);
    json_t *calls = chat_local_mcp_extract_anthropic(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_responses_no_output(void) {
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string("x"));
    json_t *calls = chat_local_mcp_extract_responses(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_responses_via_response_key(void) {
    json_t *root = json_object();
    json_t *response = json_object();
    json_t *output = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("function_call"));
    json_object_set_new(item, "call_id", json_string("fc_1"));
    json_object_set_new(item, "name", json_string("System.Info"));
    json_object_set_new(item, "arguments", json_string("{}"));
    json_array_append_new(output, item);
    json_object_set_new(response, "output", output);
    json_object_set_new(root, "response", response);
    json_t *calls = chat_local_mcp_extract_responses(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(calls));
    TEST_ASSERT_EQUAL_STRING("fc_1",
        json_string_value(json_object_get(json_array_get(calls, 0), "id")));
    json_decref(calls);
    json_decref(root);
}

void test_extract_responses_non_function_call(void) {
    json_t *root = json_object();
    json_t *output = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("text"));
    json_object_set_new(item, "text", json_string("hello"));
    json_array_append_new(output, item);
    json_object_set_new(root, "output", output);
    json_t *calls = chat_local_mcp_extract_responses(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
    json_decref(root);
}

void test_extract_responses_missing_call_id(void) {
    json_t *root = json_object();
    json_t *output = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("function_call"));
    json_object_set_new(item, "id", json_string("fc_2"));
    json_object_set_new(item, "name", json_string("System.Info"));
    json_object_set_new(item, "arguments", json_string("{}"));
    json_array_append_new(output, item);
    json_object_set_new(root, "output", output);
    json_t *calls = chat_local_mcp_extract_responses(root);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(calls));
    TEST_ASSERT_EQUAL_STRING("fc_2",
        json_string_value(json_object_get(json_array_get(calls, 0), "id")));
    json_decref(calls);
    json_decref(root);
}

void test_extract_tool_calls_json_null(void) {
    json_t *calls = chat_local_mcp_extract_tool_calls_json(NULL, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(calls));
    json_decref(calls);
}

void test_extract_tool_calls_null_body(void) {
    TEST_ASSERT_NULL(chat_local_mcp_extract_tool_calls(NULL, CEC_PROVIDER_OPENAI));
}

void test_accumulate_null_acc(void) {
    json_t *delta = json_array();
    chat_local_mcp_accumulate_stream_tool_calls(NULL, delta);
    json_decref(delta);
}

void test_accumulate_non_object_delta(void) {
    json_t *acc = NULL;
    json_t *delta = json_array();
    json_array_append_new(delta, json_string("not_an_object"));
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(acc));
    json_decref(acc);
    json_decref(delta);
}

void test_accumulate_name_no_function(void) {
    json_t *acc = NULL;
    json_t *delta = json_loads(
        "[{\"index\":0,\"name\":\"System.Info\",\"arguments\":\"\"}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(acc));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(acc, 0), "name")));
    json_decref(acc);
    json_decref(delta);
}

void test_accumulate_function_empty_name(void) {
    json_t *acc = NULL;
    json_t *delta = json_loads(
        "[{\"index\":0,\"function\":{\"name\":\"\",\"arguments\":\"\"}}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    json_t *existing = json_array_get(acc, 0);
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(existing, "name")));
    json_decref(acc);
    json_decref(delta);
}

void test_accumulate_function_non_string_args(void) {
    json_t *acc = NULL;
    json_t *delta = json_loads(
        "[{\"index\":0,\"function\":{\"name\":\"System.Info\",\"arguments\":42}}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    json_t *existing = json_array_get(acc, 0);
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(existing, "name")));
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(existing, "arguments")));
    json_decref(acc);
    json_decref(delta);
}

void test_accumulate_no_index_uses_position(void) {
    json_t *acc = NULL;
    json_t *delta = json_loads(
        "[{\"id\":\"call_1\",\"function\":{\"name\":\"System.Info\",\"arguments\":\"\"}}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(acc));
    TEST_ASSERT_EQUAL_STRING("call_1",
        json_string_value(json_object_get(json_array_get(acc, 0), "id")));
    json_decref(acc);
    json_decref(delta);
}

void test_accumulate_id_override(void) {
    json_t *acc = NULL;
    json_t *delta1 = json_loads("[{\"id\":\"a\",\"name\":\"\",\"arguments\":\"\"}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta1);
    json_decref(delta1);
    json_t *delta2 = json_loads("[{\"id\":\"b\",\"name\":\"\",\"arguments\":\"\"}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta2);
    json_decref(delta2);
    TEST_ASSERT_EQUAL_STRING("b",
        json_string_value(json_object_get(json_array_get(acc, 0), "id")));
    json_decref(acc);
}

void test_accumulate_non_string_id(void) {
    json_t *acc = NULL;
    json_t *delta = json_loads("[{\"id\":123,\"name\":\"\",\"arguments\":\"\"}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta);
    TEST_ASSERT_NOT_NULL(acc);
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(json_array_get(acc, 0), "id")));
    json_decref(acc);
    json_decref(delta);
}

void test_finalize_null_acc(void) {
    TEST_ASSERT_NULL(chat_local_mcp_finalize_accumulated(NULL));
}

void test_finalize_non_string_args(void) {
    json_t *acc = json_loads(
        "[{\"id\":\"c1\",\"name\":\"System.Info\",\"arguments\":42}]", 0, NULL);
    json_t *out = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(out));
    json_decref(acc);
    json_decref(out);
}

void test_finalize_empty_args(void) {
    json_t *acc = json_loads(
        "[{\"id\":\"c1\",\"name\":\"System.Info\",\"arguments\":\"{}\"}]", 0, NULL);
    json_t *out = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(out));
    json_decref(acc);
    json_decref(out);
}

void test_finalize_no_args(void) {
    json_t *acc = json_loads(
        "[{\"id\":\"c1\",\"name\":\"System.Info\"}]", 0, NULL);
    json_t *out = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(out));
    json_decref(acc);
    json_decref(out);
}

void test_finalize_empty_returns_null(void) {
    json_t *acc = json_array();
    TEST_ASSERT_NULL(chat_local_mcp_finalize_accumulated(acc));
    json_decref(acc);
}

void test_finalize_null_name(void) {
    json_t *acc = json_loads("[{\"id\":\"c1\",\"name\":null,\"arguments\":{}}]", 0, NULL);
    json_t *out = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NULL(out);
    json_decref(acc);
}

void test_finalize_null_id(void) {
    json_t *acc = json_loads("[{\"id\":null,\"name\":\"System.Info\",\"arguments\":{}}]", 0, NULL);
    json_t *out = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(out));
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(json_array_get(out, 0), "id")));
    json_decref(acc);
    json_decref(out);
}

void test_complete_request_null_engine(void) {
    TEST_ASSERT_NULL(chat_local_mcp_complete_request(NULL, "{}", "-"));
}

void test_complete_request_null_body(void) {
    ChatEngineConfig *engine = engine_for_utils();
    engine->local_mcp.enabled = false;
    TEST_ASSERT_NULL(chat_local_mcp_complete_request(engine, NULL, "-"));
    chat_engine_config_destroy(engine);
}

void test_complete_request_disabled(void) {
    ChatEngineConfig *engine = engine_for_utils();
    engine->local_mcp.enabled = false;
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"choices\":[{\"message\":{\"content\":\"hello\"}}]}");
    struct ChatProxyResult *result = chat_local_mcp_complete_request(engine,
        "{\"model\":\"x\"}", "cid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->response_body);
    free(result->response_body);
    free(result->error_message);
    free(result);
    chat_engine_config_destroy(engine);
}

void test_complete_request_no_tool_calls(void) {
    ChatEngineConfig *engine = engine_for_utils();
    mock_auth_chat_deps_reset_all();
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"choices\":[{\"message\":{\"content\":\"hello\"}}]}");
    struct ChatProxyResult *result = chat_local_mcp_complete_request(engine,
        "{\"model\":\"x\"}", "cid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->response_body);
    free(result->response_body);
    free(result->error_message);
    free(result);
    chat_engine_config_destroy(engine);
}

void test_stream_next_body_null_ctx(void) {
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(NULL));
}

void test_stream_next_body_null_engine(void) {
    MultiStreamContext ctx = {0};
    ctx.engine = NULL;
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(&ctx));
}

void test_stream_next_body_null_body(void) {
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ChatEngineConfig *engine = engine_for_utils();
    ctx.engine = engine;
    ctx.request_body = NULL;
    TEST_ASSERT_NULL(chat_local_mcp_stream_next_body(&ctx));
    chat_engine_config_destroy(engine);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_normalize_call_valid_with_args);
    RUN_TEST(test_normalize_call_null_id);
    RUN_TEST(test_normalize_call_empty_name);
    RUN_TEST(test_normalize_call_null_name);
    RUN_TEST(test_normalize_call_null_arguments);
    RUN_TEST(test_find_server_null);
    RUN_TEST(test_find_server_not_found);
    RUN_TEST(test_find_server_found);
    RUN_TEST(test_tool_result_text_null_result);
    RUN_TEST(test_tool_result_text_no_content);
    RUN_TEST(test_tool_result_text_content_no_text);
    RUN_TEST(test_tool_result_text_no_content_array);
    RUN_TEST(test_tool_result_text_iserror_no_content);
    RUN_TEST(test_append_request_local_mcp_tools_null);
    RUN_TEST(test_append_request_local_mcp_tools_no_tools);
    RUN_TEST(test_append_request_local_mcp_tools_empty_array);
    RUN_TEST(test_append_request_local_mcp_openai);
    RUN_TEST(test_append_request_local_mcp_anthropic);
    RUN_TEST(test_append_request_local_mcp_responses);
    RUN_TEST(test_append_request_local_mcp_existing_tools_openai);
    RUN_TEST(test_extract_openai_no_choices);
    RUN_TEST(test_extract_openai_delta_message);
    RUN_TEST(test_extract_openai_non_string_args);
    RUN_TEST(test_extract_openai_missing_id);
    RUN_TEST(test_extract_anthropic_no_content);
    RUN_TEST(test_extract_anthropic_non_tool_use);
    RUN_TEST(test_extract_anthropic_missing_name);
    RUN_TEST(test_extract_responses_no_output);
    RUN_TEST(test_extract_responses_via_response_key);
    RUN_TEST(test_extract_responses_non_function_call);
    RUN_TEST(test_extract_responses_missing_call_id);
    RUN_TEST(test_extract_tool_calls_json_null);
    RUN_TEST(test_extract_tool_calls_null_body);
    RUN_TEST(test_accumulate_null_acc);
    RUN_TEST(test_accumulate_non_object_delta);
    RUN_TEST(test_accumulate_name_no_function);
    RUN_TEST(test_accumulate_function_empty_name);
    RUN_TEST(test_accumulate_function_non_string_args);
    RUN_TEST(test_accumulate_no_index_uses_position);
    RUN_TEST(test_accumulate_id_override);
    RUN_TEST(test_accumulate_non_string_id);
    RUN_TEST(test_finalize_null_acc);
    RUN_TEST(test_finalize_non_string_args);
    RUN_TEST(test_finalize_empty_args);
    RUN_TEST(test_finalize_no_args);
    RUN_TEST(test_finalize_empty_returns_null);
    RUN_TEST(test_finalize_null_name);
    RUN_TEST(test_finalize_null_id);
    RUN_TEST(test_complete_request_null_engine);
    RUN_TEST(test_complete_request_null_body);
    RUN_TEST(test_complete_request_disabled);
    RUN_TEST(test_complete_request_no_tool_calls);
    RUN_TEST(test_stream_next_body_null_ctx);
    RUN_TEST(test_stream_next_body_null_engine);
    RUN_TEST(test_stream_next_body_null_body);
    return UNITY_END();
}
