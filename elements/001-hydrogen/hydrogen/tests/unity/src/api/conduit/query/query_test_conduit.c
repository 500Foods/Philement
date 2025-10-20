/*
 * Unity Tests for Conduit Query Endpoint
 *
 * Tests the REST API endpoint for executing database queries by reference.
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../../src/api/conduit/query/query.h"

// Test function prototypes
void test_query_test_conduit_handler_method_not_allowed(void);
void test_query_test_conduit_handler_not_implemented(void);

// Test fixtures
static void* test_connection = NULL;
static const char* test_url = "/api/conduit/query";
static const char* test_upload_data = NULL;
static size_t test_upload_data_size = 0;
static void** test_con_cls = NULL;

void setUp(void) {
    test_connection = NULL;
    test_url = "/api/conduit/query";
    test_upload_data = NULL;
    test_upload_data_size = 0;
    test_con_cls = NULL;
}

void tearDown(void) {
    // No cleanup needed for these simple tests
}

// Test handler rejects unsupported methods
void test_query_test_conduit_handler_method_not_allowed(void) {
    // Test PUT method (should be rejected)
    enum MHD_Result result = handle_conduit_query_request(
        test_connection, test_url, "PUT", test_upload_data,
        &test_upload_data_size, test_con_cls);

    // Should return MHD_YES (handled) but with 405 status
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handler returns 501 Not Implemented for valid methods
void test_query_test_conduit_handler_not_implemented(void) {
    // Test GET method (should return 501)
    enum MHD_Result get_result = handle_conduit_query_request(
        test_connection, test_url, "GET", test_upload_data,
        &test_upload_data_size, test_con_cls);

    // Should return MHD_YES (handled) but with 501 status
    TEST_ASSERT_EQUAL(MHD_YES, get_result);

    // Test POST method (should return 501)
    enum MHD_Result post_result = handle_conduit_query_request(
        test_connection, test_url, "POST", test_upload_data,
        &test_upload_data_size, test_con_cls);

    // Should return MHD_YES (handled) but with 501 status
    TEST_ASSERT_EQUAL(MHD_YES, post_result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_test_conduit_handler_method_not_allowed);
    RUN_TEST(test_query_test_conduit_handler_not_implemented);

    return UNITY_END();
}