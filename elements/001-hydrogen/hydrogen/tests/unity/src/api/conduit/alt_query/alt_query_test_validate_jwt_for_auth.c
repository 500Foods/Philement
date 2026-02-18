/*
 * Unity Test File: Alt Query Validate JWT for Auth
 * This file contains unit tests for the validate_jwt_for_auth function
 * in src/api/conduit/alt_query/alt_query.c
 *
 * Tests JWT validation for alternative authenticated query.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for validate_jwt_for_auth
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source headers
#include <src/api/conduit/alt_query/alt_query.h>

// Function prototypes for test functions
void test_validate_jwt_for_auth_null_token(void);
void test_validate_jwt_for_auth_invalid_jwt(void);
void test_validate_jwt_for_auth_empty_token(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test validate_jwt_for_auth with NULL token
void test_validate_jwt_for_auth_null_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth(mock_connection, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test validate_jwt_for_auth with invalid JWT token
void test_validate_jwt_for_auth_invalid_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *token = "invalid_token";

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth(mock_connection, token);

    // Function returns MHD_NO when JWT validation fails
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_for_auth with empty string token
void test_validate_jwt_for_auth_empty_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *token = "";

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth(mock_connection, token);

    // Function returns MHD_NO when JWT validation fails
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_jwt_for_auth_null_token);
    RUN_TEST(test_validate_jwt_for_auth_invalid_jwt);
    RUN_TEST(test_validate_jwt_for_auth_empty_token);

    return UNITY_END();
}
