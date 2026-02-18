/*
 * Unity Test File: Alt Queries Validate JWT for Auth
 * This file contains unit tests for the validate_jwt_for_auth_alt function
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for validate_jwt_for_auth_alt
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
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/alt_queries/alt_queries.h>

// Function prototypes for test functions
void test_validate_jwt_for_auth_alt_null_token(void);
void test_validate_jwt_for_auth_alt_invalid_jwt(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test validate_jwt_for_auth_alt with NULL token
void test_validate_jwt_for_auth_alt_null_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth_alt(mock_connection, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test validate_jwt_for_auth_alt with invalid JWT token
void test_validate_jwt_for_auth_alt_invalid_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *token = "invalid_token";

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth_alt(mock_connection, token);

    // Should return YES because MHD response was queued successfully
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_jwt_for_auth_alt_null_token);
    RUN_TEST(test_validate_jwt_for_auth_alt_invalid_jwt);

    return UNITY_END();
}
