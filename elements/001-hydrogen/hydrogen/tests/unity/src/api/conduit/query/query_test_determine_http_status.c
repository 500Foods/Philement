/*
 * Unity Test File: determine_http_status
 * This file contains unit tests for determine_http_status function
 * in src/api/conduit/helpers/query_execution.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Function prototypes
void test_determine_http_status_database_error(void);
void test_determine_http_status_bad_request(void);
void test_determine_http_status_null_result(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test database error case - returns 422 Unprocessable Entity
void test_determine_http_status_database_error(void) {
    PendingQueryResult pending = {0};
    QueryResult result = {0};

    result.error_message = (char*)"Some database error";
    unsigned int status = determine_http_status(&pending, &result);
    TEST_ASSERT_EQUAL(422, status);  // Unprocessable Entity for database errors
}

// Test bad request case - returns 400 Bad Request
void test_determine_http_status_bad_request(void) {
    PendingQueryResult pending = {0};
    QueryResult result = {0};

    // No error message, not timed out
    unsigned int status = determine_http_status(&pending, &result);
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, status);
}

// Test with NULL result - returns 400 Bad Request
void test_determine_http_status_null_result(void) {
    PendingQueryResult pending = {0};

    unsigned int status = determine_http_status(&pending, NULL);
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, status);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_determine_http_status_database_error);
    RUN_TEST(test_determine_http_status_bad_request);
    RUN_TEST(test_determine_http_status_null_result);

    return UNITY_END();
}