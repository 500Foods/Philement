/*
 * Unity Test File: Auth Query validate_jwt_from_header Direct Tests
 * This file contains direct unit tests for the validate_jwt_from_header function
 * in src/api/conduit/auth_query/auth_query.c
 *
 * These tests directly call validate_jwt_from_header to ensure all error paths
 * in the JWT validation switch statement are covered.
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of direct JWT validation tests
 *             Tests validate_jwt_from_header function directly for maximum coverage
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/auth_query/auth_query.h>

// Function prototypes for test functions
void test_validate_jwt_null_parameters(void);
void test_validate_jwt_null_connection(void);
void test_validate_jwt_null_jwt_result(void);
void test_validate_jwt_revoked_error_message(void);
void test_validate_jwt_invalid_signature_error_message(void);
void test_validate_jwt_invalid_format_error_message(void);
void test_validate_jwt_not_yet_valid_error_message(void);
void test_validate_jwt_unsupported_algorithm_error_message(void);
void test_validate_jwt_none_error_message(void);


// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

// Helper to setup mock JWT validation result
static void setup_jwt_result(bool valid, jwt_error_t error, const char* database) {
    jwt_validation_result_t result = {0};
    result.valid = valid;
    result.error = error;
    
    if (database) {
        result.claims = calloc(1, sizeof(jwt_claims_t));
        if (result.claims) {
            result.claims->database = strdup(database);
            result.claims->username = strdup("testuser");
            result.claims->user_id = 123;
        }
    }
    
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->database);
        free(result.claims->username);
        free(result.claims);
    }
}

// Test NULL parameters - both NULL
void test_validate_jwt_null_parameters(void) {
    enum MHD_Result result = validate_jwt_from_header(NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test NULL connection only
void test_validate_jwt_null_connection(void) {
    jwt_validation_result_t *jwt_result = NULL;
    enum MHD_Result result = validate_jwt_from_header(NULL, &jwt_result);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test NULL jwt_result only
void test_validate_jwt_null_jwt_result(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    enum MHD_Result result = validate_jwt_from_header(mock_connection, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test JWT_ERROR_REVOKED specifically to cover lines 192-193
void test_validate_jwt_revoked_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return revoked token
    setup_jwt_result(false, JWT_ERROR_REVOKED, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.revoked.token");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test JWT_ERROR_INVALID_SIGNATURE specifically to cover lines 195-196
void test_validate_jwt_invalid_signature_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return invalid signature
    setup_jwt_result(false, JWT_ERROR_INVALID_SIGNATURE, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.bad.signature");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test JWT_ERROR_INVALID_FORMAT specifically
void test_validate_jwt_invalid_format_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return invalid format
    setup_jwt_result(false, JWT_ERROR_INVALID_FORMAT, NULL);
    
    mock_mhd_set_lookup_result("Bearer invalid.token.format");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test JWT_ERROR_NOT_YET_VALID specifically
void test_validate_jwt_not_yet_valid_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return not yet valid
    setup_jwt_result(false, JWT_ERROR_NOT_YET_VALID, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.future.token");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test JWT_ERROR_UNSUPPORTED_ALGORITHM specifically
void test_validate_jwt_unsupported_algorithm_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return unsupported algorithm
    setup_jwt_result(false, JWT_ERROR_UNSUPPORTED_ALGORITHM, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.unsupported.token");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test JWT_ERROR_NONE (should hit default case) to cover lines 201, 205-206
void test_validate_jwt_none_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return invalid with error NONE (hits default case in switch)
    setup_jwt_result(false, JWT_ERROR_NONE, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.none.error");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(jwt_result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_jwt_null_parameters);
    RUN_TEST(test_validate_jwt_null_connection);
    RUN_TEST(test_validate_jwt_null_jwt_result);
    RUN_TEST(test_validate_jwt_revoked_error_message);
    RUN_TEST(test_validate_jwt_invalid_signature_error_message);
    RUN_TEST(test_validate_jwt_invalid_format_error_message);
    RUN_TEST(test_validate_jwt_not_yet_valid_error_message);
    RUN_TEST(test_validate_jwt_unsupported_algorithm_error_message);
    RUN_TEST(test_validate_jwt_none_error_message);
    
    return UNITY_END();
}
