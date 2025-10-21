/*
 * Unity Tests for Conduit Query Endpoint
 *
 * Tests the REST API endpoint for executing database queries by reference.
 * This file now tests the refactored code with smaller, testable functions.
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include the source file to test static functions
#include "../../../../../src/api/conduit/query/query.c"

// Test function prototypes
void test_query_test_conduit_handler_method_not_allowed(void);
void test_query_test_conduit_handler_invalid_json(void);
void test_query_test_conduit_handler_missing_fields(void);

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

// Test handler handles invalid JSON in POST request
void test_query_test_conduit_handler_invalid_json(void) {
    // Test POST with invalid JSON
    const char* invalid_json = "{\"query_ref\": 123, \"database\": "; // Invalid JSON
    size_t data_size = strlen(invalid_json);

    enum MHD_Result result = handle_conduit_query_request(
        test_connection, test_url, "POST", invalid_json,
        &data_size, test_con_cls);

    // Should return MHD_YES (handled) but with 400 status for bad request
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handler handles missing required fields
void test_query_test_conduit_handler_missing_fields(void) {
    // Test POST with missing query_ref
    const char* missing_field_json = "{\"database\": \"test_db\"}";
    size_t data_size = strlen(missing_field_json);

    enum MHD_Result result = handle_conduit_query_request(
        test_connection, test_url, "POST", missing_field_json,
        &data_size, test_con_cls);

    // Should return MHD_YES (handled) but with 400 status for bad request
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_test_conduit_handler_method_not_allowed);
    RUN_TEST(test_query_test_conduit_handler_invalid_json);
    RUN_TEST(test_query_test_conduit_handler_missing_fields);

    return UNITY_END();
}