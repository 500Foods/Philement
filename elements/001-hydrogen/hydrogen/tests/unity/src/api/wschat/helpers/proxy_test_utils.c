#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>
#include <string.h>
#include <stdlib.h>

void test_proxy_result_create_defaults(void);
void test_proxy_result_destroy_null(void);
void test_proxy_result_destroy_valid(void);
void test_proxy_default_config(void);
void test_proxy_streaming_config(void);
void test_proxy_result_is_success_true(void);
void test_proxy_result_is_success_null(void);
void test_proxy_result_is_success_not_ok(void);
void test_proxy_result_is_success_http_4xx(void);
void test_proxy_result_is_success_http_5xx(void);
void test_proxy_result_is_success_http_3xx(void);
void test_proxy_should_retry_timeout(void);
void test_proxy_should_retry_network(void);
void test_proxy_should_retry_connect(void);
void test_proxy_should_retry_http_429(void);
void test_proxy_should_retry_http_502(void);
void test_proxy_should_retry_http_503(void);
void test_proxy_should_retry_http_504(void);
void test_proxy_should_retry_no_retry(void);
void test_proxy_should_retry_null(void);
void test_multi_result_create(void);
void test_multi_result_create_null_count(void);
void test_multi_result_destroy_null(void);
void test_multi_result_destroy_valid(void);
void test_multi_result_destroy_with_results(void);
void test_multi_manager_get_instance(void);

void setUp(void) {}
void tearDown(void) {}

void test_proxy_result_create_defaults(void) {
    ChatProxyResult* result = chat_proxy_result_create();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_UNKNOWN, result->code);
    TEST_ASSERT_EQUAL_INT(0, result->http_status);
    TEST_ASSERT_NULL(result->response_body);
    TEST_ASSERT_EQUAL_size_t(0, result->response_size);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, result->total_time_ms);
    TEST_ASSERT_NULL(result->error_message);
    free(result);
}

void test_proxy_result_destroy_null(void) {
    chat_proxy_result_destroy(NULL);
}

void test_proxy_result_destroy_valid(void) {
    ChatProxyResult* result = chat_proxy_result_create();
    TEST_ASSERT_NOT_NULL(result);
    result->response_body = strdup("response body");
    result->error_message = strdup("error message");
    chat_proxy_result_destroy(result);
}

void test_proxy_default_config(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(60, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(2, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_proxy_streaming_config(void) {
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(600, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(0, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_proxy_result_is_success_true(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_OK;
    result.http_status = 200;
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_is_success_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(NULL));
}

void test_proxy_result_is_success_not_ok(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_UNKNOWN;
    result.http_status = 200;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_is_success_http_4xx(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_OK;
    result.http_status = 404;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_is_success_http_5xx(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_OK;
    result.http_status = 500;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_is_success_http_3xx(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_OK;
    result.http_status = 301;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
}

void test_proxy_should_retry_timeout(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_TIMEOUT;
    result.http_status = 0;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_network(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_NETWORK;
    result.http_status = 0;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_connect(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_CONNECT;
    result.http_status = 0;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_http_429(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_HTTP_4XX;
    result.http_status = 429;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_http_502(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_HTTP_5XX;
    result.http_status = 502;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_http_503(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_HTTP_5XX;
    result.http_status = 503;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_http_504(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_HTTP_5XX;
    result.http_status = 504;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_no_retry(void) {
    ChatProxyResult result;
    result.code = CHAT_PROXY_ERROR_HTTP_4XX;
    result.http_status = 404;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
}

void test_proxy_should_retry_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(NULL));
}

void test_multi_result_create(void) {
    ChatMultiResult* multi = chat_multi_result_create(3);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(3, multi->count);
    TEST_ASSERT_NOT_NULL(multi->results);
    TEST_ASSERT_EQUAL_size_t(0, multi->success_count);
    TEST_ASSERT_EQUAL_size_t(0, multi->failure_count);

    /* All results should be NULL */
    for (size_t i = 0; i < multi->count; i++) {
        TEST_ASSERT_NULL(multi->results[i]);
    }

    chat_multi_result_destroy(multi);
}

void test_multi_result_create_null_count(void) {
    /* calloc(0, ...) may return NULL or a valid pointer — either is acceptable */
    ChatMultiResult* multi = chat_multi_result_create(0);
    if (multi) {
        chat_multi_result_destroy(multi);
    }
}

void test_multi_result_destroy_null(void) {
    chat_multi_result_destroy(NULL);
}

void test_multi_result_destroy_valid(void) {
    ChatMultiResult* multi = chat_multi_result_create(2);
    TEST_ASSERT_NOT_NULL(multi);
    chat_multi_result_destroy(multi);
}

void test_multi_result_destroy_with_results(void) {
    ChatMultiResult* multi = chat_multi_result_create(2);
    TEST_ASSERT_NOT_NULL(multi);

    /* Populate results */
    multi->results[0] = chat_proxy_result_create();
    multi->results[0]->code = CHAT_PROXY_OK;
    multi->results[0]->response_body = strdup("result 1");
    multi->results[1] = chat_proxy_result_create();
    multi->results[1]->code = CHAT_PROXY_ERROR_NETWORK;
    multi->results[1]->error_message = strdup("network error");

    chat_multi_result_destroy(multi);
}

void test_multi_manager_get_instance(void) {
    MultiStreamManager* mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_EQUAL_PTR(mgr, chat_proxy_get_multi_manager());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_proxy_result_create_defaults);
    RUN_TEST(test_proxy_result_destroy_null);
    RUN_TEST(test_proxy_result_destroy_valid);
    RUN_TEST(test_proxy_default_config);
    RUN_TEST(test_proxy_streaming_config);
    RUN_TEST(test_proxy_result_is_success_true);
    RUN_TEST(test_proxy_result_is_success_null);
    RUN_TEST(test_proxy_result_is_success_not_ok);
    RUN_TEST(test_proxy_result_is_success_http_4xx);
    RUN_TEST(test_proxy_result_is_success_http_5xx);
    RUN_TEST(test_proxy_result_is_success_http_3xx);
    RUN_TEST(test_proxy_should_retry_timeout);
    RUN_TEST(test_proxy_should_retry_network);
    RUN_TEST(test_proxy_should_retry_connect);
    RUN_TEST(test_proxy_should_retry_http_429);
    RUN_TEST(test_proxy_should_retry_http_502);
    RUN_TEST(test_proxy_should_retry_http_503);
    RUN_TEST(test_proxy_should_retry_http_504);
    RUN_TEST(test_proxy_should_retry_no_retry);
    RUN_TEST(test_proxy_should_retry_null);
    RUN_TEST(test_multi_result_create);
    RUN_TEST(test_multi_result_create_null_count);
    RUN_TEST(test_multi_result_destroy_null);
    RUN_TEST(test_multi_result_destroy_valid);
    RUN_TEST(test_multi_result_destroy_with_results);
    RUN_TEST(test_multi_manager_get_instance);
    return UNITY_END();
}
