/*
 * Unity Test File: API Utils api_get_request_url Function Tests
 * This file contains unit tests for the api_get_request_url function in api_utils.c
 *
 * api_get_request_url is a simple getter for the thread-local request URL
 * set by api_set_request_url. It is used by api_add_configured_headers
 * and is part of the public API.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // Reset the request URL to NULL to ensure test isolation
    // (g_api_request_url is a static __thread variable shared across tests)
    api_set_request_url(NULL);
}

void tearDown(void) {
    // Clean up after each test
    api_set_request_url(NULL);
}

// Test functions
void test_api_get_request_url_returns_null_initially(void);
void test_api_get_request_url_returns_set_url(void);
void test_api_get_request_url_returns_null_after_null_set(void);
void test_api_get_request_url_returns_same_pointer(void);
void test_api_get_request_url_with_special_chars(void);
void test_api_get_request_url_after_multiple_sets(void);

// Test that api_get_request_url returns NULL when no URL has been set
void test_api_get_request_url_returns_null_initially(void) {
    TEST_ASSERT_NULL(api_get_request_url());
}

// Test that api_get_request_url returns the URL set by api_set_request_url
void test_api_get_request_url_returns_set_url(void) {
    api_set_request_url("/api/conduit/query");
    TEST_ASSERT_NOT_NULL(api_get_request_url());
    TEST_ASSERT_EQUAL_STRING("/api/conduit/query", api_get_request_url());
}

// Test that setting NULL clears the URL
void test_api_get_request_url_returns_null_after_null_set(void) {
    api_set_request_url("/api/test");
    TEST_ASSERT_NOT_NULL(api_get_request_url());

    api_set_request_url(NULL);
    TEST_ASSERT_NULL(api_get_request_url());
}

// Test that api_get_request_url returns the exact same pointer (not a copy)
void test_api_get_request_url_returns_same_pointer(void) {
    const char *url = "/api/conduit/script";
    api_set_request_url(url);
    TEST_ASSERT_EQUAL_PTR(url, api_get_request_url());
}

// Test with a URL containing special characters
void test_api_get_request_url_with_special_chars(void) {
    api_set_request_url("/api/v1/data?key=value&foo=bar");
    TEST_ASSERT_EQUAL_STRING("/api/v1/data?key=value&foo=bar",
                             api_get_request_url());
}

// Test that each call to api_set_request_url updates the value
void test_api_get_request_url_after_multiple_sets(void) {
    api_set_request_url("/api/first");
    TEST_ASSERT_EQUAL_STRING("/api/first", api_get_request_url());

    api_set_request_url("/api/second");
    TEST_ASSERT_EQUAL_STRING("/api/second", api_get_request_url());

    api_set_request_url("/api/third");
    TEST_ASSERT_EQUAL_STRING("/api/third", api_get_request_url());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_api_get_request_url_returns_null_initially);
    RUN_TEST(test_api_get_request_url_returns_set_url);
    RUN_TEST(test_api_get_request_url_returns_null_after_null_set);
    RUN_TEST(test_api_get_request_url_returns_same_pointer);
    RUN_TEST(test_api_get_request_url_with_special_chars);
    RUN_TEST(test_api_get_request_url_after_multiple_sets);

    return UNITY_END();
}
