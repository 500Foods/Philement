/*
 * Unity Test File: Chat Proxy - Result & Config Helpers
 * Tests the network-independent helper functions in src/api/wschat/helpers/proxy.c:
 *   - chat_proxy_result_create / chat_proxy_result_destroy
 *   - chat_proxy_result_code_to_string (all enum branches)
 *   - chat_proxy_result_is_success
 *   - chat_proxy_result_should_retry (all branches)
 *   - chat_proxy_get_default_config / chat_proxy_get_streaming_config
 *   - chat_multi_result_create / chat_multi_result_destroy
 *   - chat_proxy_write_callback (buffer growth, max size, realloc failure)
 */

#include <src/hydrogen.h>
#include <unity.h>

// Source header (functions are declared non-static for Unity testing)
#include <src/api/wschat/helpers/proxy.h>

// Function prototypes
void test_result_create_destroy(void);
void test_result_code_to_string_all_cases(void);
void test_result_is_success(void);
void test_result_should_retry(void);
void test_get_default_config(void);
void test_get_streaming_config(void);
void test_multi_result_create_destroy(void);
void test_write_callback_appends(void);
void test_write_callback_grows_buffer(void);
void test_write_callback_exceeds_max_size(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_result_create_destroy(void) {
    ChatProxyResult* result = chat_proxy_result_create();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_UNKNOWN, result->code);
    TEST_ASSERT_EQUAL_INT(0, result->http_status);
    TEST_ASSERT_NULL(result->response_body);
    TEST_ASSERT_EQUAL_size_t(0, result->response_size);
    TEST_ASSERT_EQUAL(0.0, result->total_time_ms);
    TEST_ASSERT_NULL(result->error_message);

    // Should be safe to destroy a freshly created result
    chat_proxy_result_destroy(result);

    // Destroying NULL must be safe
    chat_proxy_result_destroy(NULL);
}

void test_result_code_to_string_all_cases(void) {
    TEST_ASSERT_EQUAL_STRING("Success", chat_proxy_result_code_to_string(CHAT_PROXY_OK));
    TEST_ASSERT_EQUAL_STRING("CURL initialization failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_INIT));
    TEST_ASSERT_EQUAL_STRING("Connection failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_CONNECT));
    TEST_ASSERT_EQUAL_STRING("Request timeout", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("HTTP client error (4xx)", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_HTTP_4XX));
    TEST_ASSERT_EQUAL_STRING("HTTP server error (5xx)", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_HTTP_5XX));
    TEST_ASSERT_EQUAL_STRING("Memory allocation failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Network error", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_NETWORK));
    TEST_ASSERT_EQUAL_STRING("Unknown error", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_UNKNOWN));
    // Unknown enum value falls through to "Unknown"
    TEST_ASSERT_EQUAL_STRING("Unknown", chat_proxy_result_code_to_string((ChatProxyResultCode)999));
}

void test_result_is_success(void) {
    ChatProxyResult result = {0};
    // NULL result is never a success
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(NULL));
    // Wrong code
    result.code = CHAT_PROXY_ERROR_UNKNOWN;
    result.http_status = 200;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
    // OK code but out-of-range status
    result.code = CHAT_PROXY_OK;
    result.http_status = 199;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
    result.http_status = 300;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
    // OK code with valid 2xx status
    result.http_status = 200;
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(&result));
    result.http_status = 299;
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(&result));
}

void test_result_should_retry(void) {
    ChatProxyResult result = {0};
    // NULL result
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(NULL));

    // Retry on specific result codes
    result.http_status = 0;
    result.code = CHAT_PROXY_ERROR_TIMEOUT;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
    result.code = CHAT_PROXY_ERROR_NETWORK;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
    result.code = CHAT_PROXY_ERROR_CONNECT;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));

    // Retry on specific HTTP status codes
    result.code = CHAT_PROXY_OK;
    result.http_status = 429;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
    result.http_status = 502;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
    result.http_status = 503;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
    result.http_status = 504;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));

    // Non-retryable codes/status
    result.code = CHAT_PROXY_ERROR_UNKNOWN;
    result.http_status = 200;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
    result.code = CHAT_PROXY_OK;
    result.http_status = 400;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
    result.http_status = 500;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
    result.http_status = 200;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
}

void test_get_default_config(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(60, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(2, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_get_streaming_config(void) {
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(600, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(0, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_multi_result_create_destroy(void) {
    ChatMultiResult* multi = chat_multi_result_create(3);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(3, multi->count);
    TEST_ASSERT_EQUAL(0.0, multi->total_time_ms);
    TEST_ASSERT_EQUAL_size_t(0, multi->success_count);
    TEST_ASSERT_EQUAL_size_t(0, multi->failure_count);
    TEST_ASSERT_NOT_NULL(multi->results);

    // Destroying NULL must be safe
    chat_multi_result_destroy(NULL);
    // Destroying a freshly created multi must be safe
    chat_multi_result_destroy(multi);

    // Zero-count allocation still yields a valid (empty) structure
    ChatMultiResult* empty = chat_multi_result_create(0);
    TEST_ASSERT_NOT_NULL(empty);
    TEST_ASSERT_EQUAL_size_t(0, empty->count);
    chat_multi_result_destroy(empty);
}

/* chat_proxy_write_callback copies incoming data into a ChatResponseBuffer,
 * growing it as needed, or returning 0 when the max size is exceeded. The
 * USE_MOCK_SYSTEM global mock cannot reliably force realloc failure here, so
 * we exercise the normal append and growth paths plus the max-size guard. */
void test_write_callback_appends(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(16);
    TEST_ASSERT_NOT_NULL(buffer.data);
    buffer.data[0] = '\0';
    buffer.size = 0;
    buffer.capacity = 16;

    const char* chunk = "hello";
    size_t written = chat_proxy_write_callback(chunk, 1, 5, &buffer);
    TEST_ASSERT_EQUAL_size_t(5, written);
    TEST_ASSERT_EQUAL_size_t(5, buffer.size);
    TEST_ASSERT_EQUAL_STRING("hello", buffer.data);

    // Append more data without exceeding capacity
    written = chat_proxy_write_callback("!", 1, 1, &buffer);
    TEST_ASSERT_EQUAL_size_t(1, written);
    TEST_ASSERT_EQUAL_size_t(6, buffer.size);
    TEST_ASSERT_EQUAL_STRING("hello!", buffer.data);

    free(buffer.data);
}

void test_write_callback_grows_buffer(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(4);
    TEST_ASSERT_NOT_NULL(buffer.data);
    buffer.data[0] = '\0';
    buffer.size = 0;
    buffer.capacity = 4;

    // Pushing 10 bytes forces reallocation (capacity doubles from 4 -> 8 -> 16)
    const char* chunk = "abcdefghij";
    size_t written = chat_proxy_write_callback(chunk, 1, 10, &buffer);
    TEST_ASSERT_EQUAL_size_t(10, written);
    TEST_ASSERT_EQUAL_size_t(10, buffer.size);
    TEST_ASSERT_EQUAL_INT(16, (int)buffer.capacity);
    TEST_ASSERT_EQUAL_STRING("abcdefghij", buffer.data);

    free(buffer.data);
}

void test_write_callback_exceeds_max_size(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(8);
    TEST_ASSERT_NOT_NULL(buffer.data);
    buffer.data[0] = '\0';
    buffer.size = 0;
    buffer.capacity = 8;

    // MAX_RESPONSE_SIZE is 8MB; synthesize an almost-full buffer so that
    // size + realsize exceeds the limit and trips the guard.
    buffer.size = (8 * 1024 * 1024) - 1;
    const char* chunk = "xy";
    size_t written = chat_proxy_write_callback(chunk, 1, 2, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, written);

    free(buffer.data);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_result_create_destroy);
    RUN_TEST(test_result_code_to_string_all_cases);
    RUN_TEST(test_result_is_success);
    RUN_TEST(test_result_should_retry);
    RUN_TEST(test_get_default_config);
    RUN_TEST(test_get_streaming_config);
    RUN_TEST(test_multi_result_create_destroy);
    RUN_TEST(test_write_callback_appends);
    RUN_TEST(test_write_callback_grows_buffer);
    RUN_TEST(test_write_callback_exceeds_max_size);

    return UNITY_END();
}
