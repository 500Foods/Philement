/*
 * Unity Test File: determine_http_status
 * This file contains unit tests for determine_http_status function in query.c
 */

// Standard includes
#include <stdlib.h>
#include <string.h>

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database_pending.h>
#include <src/database/database_types.h>

// Include source header
#include <src/api/conduit/query/query.h>

void setUp(void);
void tearDown(void);

// Helper to create dummy QueryResult
static QueryResult* create_dummy_query_result(bool success, bool has_error, const char* error_msg) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) return NULL;

    result->success = success;
    if (has_error && error_msg) {
        result->error_message = strdup(error_msg);
    }
    return result;
}

// Helper to create dummy PendingQueryResult
static PendingQueryResult* create_dummy_pending(bool timed_out) {
    PendingQueryResult* pending = calloc(1, sizeof(PendingQueryResult));
    if (!pending) return NULL;

    pending->timed_out = timed_out;
    return pending;
}

void setUp(void) {
    // No setup needed for real implementations
}

void tearDown(void) {
    // No cleanup needed
}

// Test timeout case
static void test_determine_http_status_timeout(void) {
    PendingQueryResult* pending = create_dummy_pending(true);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(true, false, NULL);
    TEST_ASSERT_NOT_NULL(result);

    unsigned int status = determine_http_status(pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_REQUEST_TIMEOUT, status);

    free(pending);
    free(result->error_message);
    free(result);
}

// Test database error case
static void test_determine_http_status_database_error(void) {
    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(false, true, "Database connection failed");
    TEST_ASSERT_NOT_NULL(result);

    unsigned int status = determine_http_status(pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, status);

    free(pending);
    free(result->error_message);
    free(result);
}

// Test general bad request case (no timeout, no error_message)
static void test_determine_http_status_general_failure(void) {
    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(false, false, NULL);
    TEST_ASSERT_NOT_NULL(result);

    unsigned int status = determine_http_status(pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);

    free(pending);
    free(result);
}

// Test with NULL result
static void test_determine_http_status_null_result(void) {
    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    const QueryResult* result = NULL;

    unsigned int status = determine_http_status(pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);

    free(pending);
}

// Test with NULL pending (should fall to bad request)
static void test_determine_http_status_null_pending(void) {
    const PendingQueryResult* pending = NULL;

    QueryResult* result = create_dummy_query_result(false, false, NULL);
    TEST_ASSERT_NOT_NULL(result);

    unsigned int status = determine_http_status(pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_determine_http_status_timeout);
    RUN_TEST(test_determine_http_status_database_error);
    RUN_TEST(test_determine_http_status_general_failure);
    RUN_TEST(test_determine_http_status_null_result);
    RUN_TEST(test_determine_http_status_null_pending);

    return UNITY_END();
}