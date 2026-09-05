#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>
#include <string.h>

void test_unwrap_body_null(void);
void test_unwrap_body_plain_json(void);
void test_unwrap_body_data_prefix(void);
void test_unwrap_body_data_prefix_with_space(void);
void test_unwrap_body_event_prefix(void);
void test_unwrap_body_sse_multiline(void);
void test_unwrap_body_sse_data_newline_trailing(void);
void test_unwrap_body_empty_string(void);
void test_unwrap_body_data_colon_at_start(void);

void setUp(void) {}
void tearDown(void) {}

void test_unwrap_body_null(void) {
    TEST_ASSERT_NULL(mcp_client_unwrap_body(NULL));
}

void test_unwrap_body_plain_json(void) {
    char *result = mcp_client_unwrap_body("{\"jsonrpc\":\"2.0\"}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"jsonrpc\":\"2.0\"}", result);
    free(result);
}

void test_unwrap_body_data_prefix(void) {
    char *result = mcp_client_unwrap_body("data: {\"jsonrpc\":\"2.0\"}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"jsonrpc\":\"2.0\"}", result);
    free(result);
}

void test_unwrap_body_data_prefix_with_space(void) {
    char *result = mcp_client_unwrap_body("data: {\"jsonrpc\":\"2.0\",\"id\":1}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"jsonrpc\":\"2.0\",\"id\":1}", result);
    free(result);
}

void test_unwrap_body_event_prefix(void) {
    char *result = mcp_client_unwrap_body("event: message\n");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("event: message\n", result);
    free(result);
}

void test_unwrap_body_sse_multiline(void) {
    char *result = mcp_client_unwrap_body(
        "event: message\n"
        "data: {\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}\n\n");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}", result);
    free(result);
}

void test_unwrap_body_sse_data_newline_trailing(void) {
    char *result = mcp_client_unwrap_body("data: {\"ok\":true}\n");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"ok\":true}", result);
    free(result);
}

void test_unwrap_body_empty_string(void) {
    char *result = mcp_client_unwrap_body("");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_unwrap_body_data_colon_at_start(void) {
    char *result = mcp_client_unwrap_body("data:{}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{}", result);
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_unwrap_body_null);
    RUN_TEST(test_unwrap_body_plain_json);
    RUN_TEST(test_unwrap_body_data_prefix);
    RUN_TEST(test_unwrap_body_data_prefix_with_space);
    RUN_TEST(test_unwrap_body_event_prefix);
    RUN_TEST(test_unwrap_body_sse_multiline);
    RUN_TEST(test_unwrap_body_sse_data_newline_trailing);
    RUN_TEST(test_unwrap_body_empty_string);
    RUN_TEST(test_unwrap_body_data_colon_at_start);
    return UNITY_END();
}
