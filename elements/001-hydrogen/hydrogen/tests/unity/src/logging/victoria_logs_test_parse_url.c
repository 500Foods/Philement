/*
 * Unity Test File: victoria_logs_parse_url Tests
 * This file contains unit tests for the victoria_logs_parse_url() function
 * from src/logging/victoria_logs.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// Function prototypes for test functions
void test_victoria_logs_parse_url_null_params(void);
void test_victoria_logs_parse_url_http_default(void);
void test_victoria_logs_parse_url_https_default(void);
void test_victoria_logs_parse_url_with_port(void);
void test_victoria_logs_parse_url_no_path(void);
void test_victoria_logs_parse_url_custom_path(void);
void test_victoria_logs_parse_url_host_too_long(void);

// Test fixtures
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test null parameter handling
void test_victoria_logs_parse_url_null_params(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    TEST_ASSERT_FALSE(victoria_logs_parse_url(NULL, host, &port, path, &use_ssl));
    TEST_ASSERT_FALSE(victoria_logs_parse_url("http://localhost", NULL, &port, path, &use_ssl));
    TEST_ASSERT_FALSE(victoria_logs_parse_url("http://localhost", host, NULL, path, &use_ssl));
    TEST_ASSERT_FALSE(victoria_logs_parse_url("http://localhost", host, &port, NULL, &use_ssl));
    TEST_ASSERT_FALSE(victoria_logs_parse_url("http://localhost", host, &port, path, NULL));
}

// Test http default port (80) with path
void test_victoria_logs_parse_url_http_default(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url("http://localhost/insert/jsonline", host, &port, path, &use_ssl);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(use_ssl);
    TEST_ASSERT_EQUAL(80, port);
    TEST_ASSERT_EQUAL_STRING("localhost", host);
    TEST_ASSERT_EQUAL_STRING("/insert/jsonline", path);
}

// Test https default port (443) with path
void test_victoria_logs_parse_url_https_default(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url("https://logs.example.com/insert", host, &port, path, &use_ssl);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(use_ssl);
    TEST_ASSERT_EQUAL(443, port);
    TEST_ASSERT_EQUAL_STRING("logs.example.com", host);
    TEST_ASSERT_EQUAL_STRING("/insert", path);
}

// Test explicit port in host:port format
void test_victoria_logs_parse_url_with_port(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url("http://victoria:9428/insert/jsonline", host, &port, path, &use_ssl);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(use_ssl);
    TEST_ASSERT_EQUAL(9428, port);
    TEST_ASSERT_EQUAL_STRING("victoria", host);
    TEST_ASSERT_EQUAL_STRING("/insert/jsonline", path);
}

// Test URL with no path (defaults to "/")
void test_victoria_logs_parse_url_no_path(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url("http://localhost", host, &port, path, &use_ssl);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("localhost", host);
    TEST_ASSERT_EQUAL_STRING("/", path);
}

// Test URL with a custom deep path
void test_victoria_logs_parse_url_custom_path(void) {
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url("http://localhost:8428/custom/path/here", host, &port, path, &use_ssl);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(8428, port);
    TEST_ASSERT_EQUAL_STRING("localhost", host);
    TEST_ASSERT_EQUAL_STRING("/custom/path/here", path);
}

// Test hostname longer than 256 chars fails parsing
void test_victoria_logs_parse_url_host_too_long(void) {
    char url[300];
    strcpy(url, "http://");
    for (int i = 0; i < 260; i++) {
        url[7 + i] = 'a';
    }
    url[267] = '\0';

    char host[256];
    int port;
    char path[1024];
    bool use_ssl;

    bool result = victoria_logs_parse_url(url, host, &port, path, &use_ssl);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_parse_url_null_params);
    RUN_TEST(test_victoria_logs_parse_url_http_default);
    RUN_TEST(test_victoria_logs_parse_url_https_default);
    RUN_TEST(test_victoria_logs_parse_url_with_port);
    RUN_TEST(test_victoria_logs_parse_url_no_path);
    RUN_TEST(test_victoria_logs_parse_url_custom_path);
    RUN_TEST(test_victoria_logs_parse_url_host_too_long);

    return UNITY_END();
}
