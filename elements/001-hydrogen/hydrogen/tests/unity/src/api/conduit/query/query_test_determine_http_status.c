/*
 * Unity Test File: determine_http_status
 * This file contains unit tests for determine_http_status function in query.c
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/database_pending.h"

// Include source header
#include "../../../../../src/api/conduit/query/query.h"

// Forward declaration for the function under test

// Mock for pending_result_is_timed_out
static bool mock_pending_result_is_timed_out(const PendingQueryResult* pending);
#define pending_result_is_timed_out mock_pending_result_is_timed_out

// Mock implementation
static bool mock_is_timed_out = false;

static bool mock_pending_result_is_timed_out(const PendingQueryResult* pending) {
    (void)pending;
    return mock_is_timed_out;
}

void setUp(void) {
    // Reset mocks
    mock_is_timed_out = false;
    (void)mock_pending_result_is_timed_out(NULL);
}

void tearDown(void) {
    // Clean up
}

// Test timeout case
static void test_determine_http_status_timeout(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* dummy_result = (const QueryResult*)0x1;
    mock_is_timed_out = true;

    unsigned int status = determine_http_status(dummy_pending, dummy_result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_REQUEST_TIMEOUT, status);
}

// Test database error case
static void test_determine_http_status_database_error(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* result = (const QueryResult*)0x1;
    mock_is_timed_out = false;

    unsigned int status = determine_http_status(dummy_pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, status);
}

// Test general bad request case (no timeout, no error_message)
static void test_determine_http_status_general_failure(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* result = (const QueryResult*)0x1; // No error_message
    mock_is_timed_out = false;

    unsigned int status = determine_http_status(dummy_pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);
}

// Test with NULL result
static void test_determine_http_status_null_result(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* result = NULL;
    mock_is_timed_out = false;

    unsigned int status = determine_http_status(dummy_pending, result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);
}

// Test with NULL pending (should fall to bad request)
static void test_determine_http_status_null_pending(void) {
    PendingQueryResult* pending = NULL;
    const QueryResult* dummy_result = (const QueryResult*)0x1;
    mock_is_timed_out = false; // Won't be called

    unsigned int status = determine_http_status(pending, dummy_result);

    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, status);
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