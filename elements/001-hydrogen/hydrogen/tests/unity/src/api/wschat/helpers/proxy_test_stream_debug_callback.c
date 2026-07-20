/*
 * Unity Test File: Chat Proxy - Streaming Debug Callback
 * Tests chat_proxy_stream_debug_callback() in proxy.c.
 *
 * The CURL debug callback logs only relevant connection/header events at the
 * appropriate log levels; everything else is ignored (returns 0).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>

// Function prototype (declared non-static in proxy.h for testing)
int chat_proxy_stream_debug_callback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);

// Test function prototypes
void test_stream_debug_callback_connect_text(void);
void test_stream_debug_callback_trying_text(void);
void test_stream_debug_callback_tls_text(void);
void test_stream_debug_callback_uninteresting_text(void);
void test_stream_debug_callback_header_out(void);
void test_stream_debug_callback_empty(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_stream_debug_callback_connect_text(void) {
    // CURLINFO_TEXT containing "Connected to" should be accepted and return 0
    const char* msg = "Connected to api.openai.com (1.2.3.4) port 443\n";
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_stream_debug_callback_trying_text(void) {
    const char* msg = "Trying 1.2.3.4:443...\n";
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_stream_debug_callback_tls_text(void) {
    const char* msg = "SSL connection using TLSv1.3\n";
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_stream_debug_callback_uninteresting_text(void) {
    // Text that does not match any interesting substring is still accepted
    const char* msg = "Expire: never\n";
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_stream_debug_callback_header_out(void) {
    const char* msg = "POST /v1/chat/completions HTTP/1.1\r\n";
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_HEADER_OUT, (char*)msg, strlen(msg), NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_stream_debug_callback_empty(void) {
    // size == 0 must be tolerated (no logging, returns 0)
    int rc = chat_proxy_stream_debug_callback(NULL, CURLINFO_TEXT, (char*)"", 0, NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_stream_debug_callback_connect_text);
    RUN_TEST(test_stream_debug_callback_trying_text);
    RUN_TEST(test_stream_debug_callback_tls_text);
    RUN_TEST(test_stream_debug_callback_uninteresting_text);
    RUN_TEST(test_stream_debug_callback_header_out);
    RUN_TEST(test_stream_debug_callback_empty);

    return UNITY_END();
}
