#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_rpc.h>

void test_mcp_rpc_parse_error_invalid_json(void);
void test_mcp_rpc_parse_error_empty(void);
void test_mcp_rpc_parse_invalid_request_bad_version(void);
void test_mcp_rpc_parse_invalid_request_empty_method(void);
void test_mcp_rpc_parse_invalid_request_object_id(void);
void test_mcp_rpc_parse_batch_rejected(void);
void test_mcp_rpc_parse_oversize(void);
void test_mcp_rpc_parse_request_ok(void);
void test_mcp_rpc_parse_notify(void);
void test_mcp_rpc_parse_protocol_version_header(void);
void test_mcp_rpc_parse_protocol_version_default(void);
void test_mcp_rpc_is_initialize(void);
void test_mcp_rpc_make_error_shape(void);
void test_mcp_rpc_status_helpers(void);
void test_mcp_rpc_parse_null_out(void);
void test_mcp_rpc_parse_non_object(void);
void test_mcp_rpc_envelope_cleanup_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mcp_rpc_parse_error_invalid_json(void) {
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_PARSE, mcp_rpc_parse("{", 1, 1024, NULL, &env));
}

void test_mcp_rpc_parse_error_empty(void) {
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_PARSE, mcp_rpc_parse("", 0, 1024, NULL, &env));
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_PARSE, mcp_rpc_parse(NULL, 0, 1024, NULL, &env));
}

void test_mcp_rpc_parse_invalid_request_bad_version(void) {
    const char *body = "{\"jsonrpc\":\"1.0\",\"id\":1,\"method\":\"ping\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_INVALID, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
}

void test_mcp_rpc_parse_invalid_request_empty_method(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_INVALID, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
}

void test_mcp_rpc_parse_invalid_request_object_id(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":{},\"method\":\"ping\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_INVALID, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
}

void test_mcp_rpc_parse_batch_rejected(void) {
    const char *body = "[{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}]";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_INVALID, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
    TEST_ASSERT_EQUAL(MCP_RPC_INVALID_REQUEST, mcp_rpc_status_code(MCP_RPC_ERR_INVALID));
}

void test_mcp_rpc_parse_oversize(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_OVERSIZE, mcp_rpc_parse(body, strlen(body), 4, NULL, &env));
    TEST_ASSERT_EQUAL(MCP_RPC_PARSE_ERROR, mcp_rpc_status_code(MCP_RPC_ERR_OVERSIZE));
}

void test_mcp_rpc_parse_request_ok(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/list\",\"params\":{}}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
    TEST_ASSERT_FALSE(env.is_notification);
    TEST_ASSERT_EQUAL_STRING("tools/list", env.method);
    TEST_ASSERT_TRUE(json_is_integer(env.id));
    TEST_ASSERT_EQUAL(7, json_integer_value(env.id));
    TEST_ASSERT_TRUE(json_is_object(env.params));
    mcp_rpc_envelope_cleanup(&env);
}

void test_mcp_rpc_parse_notify(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
    TEST_ASSERT_TRUE(env.is_notification);
    TEST_ASSERT_NULL(env.id);
    mcp_rpc_envelope_cleanup(&env);
}

void test_mcp_rpc_parse_protocol_version_header(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":\"a\",\"method\":\"ping\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 1024, "2025-06-18", &env));
    TEST_ASSERT_EQUAL_STRING("2025-06-18", env.protocol_version);
    TEST_ASSERT_TRUE(json_is_string(env.id));
    mcp_rpc_envelope_cleanup(&env);
}

void test_mcp_rpc_parse_protocol_version_default(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":null,\"method\":\"ping\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
    TEST_ASSERT_EQUAL_STRING(MCP_DEFAULT_PROTOCOL_VERSION, env.protocol_version);
    TEST_ASSERT_FALSE(env.is_notification);
    mcp_rpc_envelope_cleanup(&env);
}

void test_mcp_rpc_is_initialize(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
    TEST_ASSERT_TRUE(mcp_rpc_is_initialize(&env));
    mcp_rpc_envelope_cleanup(&env);
    TEST_ASSERT_FALSE(mcp_rpc_is_initialize(NULL));
}

void test_mcp_rpc_make_error_shape(void) {
    char *body = mcp_rpc_make_error(NULL, MCP_RPC_PARSE_ERROR, "Parse error");
    json_t *root;
    json_t *err;
    TEST_ASSERT_NOT_NULL(body);
    root = json_loads(body, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("2.0", json_string_value(json_object_get(root, "jsonrpc")));
    TEST_ASSERT_TRUE(json_is_null(json_object_get(root, "id")));
    err = json_object_get(root, "error");
    TEST_ASSERT_EQUAL(MCP_RPC_PARSE_ERROR, json_integer_value(json_object_get(err, "code")));
    json_decref(root);
    free(body);
}

void test_mcp_rpc_status_helpers(void) {
    TEST_ASSERT_EQUAL_STRING("OK", mcp_rpc_status_message(MCP_RPC_OK));
    TEST_ASSERT_EQUAL_STRING("Parse error", mcp_rpc_status_message(MCP_RPC_ERR_PARSE));
    TEST_ASSERT_EQUAL_STRING("Parse error", mcp_rpc_status_message(MCP_RPC_ERR_OVERSIZE));
    TEST_ASSERT_EQUAL_STRING("Invalid Request", mcp_rpc_status_message(MCP_RPC_ERR_INVALID));
    TEST_ASSERT_EQUAL_STRING("Invalid Request", mcp_rpc_status_message((McpRpcStatus)99));
    TEST_ASSERT_EQUAL(0, mcp_rpc_status_code(MCP_RPC_OK));
    TEST_ASSERT_EQUAL(MCP_RPC_PARSE_ERROR, mcp_rpc_status_code(MCP_RPC_ERR_PARSE));
    TEST_ASSERT_EQUAL(MCP_RPC_PARSE_ERROR, mcp_rpc_status_code(MCP_RPC_ERR_OVERSIZE));
    TEST_ASSERT_EQUAL(MCP_RPC_INVALID_REQUEST, mcp_rpc_status_code(MCP_RPC_ERR_INVALID));
    TEST_ASSERT_EQUAL(MCP_RPC_INVALID_REQUEST, mcp_rpc_status_code((McpRpcStatus)99));
}

void test_mcp_rpc_parse_null_out(void) {
    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_INVALID, mcp_rpc_parse(body, strlen(body), 1024, NULL, NULL));
}

void test_mcp_rpc_parse_non_object(void) {
    const char *body = "\"ping\"";
    McpRpcEnvelope env;
    TEST_ASSERT_EQUAL(MCP_RPC_ERR_PARSE, mcp_rpc_parse(body, strlen(body), 1024, NULL, &env));
}

void test_mcp_rpc_envelope_cleanup_null(void) {
    mcp_rpc_envelope_cleanup(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_rpc_parse_error_invalid_json);
    RUN_TEST(test_mcp_rpc_parse_error_empty);
    RUN_TEST(test_mcp_rpc_parse_invalid_request_bad_version);
    RUN_TEST(test_mcp_rpc_parse_invalid_request_empty_method);
    RUN_TEST(test_mcp_rpc_parse_invalid_request_object_id);
    RUN_TEST(test_mcp_rpc_parse_batch_rejected);
    RUN_TEST(test_mcp_rpc_parse_oversize);
    RUN_TEST(test_mcp_rpc_parse_request_ok);
    RUN_TEST(test_mcp_rpc_parse_notify);
    RUN_TEST(test_mcp_rpc_parse_protocol_version_header);
    RUN_TEST(test_mcp_rpc_parse_protocol_version_default);
    RUN_TEST(test_mcp_rpc_is_initialize);
    RUN_TEST(test_mcp_rpc_make_error_shape);
    RUN_TEST(test_mcp_rpc_status_helpers);
    RUN_TEST(test_mcp_rpc_parse_null_out);
    RUN_TEST(test_mcp_rpc_parse_non_object);
    RUN_TEST(test_mcp_rpc_envelope_cleanup_null);
    return UNITY_END();
}
