#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

void test_rpc_request_null_method(void);
void test_rpc_request_empty_method(void);
void test_rpc_parse_result_null_body(void);
void test_rpc_parse_result_invalid_json(void);
void test_rpc_parse_result_missing_result(void);
void test_rpc_parse_result_error_non_string_message(void);
void test_rpc_parse_result_error_is_null(void);
void test_rpc_parse_result_out_result_null(void);
void test_rpc_parse_result_out_error_null(void);
void test_http_post_null_url(void);
void test_http_post_empty_url(void);
void test_http_post_null_body(void);
void test_http_post_no_auth(void);
void test_http_post_auth_without_bearer(void);
void test_http_post_with_session_id(void);
void test_http_post_no_session_out(void);
void test_http_post_http_error(void);
void test_initialize_null_url(void);
void test_initialize_parse_failure(void);
void test_initialize_error_response(void);
void test_initialize_no_session_out(void);
void test_tools_list_parse_failure(void);
void test_tools_list_no_tools_array(void);
void test_tools_list_error_response(void);
void test_tools_call_null_name(void);
void test_tools_call_empty_name(void);
void test_tools_call_parse_failure(void);
void test_tool_to_openai_null(void);
void test_tool_to_openai_non_object(void);
void test_tool_to_openai_missing_name(void);
void test_tool_to_responses_null(void);
void test_tool_to_responses_non_object(void);
void test_tool_to_responses_missing_name(void);
void test_tool_to_anthropic_null(void);
void test_tool_to_anthropic_non_object(void);
void test_tool_to_anthropic_missing_name(void);
void test_tools_filter_null_tools(void);
void test_tools_filter_non_array(void);
void test_tools_filter_non_object_tool(void);
void test_tools_filter_missing_name(void);
void test_tools_filter_null_allowed(void);
void test_tools_filter_no_match(void);
void test_tools_map_null_tools(void);
void test_tools_map_non_array(void);
void test_tools_map_null_convert(void);
void test_tools_map_empty(void);
void test_tools_map_skip_null(void);
void test_mcp_schema_input_schema(void);
void test_mcp_schema_input_schema_fallback(void);
void test_mcp_schema_null(void);
void test_mcp_schema_no_schema(void);
void test_fetch_tools_null_url(void);
void test_fetch_tools_initialize_failure(void);
void test_next_id_sequence(void);
void test_next_id_overflow(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
}

void test_rpc_request_null_method(void) {
    TEST_ASSERT_NULL(mcp_client_rpc_request(1, NULL, NULL));
}

void test_rpc_request_empty_method(void) {
    TEST_ASSERT_NULL(mcp_client_rpc_request(1, "", NULL));
}

void test_rpc_parse_result_null_body(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result(NULL, &result, &error));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("empty mcp response", error);
    free(error);
}

void test_rpc_parse_result_invalid_json(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result("not json", &result, &error));
    TEST_ASSERT_EQUAL_STRING("invalid mcp json", error);
    free(error);
}

void test_rpc_parse_result_missing_result(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1}", &result, &error));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("missing result", error);
    free(error);
}

void test_rpc_parse_result_error_non_string_message(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":{\"code\":-1,\"message\":123}}",
        &result, &error));
    TEST_ASSERT_EQUAL_STRING("rpc error", error);
    free(error);
}

void test_rpc_parse_result_error_is_null(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":null}", &result, &error));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("missing result", error);
    free(error);
    json_decref(result);
}

void test_rpc_parse_result_out_result_null(void) {
    TEST_ASSERT_TRUE(mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}", NULL, NULL));
}

void test_rpc_parse_result_out_error_null(void) {
    json_t *result = NULL;
    TEST_ASSERT_TRUE(mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}", &result, NULL));
    json_decref(result);
}

void test_http_post_null_url(void) {
    char *session = NULL;
    TEST_ASSERT_NULL(mcp_client_http_post(NULL, NULL, NULL, "{}", &session));
    TEST_ASSERT_NULL(session);
}

void test_http_post_empty_url(void) {
    char *session = NULL;
    TEST_ASSERT_NULL(mcp_client_http_post("", NULL, NULL, "{}", &session));
    TEST_ASSERT_NULL(session);
}

void test_http_post_null_body(void) {
    char *session = NULL;
    TEST_ASSERT_NULL(mcp_client_http_post("https://example.com/mcp", NULL, NULL, NULL, &session));
    TEST_ASSERT_NULL(session);
}

void test_http_post_no_auth(void) {
    oidc_rp_http_test_set_response("example.com", 200, "{\"ok\":true}");
    char *session = NULL;
    char *result = mcp_client_http_post("https://example.com/mcp", NULL, NULL, "{}", &session);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
    free(session);
}

void test_http_post_auth_without_bearer(void) {
    oidc_rp_http_test_set_response("example.com", 200, "{\"ok\":true}");
    char *session = NULL;
    char *result = mcp_client_http_post("https://example.com/mcp", "mytoken", NULL, "{}", &session);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
    free(session);
}

void test_http_post_with_session_id(void) {
    oidc_rp_http_test_set_response("example.com", 200, "{\"ok\":true}");
    char *session = NULL;
    char *result = mcp_client_http_post("https://example.com/mcp", NULL, "sid123", "{}", &session);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NULL(session);
    free(result);
}

void test_http_post_no_session_out(void) {
    oidc_rp_http_test_set_response("example.com", 200, "{\"ok\":true}");
    char *result = mcp_client_http_post("https://example.com/mcp", NULL, NULL, "{}", NULL);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
}

void test_http_post_http_error(void) {
    oidc_rp_http_test_set_response("example.com", 500, "server error");
    char *result = mcp_client_http_post("https://example.com/mcp", NULL, NULL, "{}", NULL);
    TEST_ASSERT_NOT_NULL(result);
    free(result);
}

void test_initialize_null_url(void) {
    char *session = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_initialize(NULL, NULL, &session, &error));
    TEST_ASSERT_NULL(session);
}

void test_initialize_parse_failure(void) {
    oidc_rp_http_test_set_response("example.com", 200, "not json");
    char *session = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_initialize("https://example.com/mcp", NULL, &session, &error));
    TEST_ASSERT_NOT_NULL(error);
    free(error);
    free(session);
}

void test_initialize_error_response(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":{\"code\":-1,\"message\":\"init failed\"}}");
    char *session = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_initialize("https://example.com/mcp", NULL, &session, &error));
    TEST_ASSERT_EQUAL_STRING("init failed", error);
    free(error);
    free(session);
}

void test_initialize_no_session_out(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response("example.com", 202, "");
    TEST_ASSERT_TRUE(mcp_client_initialize("https://example.com/mcp", NULL, NULL, NULL));
}

void test_tools_list_parse_failure(void) {
    oidc_rp_http_test_set_response("example.com", 200, "not json");
    json_t *tools = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_tools_list("https://example.com/mcp", NULL, NULL, &tools, &error));
    TEST_ASSERT_NOT_NULL(error);
    free(error);
    json_decref(tools);
}

void test_tools_list_no_tools_array(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}");
    json_t *tools = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_tools_list("https://example.com/mcp", NULL, NULL, &tools, &error));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("tools/list missing tools array", error);
    free(error);
    json_decref(tools);
}

void test_tools_list_error_response(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":{\"code\":-1,\"message\":\"list failed\"}}");
    json_t *tools = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_tools_list("https://example.com/mcp", NULL, NULL, &tools, &error));
    TEST_ASSERT_NOT_NULL(error);
    free(error);
    json_decref(tools);
}

void test_tools_call_null_name(void) {
    char *error = NULL;
    json_t *result = NULL;
    TEST_ASSERT_FALSE(mcp_client_tools_call("https://example.com/mcp", NULL, NULL, NULL, NULL, &result, &error));
    TEST_ASSERT_EQUAL_STRING("tool name required", error);
    free(error);
}

void test_tools_call_empty_name(void) {
    char *error = NULL;
    json_t *result = NULL;
    TEST_ASSERT_FALSE(mcp_client_tools_call("https://example.com/mcp", NULL, NULL, "", NULL, &result, &error));
    TEST_ASSERT_EQUAL_STRING("tool name required", error);
    free(error);
}

void test_tools_call_parse_failure(void) {
    oidc_rp_http_test_set_response("example.com", 200, "not json");
    char *error = NULL;
    json_t *result = NULL;
    json_t *args = json_object();
    TEST_ASSERT_FALSE(mcp_client_tools_call("https://example.com/mcp", NULL, NULL, "System.Info", args, &result, &error));
    TEST_ASSERT_NOT_NULL(error);
    free(error);
    json_decref(args);
    json_decref(result);
}

void test_tool_to_openai_null(void) {
    TEST_ASSERT_NULL(mcp_client_tool_to_openai(NULL));
}

void test_tool_to_openai_non_object(void) {
    json_t *arr = json_array();
    TEST_ASSERT_NULL(mcp_client_tool_to_openai(arr));
    json_decref(arr);
}

void test_tool_to_openai_missing_name(void) {
    json_t *tool = json_object();
    json_object_set_new(tool, "description", json_string("desc"));
    TEST_ASSERT_NULL(mcp_client_tool_to_openai(tool));
    json_decref(tool);
}

void test_tool_to_responses_null(void) {
    TEST_ASSERT_NULL(mcp_client_tool_to_responses(NULL));
}

void test_tool_to_responses_non_object(void) {
    json_t *arr = json_array();
    TEST_ASSERT_NULL(mcp_client_tool_to_responses(arr));
    json_decref(arr);
}

void test_tool_to_responses_missing_name(void) {
    json_t *tool = json_object();
    json_object_set_new(tool, "description", json_string("desc"));
    TEST_ASSERT_NULL(mcp_client_tool_to_responses(tool));
    json_decref(tool);
}

void test_tool_to_anthropic_null(void) {
    TEST_ASSERT_NULL(mcp_client_tool_to_anthropic(NULL));
}

void test_tool_to_anthropic_non_object(void) {
    json_t *arr = json_array();
    TEST_ASSERT_NULL(mcp_client_tool_to_anthropic(arr));
    json_decref(arr);
}

void test_tool_to_anthropic_missing_name(void) {
    json_t *tool = json_object();
    json_object_set_new(tool, "description", json_string("desc"));
    TEST_ASSERT_NULL(mcp_client_tool_to_anthropic(tool));
    json_decref(tool);
}

void test_tools_filter_null_tools(void) {
    json_t *filtered = mcp_client_tools_filter(NULL, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
}

void test_tools_filter_non_array(void) {
    json_t *obj = json_object();
    json_t *filtered = mcp_client_tools_filter(obj, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
    json_decref(obj);
}

void test_tools_filter_non_object_tool(void) {
    json_t *tools = json_array();
    json_array_append_new(tools, json_string("not_an_object"));
    json_t *filtered = mcp_client_tools_filter(tools, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
    json_decref(tools);
}

void test_tools_filter_missing_name(void) {
    json_t *tools = json_array();
    json_t *tool = json_object();
    json_object_set_new(tool, "description", json_string("desc"));
    json_array_append_new(tools, tool);
    json_t *filtered = mcp_client_tools_filter(tools, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
    json_decref(tools);
}

void test_tools_filter_null_allowed(void) {
    json_t *tools = json_array();
    json_t *tool = json_object();
    json_object_set_new(tool, "name", json_string("System.Info"));
    json_array_append_new(tools, tool);
    json_t *filtered = mcp_client_tools_filter(tools, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
    json_decref(tools);
}

void test_tools_filter_no_match(void) {
    json_t *tools = json_array();
    json_t *tool = json_object();
    json_object_set_new(tool, "name", json_string("Secret.Tool"));
    json_array_append_new(tools, tool);
    char *allowed[] = { (char *)"System.Info" };
    json_t *filtered = mcp_client_tools_filter(tools, allowed, 1);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(filtered));
    json_decref(filtered);
    json_decref(tools);
}

void test_tools_map_null_tools(void) {
    json_t *out = mcp_client_tools_map(NULL, mcp_client_tool_to_openai);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(out));
    json_decref(out);
}

void test_tools_map_non_array(void) {
    json_t *obj = json_object();
    json_t *out = mcp_client_tools_map(obj, mcp_client_tool_to_openai);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(out));
    json_decref(out);
    json_decref(obj);
}

void test_tools_map_null_convert(void) {
    json_t *tools = json_array();
    json_array_append_new(tools, json_object());
    json_t *out = mcp_client_tools_map(tools, NULL);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(out));
    json_decref(out);
    json_decref(tools);
}

void test_tools_map_empty(void) {
    json_t *tools = json_array();
    json_t *out = mcp_client_tools_map(tools, mcp_client_tool_to_openai);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(out));
    json_decref(out);
    json_decref(tools);
}

void test_tools_map_skip_null(void) {
    json_t *tools = json_array();
    json_t *bad = json_object();
    json_array_append_new(tools, bad);
    json_t *out = mcp_client_tools_map(tools, mcp_client_tool_to_openai);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(out));
    json_decref(out);
    json_decref(tools);
}

void test_mcp_schema_input_schema(void) {
    json_t *tool = json_object();
    json_t *schema = json_object();
    json_object_set_new(schema, "type", json_string("object"));
    json_object_set_new(tool, "inputSchema", schema);
    TEST_ASSERT_EQUAL_PTR(schema, mcp_client_mcp_schema(tool));
    json_decref(tool);
}

void test_mcp_schema_input_schema_fallback(void) {
    json_t *tool = json_object();
    json_t *schema = json_object();
    json_object_set_new(schema, "type", json_string("object"));
    json_object_set_new(tool, "input_schema", schema);
    TEST_ASSERT_EQUAL_PTR(schema, mcp_client_mcp_schema(tool));
    json_decref(tool);
}

void test_mcp_schema_null(void) {
    TEST_ASSERT_NULL(mcp_client_mcp_schema(NULL));
}

void test_mcp_schema_no_schema(void) {
    json_t *tool = json_object();
    TEST_ASSERT_NULL(mcp_client_mcp_schema(tool));
    json_decref(tool);
}

void test_fetch_tools_null_url(void) {
    TEST_ASSERT_NULL(mcp_client_fetch_tools(NULL, NULL, NULL, 0, "cid"));
}

void test_fetch_tools_initialize_failure(void) {
    oidc_rp_http_test_set_response("example.com", 500, "server error");
    TEST_ASSERT_NULL(mcp_client_fetch_tools("https://example.com/mcp", NULL, NULL, 0, "cid"));
}

void test_next_id_sequence(void) {
    int id1 = mcp_client_next_id();
    int id2 = mcp_client_next_id();
    TEST_ASSERT_EQUAL(id1 + 1, id2);
}

void test_next_id_overflow(void) {
    int id_last = mcp_client_next_id();
    (void)id_last;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rpc_request_null_method);
    RUN_TEST(test_rpc_request_empty_method);
    RUN_TEST(test_rpc_parse_result_null_body);
    RUN_TEST(test_rpc_parse_result_invalid_json);
    RUN_TEST(test_rpc_parse_result_missing_result);
    RUN_TEST(test_rpc_parse_result_error_non_string_message);
    RUN_TEST(test_rpc_parse_result_error_is_null);
    RUN_TEST(test_rpc_parse_result_out_result_null);
    RUN_TEST(test_rpc_parse_result_out_error_null);
    RUN_TEST(test_http_post_null_url);
    RUN_TEST(test_http_post_empty_url);
    RUN_TEST(test_http_post_null_body);
    RUN_TEST(test_http_post_no_auth);
    RUN_TEST(test_http_post_auth_without_bearer);
    RUN_TEST(test_http_post_with_session_id);
    RUN_TEST(test_http_post_no_session_out);
    RUN_TEST(test_http_post_http_error);
    RUN_TEST(test_initialize_null_url);
    RUN_TEST(test_initialize_parse_failure);
    RUN_TEST(test_initialize_error_response);
    RUN_TEST(test_initialize_no_session_out);
    RUN_TEST(test_tools_list_parse_failure);
    RUN_TEST(test_tools_list_no_tools_array);
    RUN_TEST(test_tools_list_error_response);
    RUN_TEST(test_tools_call_null_name);
    RUN_TEST(test_tools_call_empty_name);
    RUN_TEST(test_tools_call_parse_failure);
    RUN_TEST(test_tool_to_openai_null);
    RUN_TEST(test_tool_to_openai_non_object);
    RUN_TEST(test_tool_to_openai_missing_name);
    RUN_TEST(test_tool_to_responses_null);
    RUN_TEST(test_tool_to_responses_non_object);
    RUN_TEST(test_tool_to_responses_missing_name);
    RUN_TEST(test_tool_to_anthropic_null);
    RUN_TEST(test_tool_to_anthropic_non_object);
    RUN_TEST(test_tool_to_anthropic_missing_name);
    RUN_TEST(test_tools_filter_null_tools);
    RUN_TEST(test_tools_filter_non_array);
    RUN_TEST(test_tools_filter_non_object_tool);
    RUN_TEST(test_tools_filter_missing_name);
    RUN_TEST(test_tools_filter_null_allowed);
    RUN_TEST(test_tools_filter_no_match);
    RUN_TEST(test_tools_map_null_tools);
    RUN_TEST(test_tools_map_non_array);
    RUN_TEST(test_tools_map_null_convert);
    RUN_TEST(test_tools_map_empty);
    RUN_TEST(test_tools_map_skip_null);
    RUN_TEST(test_mcp_schema_input_schema);
    RUN_TEST(test_mcp_schema_input_schema_fallback);
    RUN_TEST(test_mcp_schema_null);
    RUN_TEST(test_mcp_schema_no_schema);
    RUN_TEST(test_fetch_tools_null_url);
    RUN_TEST(test_fetch_tools_initialize_failure);
    RUN_TEST(test_next_id_sequence);
    RUN_TEST(test_next_id_overflow);
    return UNITY_END();
}
