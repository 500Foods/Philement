#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>

void test_rpc_parse_result_ok(void);
void test_rpc_parse_result_error(void);
void test_rpc_parse_result_sse(void);
void test_rpc_parse_result_empty(void);

void setUp(void) {}
void tearDown(void) {}

void test_rpc_parse_result_ok(void) {
    json_t *result = NULL;
    char *error = NULL;
    bool ok = mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"tools\":[]}}", &result, &error);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_array(json_object_get(result, "tools")));
    json_decref(result);
}

void test_rpc_parse_result_error(void) {
    json_t *result = NULL;
    char *error = NULL;
    bool ok = mcp_client_rpc_parse_result(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":{\"code\":-32603,\"message\":\"boom\"}}",
        &result, &error);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("boom", error);
    free(error);
}

void test_rpc_parse_result_sse(void) {
    json_t *result = NULL;
    char *error = NULL;
    bool ok = mcp_client_rpc_parse_result(
        "event: message\ndata: {\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}\n\n",
        &result, &error);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(result, "ok")));
    json_decref(result);
    free(error);
}

void test_rpc_parse_result_empty(void) {
    json_t *result = NULL;
    char *error = NULL;
    TEST_ASSERT_FALSE(mcp_client_rpc_parse_result("", &result, &error));
    TEST_ASSERT_NOT_NULL(error);
    free(error);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rpc_parse_result_ok);
    RUN_TEST(test_rpc_parse_result_error);
    RUN_TEST(test_rpc_parse_result_sse);
    RUN_TEST(test_rpc_parse_result_empty);
    return UNITY_END();
}
