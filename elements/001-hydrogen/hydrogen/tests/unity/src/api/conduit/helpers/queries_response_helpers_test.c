/*
 * Unity Test File: Queries Response Helpers
 * This file contains unit tests for the queries_response_helpers functions
 * in src/api/conduit/helpers/queries_response_helpers.c
 *
 * Tests the shared response building helper functions used by
 * queries, auth_queries, and alt_queries endpoints.
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for MHD (needed for send_conduit_error_response tests)
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source header
#include <src/api/conduit/helpers/queries_response_helpers.h>

// Function prototypes for test functions
void test_build_dedup_error_json_rate_limit(void);
void test_build_dedup_error_json_database_not_found(void);
void test_build_dedup_error_json_generic_error(void);
void test_build_dedup_error_json_ok_code(void);
void test_get_dedup_http_status_rate_limit(void);
void test_get_dedup_http_status_other(void);
void test_determine_queries_http_status_null_array(void);
void test_determine_queries_http_status_no_errors(void);
void test_determine_queries_http_status_rate_limit(void);
void test_determine_queries_http_status_parameter_error(void);
void test_determine_queries_http_status_auth_error(void);
void test_determine_queries_http_status_not_found_error(void);
void test_determine_queries_http_status_database_error(void);
void test_determine_queries_http_status_duplicate_only(void);
void test_determine_queries_http_status_mixed_errors(void);
void test_build_rate_limit_result_entry(void);
void test_build_duplicate_result_entry(void);
void test_build_invalid_mapping_result_entry(void);
void test_send_conduit_error_response_basic(void);
void test_send_conduit_error_response_null_msg(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// === build_dedup_error_json tests ===

void test_build_dedup_error_json_rate_limit(void) {
    json_t *response = build_dedup_error_json(DEDUP_RATE_LIMIT, "testdb", 10);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(response, "success")));
    TEST_ASSERT_EQUAL_STRING("Rate limit exceeded", json_string_value(json_object_get(response, "error")));
    TEST_ASSERT_NOT_NULL(json_object_get(response, "message"));
    json_decref(response);
}

void test_build_dedup_error_json_database_not_found(void) {
    json_t *response = build_dedup_error_json(DEDUP_DATABASE_NOT_FOUND, "baddb", 10);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_EQUAL_STRING("Invalid database", json_string_value(json_object_get(response, "error")));
    json_decref(response);
}

void test_build_dedup_error_json_generic_error(void) {
    json_t *response = build_dedup_error_json(DEDUP_ERROR, "testdb", 10);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_EQUAL_STRING("Validation failed", json_string_value(json_object_get(response, "error")));
    json_decref(response);
}

void test_build_dedup_error_json_ok_code(void) {
    json_t *response = build_dedup_error_json(DEDUP_OK, "testdb", 10);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_EQUAL_STRING("Validation failed", json_string_value(json_object_get(response, "error")));
    json_decref(response);
}

// === get_dedup_http_status tests ===

void test_get_dedup_http_status_rate_limit(void) {
    TEST_ASSERT_EQUAL(MHD_HTTP_TOO_MANY_REQUESTS, get_dedup_http_status(DEDUP_RATE_LIMIT));
}

void test_get_dedup_http_status_other(void) {
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, get_dedup_http_status(DEDUP_DATABASE_NOT_FOUND));
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, get_dedup_http_status(DEDUP_ERROR));
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, get_dedup_http_status(DEDUP_OK));
}

// === determine_queries_http_status tests ===

void test_determine_queries_http_status_null_array(void) {
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, determine_queries_http_status(NULL, 0));
}

void test_determine_queries_http_status_no_errors(void) {
    json_t *results = json_array();
    json_t *ok_result = json_object();
    json_object_set_new(ok_result, "success", json_true());
    json_array_append_new(results, ok_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_OK, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_rate_limit(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Rate limit exceeded"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_TOO_MANY_REQUESTS, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_parameter_error(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Parameter validation failed"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_auth_error(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Unauthorized access"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_not_found_error(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Not found"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_FOUND, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_database_error(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Database execution error"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_UNPROCESSABLE_ENTITY, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_duplicate_only(void) {
    json_t *results = json_array();
    json_t *error_result = json_object();
    json_object_set_new(error_result, "error", json_string("Duplicate query"));
    json_array_append_new(results, error_result);

    TEST_ASSERT_EQUAL(MHD_HTTP_OK, determine_queries_http_status(results, 1));
    json_decref(results);
}

void test_determine_queries_http_status_mixed_errors(void) {
    json_t *results = json_array();

    // Add rate limit error (highest priority)
    json_t *rate_error = json_object();
    json_object_set_new(rate_error, "error", json_string("Rate limit exceeded"));
    json_array_append_new(results, rate_error);

    // Add parameter error (lower priority)
    json_t *param_error = json_object();
    json_object_set_new(param_error, "error", json_string("Parameter validation failed"));
    json_array_append_new(results, param_error);

    // Rate limit takes precedence
    TEST_ASSERT_EQUAL(MHD_HTTP_TOO_MANY_REQUESTS, determine_queries_http_status(results, 2));
    json_decref(results);
}

// === Result entry builder tests ===

void test_build_rate_limit_result_entry(void) {
    json_t *entry = build_rate_limit_result_entry(10);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(entry, "success")));
    TEST_ASSERT_EQUAL_STRING("Rate limit exceeded", json_string_value(json_object_get(entry, "error")));
    TEST_ASSERT_NOT_NULL(json_object_get(entry, "message"));
    json_decref(entry);
}

void test_build_duplicate_result_entry(void) {
    json_t *entry = build_duplicate_result_entry();
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(entry, "success")));
    TEST_ASSERT_EQUAL_STRING("Duplicate query", json_string_value(json_object_get(entry, "error")));
    json_decref(entry);
}

void test_build_invalid_mapping_result_entry(void) {
    json_t *entry = build_invalid_mapping_result_entry();
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(entry, "success")));
    TEST_ASSERT_EQUAL_STRING("Internal error: invalid query mapping", json_string_value(json_object_get(entry, "error")));
    json_decref(entry);
}

// === send_conduit_error_response tests ===

void test_send_conduit_error_response_basic(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = send_conduit_error_response(
        mock_connection, "Test error message", MHD_HTTP_BAD_REQUEST);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_conduit_error_response_null_msg(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = send_conduit_error_response(
        mock_connection, NULL, MHD_HTTP_INTERNAL_SERVER_ERROR);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    // build_dedup_error_json tests
    RUN_TEST(test_build_dedup_error_json_rate_limit);
    RUN_TEST(test_build_dedup_error_json_database_not_found);
    RUN_TEST(test_build_dedup_error_json_generic_error);
    RUN_TEST(test_build_dedup_error_json_ok_code);

    // get_dedup_http_status tests
    RUN_TEST(test_get_dedup_http_status_rate_limit);
    RUN_TEST(test_get_dedup_http_status_other);

    // determine_queries_http_status tests
    RUN_TEST(test_determine_queries_http_status_null_array);
    RUN_TEST(test_determine_queries_http_status_no_errors);
    RUN_TEST(test_determine_queries_http_status_rate_limit);
    RUN_TEST(test_determine_queries_http_status_parameter_error);
    RUN_TEST(test_determine_queries_http_status_auth_error);
    RUN_TEST(test_determine_queries_http_status_not_found_error);
    RUN_TEST(test_determine_queries_http_status_database_error);
    RUN_TEST(test_determine_queries_http_status_duplicate_only);
    RUN_TEST(test_determine_queries_http_status_mixed_errors);

    // Result entry builder tests
    RUN_TEST(test_build_rate_limit_result_entry);
    RUN_TEST(test_build_duplicate_result_entry);
    RUN_TEST(test_build_invalid_mapping_result_entry);
    RUN_TEST(test_send_conduit_error_response_basic);
    RUN_TEST(test_send_conduit_error_response_null_msg);

    return UNITY_END();
}
