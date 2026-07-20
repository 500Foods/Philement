/*
 * Unity Test File: Chat Proxy Multi - Debug Callback
 * Tests multi_stream_debug_callback() in proxy_mc.c.
 *
 * The callback logs selected CURL connection/header diagnostics at the
 * appropriate log levels and ignores everything else (returns 0).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy_multi.h>

// Function prototype (declared non-static in proxy_multi.h for testing)
int multi_stream_debug_callback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);

// Test function prototypes
void test_multi_debug_callback_connected_text(void);
void test_multi_debug_callback_trying_text(void);
void test_multi_debug_callback_ssl_verify_text(void);
void test_multi_debug_callback_tls_text(void);
void test_multi_debug_callback_uninteresting_text(void);
void test_multi_debug_callback_header_out(void);
void test_multi_debug_callback_header_in(void);
void test_multi_debug_callback_empty(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_multi_debug_callback_connected_text(void) {
    const char* msg = "Connected to api.anthropic.com (9.9.9.9) port 443\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_trying_text(void) {
    const char* msg = "Trying 9.9.9.9:443...\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_ssl_verify_text(void) {
    const char* msg = "SSL certificate verify ok\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_tls_text(void) {
    const char* msg = "TLSv1.3 (IN), TLS handshake\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_uninteresting_text(void) {
    const char* msg = "Re-using existing connection\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_header_out(void) {
    const char* msg = "POST /v1/messages HTTP/1.1\r\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_HEADER_OUT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_header_in(void) {
    const char* msg = "HTTP/1.1 200 OK\r\n";
    int rc = multi_stream_debug_callback(NULL, CURLINFO_HEADER_IN, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_multi_debug_callback_empty(void) {
    int rc = multi_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)"", 0, NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_multi_debug_callback_connected_text);
    RUN_TEST(test_multi_debug_callback_trying_text);
    RUN_TEST(test_multi_debug_callback_ssl_verify_text);
    RUN_TEST(test_multi_debug_callback_tls_text);
    RUN_TEST(test_multi_debug_callback_uninteresting_text);
    RUN_TEST(test_multi_debug_callback_header_out);
    RUN_TEST(test_multi_debug_callback_header_in);
    RUN_TEST(test_multi_debug_callback_empty);

    return UNITY_END();
}
