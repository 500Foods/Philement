#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

void test_initialize_ok(void);
void test_tools_list_ok(void);
void test_tools_call_ok(void);
void test_fetch_tools_allowlist(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
}

void test_initialize_ok(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response("example.com", 202, "");
    char *session = NULL;
    char *error = NULL;
    TEST_ASSERT_TRUE(mcp_client_initialize("https://example.com/mcp", NULL, &session, &error));
    TEST_ASSERT_NULL(error);
    free(session);
    free(error);
}

void test_tools_list_ok(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"tools\":[{\"name\":\"System.Info\",\"inputSchema\":{\"type\":\"object\"}}]}}");
    json_t *tools = NULL;
    char *error = NULL;
    TEST_ASSERT_TRUE(mcp_client_tools_list("https://example.com/mcp", NULL, NULL, &tools, &error));
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    json_decref(tools);
    free(error);
}

void test_tools_call_ok(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"hi\"}],\"isError\":false}}");
    json_t *result = NULL;
    char *error = NULL;
    json_t *args = json_object();
    TEST_ASSERT_TRUE(mcp_client_tools_call("https://example.com/mcp", NULL, NULL, "System.Info",
                                           args, &result, &error));
    json_decref(args);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "isError")));
    json_decref(result);
    free(error);
}

void test_fetch_tools_allowlist(void) {
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"protocolVersion\":\"2025-03-26\"}}");
    oidc_rp_http_test_set_response("example.com", 202, "");
    oidc_rp_http_test_set_response("example.com", 200,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":{\"tools\":["
        "{\"name\":\"System.Info\",\"inputSchema\":{\"type\":\"object\"}},"
        "{\"name\":\"Secret.Tool\",\"inputSchema\":{\"type\":\"object\"}}]}}");
    char *allowed[] = { (char *)"System.Info" };
    json_t *tools = mcp_client_fetch_tools("https://example.com/mcp", NULL, allowed, 1, "cid-1");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(tools, 0), "name")));
    json_decref(tools);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initialize_ok);
    RUN_TEST(test_tools_list_ok);
    RUN_TEST(test_tools_call_ok);
    RUN_TEST(test_fetch_tools_allowlist);
    return UNITY_END();
}
