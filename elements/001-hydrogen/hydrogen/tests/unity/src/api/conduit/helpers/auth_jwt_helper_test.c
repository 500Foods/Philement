/*
 * Unity Test File: Auth JWT Helper Tests
 * This file contains unit tests for auth_jwt_helper.c functions
 *
 * These tests provide direct coverage of the helper functions without
 * relying on mocks, ensuring gcov-measurable coverage.
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of auth_jwt_helper tests
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>

// Include the helper header
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Function prototypes for test functions
void test_get_jwt_error_message_expired(void);
void test_get_jwt_error_message_revoked(void);
void test_get_jwt_error_message_invalid_signature(void);
void test_get_jwt_error_message_invalid_format(void);
void test_get_jwt_error_message_not_yet_valid(void);
void test_get_jwt_error_message_unsupported_algorithm(void);
void test_get_jwt_error_message_none(void);
void test_get_jwt_error_message_default(void);
void test_extract_and_validate_jwt_null_result(void);
void test_extract_and_validate_jwt_null_header(void);
void test_extract_and_validate_jwt_no_bearer_prefix(void);
void test_extract_and_validate_jwt_invalid_token(void);
void test_validate_jwt_claims_null_result(void);
void test_validate_jwt_claims_invalid_result(void);
void test_validate_jwt_claims_null_claims(void);
void test_validate_jwt_claims_null_database(void);
void test_validate_jwt_claims_empty_database(void);
void test_validate_jwt_claims_success(void);
void test_send_jwt_error_response(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
}

// Test get_jwt_error_message for JWT_ERROR_EXPIRED
void test_get_jwt_error_message_expired(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_EXPIRED);
    TEST_ASSERT_EQUAL_STRING("JWT token has expired", msg);
}

// Test get_jwt_error_message for JWT_ERROR_REVOKED
void test_get_jwt_error_message_revoked(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_REVOKED);
    TEST_ASSERT_EQUAL_STRING("JWT token has been revoked", msg);
}

// Test get_jwt_error_message for JWT_ERROR_INVALID_SIGNATURE
void test_get_jwt_error_message_invalid_signature(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_INVALID_SIGNATURE);
    TEST_ASSERT_EQUAL_STRING("Invalid JWT signature", msg);
}

// Test get_jwt_error_message for JWT_ERROR_INVALID_FORMAT
void test_get_jwt_error_message_invalid_format(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_INVALID_FORMAT);
    TEST_ASSERT_EQUAL_STRING("Invalid JWT format", msg);
}

// Test get_jwt_error_message for JWT_ERROR_NOT_YET_VALID
void test_get_jwt_error_message_not_yet_valid(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_NOT_YET_VALID);
    TEST_ASSERT_EQUAL_STRING("JWT token not yet valid", msg);
}

// Test get_jwt_error_message for JWT_ERROR_UNSUPPORTED_ALGORITHM
void test_get_jwt_error_message_unsupported_algorithm(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_UNSUPPORTED_ALGORITHM);
    TEST_ASSERT_EQUAL_STRING("Unsupported JWT algorithm", msg);
}

// Test get_jwt_error_message for JWT_ERROR_NONE
void test_get_jwt_error_message_none(void) {
    const char* msg = get_jwt_error_message(JWT_ERROR_NONE);
    TEST_ASSERT_EQUAL_STRING("Invalid or expired JWT token", msg);
}

// Test get_jwt_error_message for default/unknown error
void test_get_jwt_error_message_default(void) {
    const char* msg = get_jwt_error_message((jwt_error_t)999);
    TEST_ASSERT_EQUAL_STRING("Invalid or expired JWT token", msg);
}

// Test extract_and_validate_jwt with NULL result
void test_extract_and_validate_jwt_null_result(void) {
    bool result = extract_and_validate_jwt("Bearer token", NULL);
    TEST_ASSERT_FALSE(result);
}

// Test extract_and_validate_jwt with NULL header
void test_extract_and_validate_jwt_null_header(void) {
    jwt_validation_result_t jwt_result = {0};
    bool result = extract_and_validate_jwt(NULL, &jwt_result);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(jwt_result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, jwt_result.error);
}

// Test extract_and_validate_jwt without Bearer prefix
void test_extract_and_validate_jwt_no_bearer_prefix(void) {
    jwt_validation_result_t jwt_result = {0};
    bool result = extract_and_validate_jwt("Basic dXNlcjpwYXNz", &jwt_result);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(jwt_result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, jwt_result.error);
}

// Test extract_and_validate_jwt with invalid token (mock will return invalid)
void test_extract_and_validate_jwt_invalid_token(void) {
    jwt_validation_result_t jwt_result = {0};
    
    // Setup mock to return invalid result
    jwt_validation_result_t mock_result = {0};
    mock_result.valid = false;
    mock_result.error = JWT_ERROR_EXPIRED;
    mock_auth_service_jwt_set_validation_result(mock_result);
    
    bool result = extract_and_validate_jwt("Bearer invalid.token.here", &jwt_result);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(jwt_result.valid);
    
    mock_auth_service_jwt_reset_all();
}

// Test validate_jwt_claims with NULL result
void test_validate_jwt_claims_null_result(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    bool result = validate_jwt_claims(NULL, mock_connection);
    TEST_ASSERT_FALSE(result);
}

// Test validate_jwt_claims with invalid result (valid=false)
void test_validate_jwt_claims_invalid_result(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    jwt_validation_result_t jwt_result = {0};
    jwt_result.valid = false;
    
    bool result = validate_jwt_claims(&jwt_result, mock_connection);
    TEST_ASSERT_FALSE(result);
}

// Test validate_jwt_claims with NULL claims
void test_validate_jwt_claims_null_claims(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    jwt_validation_result_t jwt_result = {0};
    jwt_result.valid = true;
    jwt_result.claims = NULL;
    
    bool result = validate_jwt_claims(&jwt_result, mock_connection);
    TEST_ASSERT_FALSE(result);
}

// Test validate_jwt_claims with NULL database
void test_validate_jwt_claims_null_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    jwt_validation_result_t jwt_result = {0};
    jwt_result.valid = true;
    jwt_result.claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(jwt_result.claims);
    jwt_result.claims->database = NULL;
    
    bool result = validate_jwt_claims(&jwt_result, mock_connection);
    TEST_ASSERT_FALSE(result);
    // Note: claims are freed by the function on error
}

// Test validate_jwt_claims with empty database
void test_validate_jwt_claims_empty_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    jwt_validation_result_t jwt_result = {0};
    jwt_result.valid = true;
    jwt_result.claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(jwt_result.claims);
    jwt_result.claims->database = strdup("");
    
    bool result = validate_jwt_claims(&jwt_result, mock_connection);
    TEST_ASSERT_FALSE(result);
    // Note: claims are freed by the function on error
}

// Test validate_jwt_claims success case
void test_validate_jwt_claims_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    
    jwt_validation_result_t jwt_result = {0};
    jwt_result.valid = true;
    jwt_result.claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(jwt_result.claims);
    jwt_result.claims->database = strdup("testdb");
    
    bool result = validate_jwt_claims(&jwt_result, mock_connection);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(jwt_result.claims);
    TEST_ASSERT_EQUAL_STRING("testdb", jwt_result.claims->database);
    
    // Cleanup
    free(jwt_result.claims->database);
    free(jwt_result.claims);
}

// Test send_jwt_error_response
void test_send_jwt_error_response(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = send_jwt_error_response(mock_connection, "Test error message", MHD_HTTP_UNAUTHORIZED);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();
    
    // get_jwt_error_message tests
    RUN_TEST(test_get_jwt_error_message_expired);
    RUN_TEST(test_get_jwt_error_message_revoked);
    RUN_TEST(test_get_jwt_error_message_invalid_signature);
    RUN_TEST(test_get_jwt_error_message_invalid_format);
    RUN_TEST(test_get_jwt_error_message_not_yet_valid);
    RUN_TEST(test_get_jwt_error_message_unsupported_algorithm);
    RUN_TEST(test_get_jwt_error_message_none);
    RUN_TEST(test_get_jwt_error_message_default);
    
    // extract_and_validate_jwt tests
    RUN_TEST(test_extract_and_validate_jwt_null_result);
    RUN_TEST(test_extract_and_validate_jwt_null_header);
    RUN_TEST(test_extract_and_validate_jwt_no_bearer_prefix);
    RUN_TEST(test_extract_and_validate_jwt_invalid_token);
    
    // validate_jwt_claims tests
    RUN_TEST(test_validate_jwt_claims_null_result);
    RUN_TEST(test_validate_jwt_claims_invalid_result);
    RUN_TEST(test_validate_jwt_claims_null_claims);
    RUN_TEST(test_validate_jwt_claims_null_database);
    RUN_TEST(test_validate_jwt_claims_empty_database);
    RUN_TEST(test_validate_jwt_claims_success);
    
    // send_jwt_error_response tests
    RUN_TEST(test_send_jwt_error_response);
    
    return UNITY_END();
}
