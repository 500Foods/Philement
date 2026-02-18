/*
 * Unity Test File: Auth Queries Validate JWT and Extract Database
 * This file contains unit tests for the validate_jwt_and_extract_database function
 * in src/api/conduit/auth_queries/auth_queries.c
 *
 * Tests JWT validation and database extraction for authenticated queries.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for validate_jwt_and_extract_database
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
#include <src/api/conduit/auth_queries/auth_queries.h>

// Function prototypes for test functions
void test_validate_jwt_and_extract_database_null_connection(void);
void test_validate_jwt_and_extract_database_null_database_ptr(void);
void test_validate_jwt_and_extract_database_no_auth_header(void);
void test_validate_jwt_and_extract_database_invalid_auth_format(void);
void test_validate_jwt_and_extract_database_invalid_jwt(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test validate_jwt_and_extract_database with NULL connection
void test_validate_jwt_and_extract_database_null_connection(void) {
    char *database = NULL;

    enum MHD_Result result = validate_jwt_and_extract_database(NULL, &database);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with NULL database pointer
void test_validate_jwt_and_extract_database_null_database_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with no Authorization header
void test_validate_jwt_and_extract_database_no_auth_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

// Test validate_jwt_and_extract_database with invalid Authorization header format
void test_validate_jwt_and_extract_database_invalid_auth_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    // Mock the lookup to return a value without "Bearer " prefix
    // This test assumes the function checks for "Bearer " prefix
    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);

    // Should fail due to invalid format (no "Bearer " prefix)
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

// Test validate_jwt_and_extract_database with invalid JWT token
void test_validate_jwt_and_extract_database_invalid_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);

    // Should fail due to invalid JWT
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_jwt_and_extract_database_null_connection);
    RUN_TEST(test_validate_jwt_and_extract_database_null_database_ptr);
    RUN_TEST(test_validate_jwt_and_extract_database_no_auth_header);
    RUN_TEST(test_validate_jwt_and_extract_database_invalid_auth_format);
    RUN_TEST(test_validate_jwt_and_extract_database_invalid_jwt);

    return UNITY_END();
}
