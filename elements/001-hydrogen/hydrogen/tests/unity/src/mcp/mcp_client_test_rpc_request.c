#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>

void test_rpc_request_shape(void);
void test_rpc_request_null_method(void);
void test_rpc_request_with_params(void);

void setUp(void) {}
void tearDown(void) {}

void test_rpc_request_shape(void) {
    char *body = mcp_client_rpc_request(7, "tools/list", NULL);
    TEST_ASSERT_NOT_NULL(body);
    json_t *root = json_loads(body, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("2.0", json_string_value(json_object_get(root, "jsonrpc")));
    TEST_ASSERT_EQUAL_STRING("tools/list", json_string_value(json_object_get(root, "method")));
    TEST_ASSERT_EQUAL(7, json_integer_value(json_object_get(root, "id")));
    TEST_ASSERT_TRUE(json_is_object(json_object_get(root, "params")));
    json_decref(root);
    free(body);
}

void test_rpc_request_null_method(void) {
    TEST_ASSERT_NULL(mcp_client_rpc_request(1, NULL, NULL));
    TEST_ASSERT_NULL(mcp_client_rpc_request(1, "", NULL));
}

void test_rpc_request_with_params(void) {
    json_t *params = json_object();
    json_object_set_new(params, "name", json_string("System.Info"));
    char *body = mcp_client_rpc_request(3, "tools/call", params);
    json_decref(params);
    TEST_ASSERT_NOT_NULL(body);
    json_t *root = json_loads(body, 0, NULL);
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_object_get(root, "params"), "name")));
    json_decref(root);
    free(body);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rpc_request_shape);
    RUN_TEST(test_rpc_request_null_method);
    RUN_TEST(test_rpc_request_with_params);
    return UNITY_END();
}
