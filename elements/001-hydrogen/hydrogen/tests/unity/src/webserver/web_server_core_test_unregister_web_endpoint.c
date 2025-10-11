/*
 * Unity Test File: Web Server Core - Unregister Web Endpoint Test
 * This file contains unit tests for unregister_web_endpoint() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_core.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>

// Dummy functions for testing
static bool dummy_validator(const char* url) {
    (void)url; // Unused parameter
    return true;
}

static enum MHD_Result dummy_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)connection; (void)url; (void)method; (void)version;
    (void)upload_data; (void)upload_data_size; (void)con_cls;
    return MHD_YES;
}

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_unregister_web_endpoint_null_prefix(void) {
    // Test null prefix parameter - should not crash
    unregister_web_endpoint(NULL);
    TEST_PASS(); // If we reach here, function handled null gracefully
}

static void test_unregister_web_endpoint_nonexistent_prefix(void) {
    // Test unregistering a prefix that doesn't exist - should not crash
    unregister_web_endpoint("/nonexistent");
    TEST_PASS(); // If we reach here, function handled gracefully
}

static void test_unregister_web_endpoint_empty_prefix(void) {
    // Test unregistering with empty prefix
    unregister_web_endpoint("");
    TEST_PASS(); // If we reach here, function handled gracefully
}

static void test_unregister_web_endpoint_valid_prefix(void) {
    // Test unregistering a valid registered endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test_unregister";
    endpoint.validator = dummy_validator;
    endpoint.handler = dummy_handler;

    // First register the endpoint
    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Now test unregistering it
    unregister_web_endpoint("/test_unregister");

    // Should be able to register the same prefix again (confirming it was removed)
    bool registered_again = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered_again);
}

static void test_unregister_web_endpoint_multiple_calls(void) {
    // Test multiple calls to unregister the same prefix
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test_multiple";
    endpoint.validator = dummy_validator;
    endpoint.handler = dummy_handler;

    // Register endpoint
    bool registered = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered);

    // Multiple unregister calls should be safe
    unregister_web_endpoint("/test_multiple");
    unregister_web_endpoint("/test_multiple");
    unregister_web_endpoint("/test_multiple");

    // Should be able to register again
    bool registered_again = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(registered_again);
}

static void test_unregister_web_endpoint_long_prefix(void) {
    // Test unregistering with a very long prefix
    char long_prefix[1024];
    memset(long_prefix, 'a', sizeof(long_prefix) - 1);
    long_prefix[sizeof(long_prefix) - 1] = '\0';

    unregister_web_endpoint(long_prefix);
    TEST_PASS(); // If we reach here, function handled gracefully
}

static void test_unregister_web_endpoint_special_characters(void) {
    // Test unregistering with special characters in prefix
    unregister_web_endpoint("/test@#$%^&*()");
    TEST_PASS(); // If we reach here, function handled gracefully
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_unregister_web_endpoint_null_prefix);
    RUN_TEST(test_unregister_web_endpoint_nonexistent_prefix);
    RUN_TEST(test_unregister_web_endpoint_empty_prefix);
    RUN_TEST(test_unregister_web_endpoint_valid_prefix);
    RUN_TEST(test_unregister_web_endpoint_multiple_calls);
    RUN_TEST(test_unregister_web_endpoint_long_prefix);
    RUN_TEST(test_unregister_web_endpoint_special_characters);

    return UNITY_END();
}
