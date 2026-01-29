/*
 * Unity Test File: Auth Query JWT Claims Validation
 * This file contains unit tests for JWT claims validation paths in
 * validate_jwt_from_header in src/api/conduit/auth_query/auth_query.c
 *
 * Tests:
 * - Valid JWT but NULL claims (lines 231-246)
 * - Valid JWT but NULL database claim (lines 250-264)
 * - Valid JWT but empty database claim (lines 269-284)
 * - Malloc failure for JWT result copy (lines 292-307)
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of JWT claims validation tests
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
void test_jwt_valid_null_claims_direct(void);
void test_jwt_valid_null_database_direct(void);
void test_jwt_valid_empty_database_direct(void);
void test_jwt_malloc_failure_direct(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
    
    // Default MHD mocks
    mock_mhd_set_lookup_result("Bearer valid.token.here");
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

// Test valid JWT but NULL claims (lines 231-246)
void test_jwt_valid_null_claims_direct(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return valid JWT with NULL claims
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = NULL;  // NULL claims
    mock_auth_service_jwt_set_validation_result(result);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.noclaims");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result test_result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, test_result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test valid JWT but NULL database claim (lines 250-264)
void test_jwt_valid_null_database_direct(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return valid JWT with NULL database
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = NULL;  // NULL database
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->username);
        free(result.claims);
    }
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.nodb");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result test_result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, test_result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test valid JWT but empty database claim (lines 269-284)
void test_jwt_valid_empty_database_direct(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return valid JWT with empty database
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = strdup("");  // Empty database
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->database);
        free(result.claims->username);
        free(result.claims);
    }
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.emptydb");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result test_result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, test_result);
    TEST_ASSERT_NULL(jwt_result);
}

// Test malloc failure when copying JWT result (lines 292-307)
void test_jwt_malloc_failure_direct(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    jwt_validation_result_t *jwt_result = NULL;
    
    // Setup mock to return valid JWT with database
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = strdup("testdb");
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->database);
        free(result.claims->username);
        free(result.claims);
    }
    
    // Make malloc fail on first call (when copying JWT result)
    mock_system_set_malloc_failure(1);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.token");
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result test_result = validate_jwt_from_header(mock_connection, &jwt_result);
    
    TEST_ASSERT_EQUAL(MHD_NO, test_result);
    TEST_ASSERT_NULL(jwt_result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_jwt_valid_null_claims_direct);
    RUN_TEST(test_jwt_valid_null_database_direct);
    RUN_TEST(test_jwt_valid_empty_database_direct);
    RUN_TEST(test_jwt_malloc_failure_direct);
    
    return UNITY_END();
}
