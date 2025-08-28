/*
 * Unity Test File: Web Server Core - Get Endpoint for URL Test
 * This file contains unit tests for get_endpoint_for_url() function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/webserver/web_server_core.h"

// Standard library includes
#include <string.h>
#include <stdlib.h>

// Simple validator functions for testing
static bool always_true_validator(const char* url) {
    (void)url; // Unused parameter
    return true;
}

static bool always_false_validator(const char* url) {
    (void)url; // Unused parameter
    return false;
}

static bool prefix_validator(const char* url) {
    // Only allow URLs that start with "/api/"
    return strncmp(url, "/api/", 5) == 0;
}

// Dummy handler functions for testing
static enum MHD_Result dummy_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)connection; (void)url; (void)method; (void)version;
    (void)upload_data; (void)upload_data_size; (void)con_cls;
    return MHD_YES;
}

void setUp(void) {
    // Set up test fixtures - ensure clean endpoint registry
    // Note: We can't easily clear the global endpoint registry, so tests may interfere
    // In a real scenario, we'd want a way to reset the registry between tests
}

void tearDown(void) {
    // Clean up test fixtures - unregister test endpoints
    // This is a simple approach - in practice, we'd need a better way to manage this
}

// Test functions
static void test_get_endpoint_for_url_null_url(void) {
    // Test null URL parameter
    TEST_ASSERT_NULL(get_endpoint_for_url(NULL));
}

static void test_get_endpoint_for_url_empty_url(void) {
    // Test empty URL parameter
    TEST_ASSERT_NULL(get_endpoint_for_url(""));
}

static void test_get_endpoint_for_url_no_registered_endpoints(void) {
    // Test URL when no endpoints are registered
    // Note: This test may be unreliable if other tests register endpoints
    get_endpoint_for_url("/test");
    // We can't guarantee no endpoints are registered, so just check it doesn't crash
    TEST_PASS();
}

static void test_get_endpoint_for_url_exact_match_with_always_true_validator(void) {
    // Test exact match with a validator that always returns true
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/api";
    endpoint.validator = always_true_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Test exact match
    const WebServerEndpoint* result = get_endpoint_for_url("/api");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/api", result->prefix);

    // Test partial match that should succeed
    result = get_endpoint_for_url("/api/test");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/api", result->prefix);

    // Clean up
    unregister_web_endpoint("/api");
}

static void test_get_endpoint_for_url_exact_match_with_always_false_validator(void) {
    // Test exact match with a validator that always returns false
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/restricted";
    endpoint.validator = always_false_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Even exact match should fail due to validator
    const WebServerEndpoint* result = get_endpoint_for_url("/restricted");
    TEST_ASSERT_NULL(result);

    // Clean up
    unregister_web_endpoint("/restricted");
}

static void test_get_endpoint_for_url_with_prefix_validator(void) {
    // Test with a validator that checks for specific prefix
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/api";
    endpoint.validator = prefix_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Should match URLs starting with /api/
    const WebServerEndpoint* result = get_endpoint_for_url("/api/users");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/api", result->prefix);

    // Should NOT match URLs not starting with /api/
    result = get_endpoint_for_url("/other/path");
    TEST_ASSERT_NULL(result);

    // Clean up
    unregister_web_endpoint("/api");
}

static void test_get_endpoint_for_url_multiple_endpoints(void) {
    // Test with multiple registered endpoints
    WebServerEndpoint endpoint1 = {0};
    endpoint1.prefix = "/api";
    endpoint1.validator = always_true_validator;
    endpoint1.handler = dummy_handler;

    WebServerEndpoint endpoint2 = {0};
    endpoint2.prefix = "/web";
    endpoint2.validator = always_true_validator;
    endpoint2.handler = dummy_handler;

    bool registered1 = register_web_endpoint(&endpoint1);
    bool registered2 = register_web_endpoint(&endpoint2);
    TEST_ASSERT_TRUE(registered1);
    TEST_ASSERT_TRUE(registered2);

    // Test that correct endpoint is returned for each prefix
    const WebServerEndpoint* result1 = get_endpoint_for_url("/api/test");
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_EQUAL_STRING("/api", result1->prefix);

    const WebServerEndpoint* result2 = get_endpoint_for_url("/web/page");
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL_STRING("/web", result2->prefix);

    // Test non-matching URL
    const WebServerEndpoint* result3 = get_endpoint_for_url("/unknown");
    TEST_ASSERT_NULL(result3);

    // Clean up
    unregister_web_endpoint("/api");
    unregister_web_endpoint("/web");
}

static void test_get_endpoint_for_url_long_url(void) {
    // Test with a very long URL
    char long_url[2048];
    memset(long_url, 'a', sizeof(long_url) - 1);
    long_url[sizeof(long_url) - 1] = '\0';

    // Register an endpoint that could match
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/long";
    endpoint.validator = always_true_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Test long URL that matches
    char long_matching_url[2050];
    strcpy(long_matching_url, "/long/");
    memset(long_matching_url + 6, 'b', sizeof(long_matching_url) - 7);
    long_matching_url[sizeof(long_matching_url) - 1] = '\0';

    const WebServerEndpoint* result = get_endpoint_for_url(long_matching_url);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/long", result->prefix);

    // Clean up
    unregister_web_endpoint("/long");
}

static void test_get_endpoint_for_url_special_characters(void) {
    // Test URL with special characters
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/special";
    endpoint.validator = always_true_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    const char* test_urls[] = {
        "/special@#$%^&*()",
        "/special?param=value",
        "/special#fragment",
        "/special with spaces",
        "/special/中文/测试"
    };

    for (size_t i = 0; i < sizeof(test_urls) / sizeof(test_urls[0]); i++) {
        const WebServerEndpoint* result = get_endpoint_for_url(test_urls[i]);
        // These should match since validator always returns true
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_EQUAL_STRING("/special", result->prefix);
    }

    // Clean up
    unregister_web_endpoint("/special");
}

static void test_get_endpoint_for_url_root_path(void) {
    // Test root path "/"
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/";
    endpoint.validator = always_true_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    const WebServerEndpoint* result = get_endpoint_for_url("/");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/", result->prefix);

    // Clean up
    unregister_web_endpoint("/");
}

static void test_get_endpoint_for_url_without_leading_slash(void) {
    // Test URL without leading slash - should not match any endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test";
    endpoint.validator = always_true_validator;
    endpoint.handler = dummy_handler;

    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // URL without leading slash should not match
    const WebServerEndpoint* result = get_endpoint_for_url("test/path");
    TEST_ASSERT_NULL(result);

    // Clean up
    unregister_web_endpoint("/test");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_endpoint_for_url_null_url);
    RUN_TEST(test_get_endpoint_for_url_empty_url);
    RUN_TEST(test_get_endpoint_for_url_no_registered_endpoints);
    RUN_TEST(test_get_endpoint_for_url_exact_match_with_always_true_validator);
    RUN_TEST(test_get_endpoint_for_url_exact_match_with_always_false_validator);
    RUN_TEST(test_get_endpoint_for_url_with_prefix_validator);
    RUN_TEST(test_get_endpoint_for_url_multiple_endpoints);
    RUN_TEST(test_get_endpoint_for_url_long_url);
    RUN_TEST(test_get_endpoint_for_url_special_characters);
    RUN_TEST(test_get_endpoint_for_url_root_path);
    RUN_TEST(test_get_endpoint_for_url_without_leading_slash);

    return UNITY_END();
}
