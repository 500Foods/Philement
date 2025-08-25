/*
 * Unity Test File: Web Server Core - Register Web Endpoint Test
 * This file contains unit tests for register_web_endpoint() function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/webserver/web_server_core.h"

// Standard library includes
#include <string.h>
#include <stdlib.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_register_web_endpoint_null_endpoint(void) {
    // Test null endpoint parameter
    TEST_ASSERT_FALSE(register_web_endpoint(NULL));
}

static void test_register_web_endpoint_null_prefix(void) {
    // Test null prefix in endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = NULL;
    endpoint.validator = (bool (*)(const char*))1; // Non-null function pointer
    endpoint.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    TEST_ASSERT_FALSE(register_web_endpoint(&endpoint));
}

static void test_register_web_endpoint_null_validator(void) {
    // Test null validator in endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test";
    endpoint.validator = NULL;
    endpoint.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    TEST_ASSERT_FALSE(register_web_endpoint(&endpoint));
}

static void test_register_web_endpoint_null_handler(void) {
    // Test null handler in endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test";
    endpoint.validator = (bool (*)(const char*))1;
    endpoint.handler = NULL;

    TEST_ASSERT_FALSE(register_web_endpoint(&endpoint));
}

static void test_register_web_endpoint_valid_endpoint(void) {
    // Test registering a valid endpoint
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "/test";
    endpoint.validator = (bool (*)(const char*))1; // Non-null function pointer
    endpoint.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    // First test that registration succeeds
    bool result = register_web_endpoint(&endpoint);
    TEST_ASSERT_TRUE(result);
}

static void test_register_web_endpoint_duplicate_prefix(void) {
    // Test registering endpoint with duplicate prefix
    WebServerEndpoint endpoint1 = {0};
    endpoint1.prefix = "/duplicate";
    endpoint1.validator = (bool (*)(const char*))1;
    endpoint1.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    WebServerEndpoint endpoint2 = {0};
    endpoint2.prefix = "/duplicate"; // Same prefix
    endpoint2.validator = (bool (*)(const char*))1;
    endpoint2.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    TEST_ASSERT_TRUE(register_web_endpoint(&endpoint1));
    TEST_ASSERT_FALSE(register_web_endpoint(&endpoint2));
}

static void test_register_web_endpoint_max_endpoints(void) {
    // Test registering beyond maximum endpoints - skip due to global state
    TEST_IGNORE_MESSAGE("Skipping test due to global state dependencies");
}

static void test_register_web_endpoint_empty_prefix(void) {
    // Test registering with empty prefix - should succeed (current implementation allows empty prefix)
    WebServerEndpoint endpoint = {0};
    endpoint.prefix = "";
    endpoint.validator = (bool (*)(const char*))1; // Non-null function pointer
    endpoint.handler = (enum MHD_Result (*)(void*, struct MHD_Connection*, const char*, const char*, const char*, const char*, size_t*, void**))1;

    TEST_ASSERT_TRUE(register_web_endpoint(&endpoint));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_register_web_endpoint_null_endpoint);
    RUN_TEST(test_register_web_endpoint_null_prefix);
    RUN_TEST(test_register_web_endpoint_null_validator);
    RUN_TEST(test_register_web_endpoint_null_handler);
    RUN_TEST(test_register_web_endpoint_valid_endpoint);
    RUN_TEST(test_register_web_endpoint_duplicate_prefix);
    RUN_TEST(test_register_web_endpoint_max_endpoints);
    RUN_TEST(test_register_web_endpoint_empty_prefix);

    return UNITY_END();
}
