/*
 * Unity Test File: WebSocket Chat Proxy Result Sender Tests
 * Tests send_chat_proxy_result helper function.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_send_chat_proxy_result_null_result(void);
void test_send_chat_proxy_result_error_code(void);
void test_send_chat_proxy_result_parse_fails(void);
void test_send_chat_proxy_result_success(void);
void test_send_chat_proxy_result_success_with_raw_response(void);

// Test fixtures
static struct lws *test_wsi;
static ChatEngineConfig test_engine;

void setUp(void) {
    test_wsi = (struct lws *)0x12345678;
    memset(&test_engine, 0, sizeof(test_engine));
    strncpy(test_engine.name, "test-engine", sizeof(test_engine.name) - 1);
    strncpy(test_engine.model, "gpt-4", sizeof(test_engine.model) - 1);
    test_engine.provider = CEC_PROVIDER_OPENAI;
    mock_lws_reset_all();
}

void tearDown(void) {
    mock_lws_reset_all();
}

void test_send_chat_proxy_result_null_result(void) {
    struct timespec start_time = {0};
    int result = send_chat_proxy_result(test_wsi, "req-123", &test_engine, NULL, start_time, 1);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_send_chat_proxy_result_error_code(void) {
    ChatProxyResult *proxy_result = chat_proxy_result_create();
    proxy_result->code = CHAT_PROXY_ERROR_CONNECT;
    proxy_result->error_message = strdup("Connection failed");

    struct timespec start_time = {0};
    int result = send_chat_proxy_result(test_wsi, "req-123", &test_engine, proxy_result, start_time, 1);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_send_chat_proxy_result_parse_fails(void) {
    ChatProxyResult *proxy_result = chat_proxy_result_create();
    proxy_result->code = CHAT_PROXY_OK;
    proxy_result->response_body = strdup("not valid json");
    proxy_result->response_size = strlen(proxy_result->response_body);

    struct timespec start_time = {0};
    int result = send_chat_proxy_result(test_wsi, "req-123", &test_engine, proxy_result, start_time, 1);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_send_chat_proxy_result_success(void) {
    ChatProxyResult *proxy_result = chat_proxy_result_create();
    proxy_result->code = CHAT_PROXY_OK;
    proxy_result->response_body = strdup("{\"choices\":[{\"message\":{\"content\":\"Hello\"},\"finish_reason\":\"stop\"}],\"model\":\"gpt-4\",\"usage\":{\"prompt_tokens\":5,\"completion_tokens\":2,\"total_tokens\":7}}");
    proxy_result->response_size = strlen(proxy_result->response_body);

    mock_lws_set_write_result(200);

    struct timespec start_time = {0};
    int result = send_chat_proxy_result(test_wsi, "req-123", &test_engine, proxy_result, start_time, 1);

    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_send_chat_proxy_result_success_with_raw_response(void) {
    ChatProxyResult *proxy_result = chat_proxy_result_create();
    proxy_result->code = CHAT_PROXY_OK;
    proxy_result->response_body = strdup("{\"choices\":[{\"message\":{\"content\":\"Hello\"},\"finish_reason\":\"stop\"}],\"model\":\"gpt-4\",\"usage\":{\"prompt_tokens\":5,\"completion_tokens\":2,\"total_tokens\":7},\"retrieval\":\"data\"}");
    proxy_result->response_size = strlen(proxy_result->response_body);

    mock_lws_set_write_result(250);

    struct timespec start_time = {0};
    int result = send_chat_proxy_result(test_wsi, "req-123", &test_engine, proxy_result, start_time, 1);

    TEST_ASSERT_EQUAL_INT(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_chat_proxy_result_null_result);
    RUN_TEST(test_send_chat_proxy_result_error_code);
    RUN_TEST(test_send_chat_proxy_result_parse_fails);
    RUN_TEST(test_send_chat_proxy_result_success);
    RUN_TEST(test_send_chat_proxy_result_success_with_raw_response);

    return UNITY_END();
}
