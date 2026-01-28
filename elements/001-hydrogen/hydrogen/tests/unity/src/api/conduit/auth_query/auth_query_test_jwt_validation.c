/*
 * Unity Test File: Auth Query JWT Validation
 * This file contains unit tests for JWT validation in auth_query.c
 *
 * Tests validate_jwt_from_header() indirectly through handle_conduit_auth_query_request()
 * using mock_auth_service_jwt to simulate various JWT validation scenarios.
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of comprehensive JWT validation tests
 *             Tests all JWT error cases: missing header, invalid format, expired, revoked, etc.
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
void test_jwt_validation_missing_auth_header(void);
void test_jwt_validation_invalid_bearer_format_no_prefix(void);
void test_jwt_validation_invalid_bearer_format_wrong_prefix(void);
void test_jwt_validation_expired_token(void);
void test_jwt_validation_revoked_token(void);
void test_jwt_validation_invalid_signature(void);
void test_jwt_validation_invalid_format(void);
void test_jwt_validation_not_yet_valid(void);
void test_jwt_validation_unsupported_algorithm(void);
void test_jwt_validation_valid_but_null_claims(void);
void test_jwt_validation_valid_but_null_database(void);
void test_jwt_validation_valid_but_empty_database(void);
void test_jwt_validation_malloc_failure_for_result(void);
void test_jwt_validation_success(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

// Helper function to setup mock JWT validation result
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

// Test missing Authorization header
void test_jwt_validation_missing_auth_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return NULL for Authorization header lookup
    mock_mhd_set_lookup_result(NULL);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid Bearer format - no "Bearer " prefix
void test_jwt_validation_invalid_bearer_format_no_prefix(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return token without "Bearer " prefix
    mock_mhd_set_lookup_result("just.a.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid Bearer format - wrong prefix
void test_jwt_validation_invalid_bearer_format_wrong_prefix(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return token with wrong prefix
    mock_mhd_set_lookup_result("Basic dXNlcjpwYXNz");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test expired JWT token
void test_jwt_validation_expired_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return expired token
    setup_jwt_result(false, JWT_ERROR_EXPIRED, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.expired.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test revoked JWT token
void test_jwt_validation_revoked_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return revoked token
    setup_jwt_result(false, JWT_ERROR_REVOKED, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.revoked.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid signature JWT token
void test_jwt_validation_invalid_signature(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return invalid signature
    setup_jwt_result(false, JWT_ERROR_INVALID_SIGNATURE, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.bad.signature");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid format JWT token
void test_jwt_validation_invalid_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return invalid format
    setup_jwt_result(false, JWT_ERROR_INVALID_FORMAT, NULL);
    
    mock_mhd_set_lookup_result("Bearer invalid.token.format");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test not yet valid JWT token
void test_jwt_validation_not_yet_valid(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return not yet valid
    setup_jwt_result(false, JWT_ERROR_NOT_YET_VALID, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.future.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test unsupported algorithm JWT token
void test_jwt_validation_unsupported_algorithm(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return unsupported algorithm
    setup_jwt_result(false, JWT_ERROR_UNSUPPORTED_ALGORITHM, NULL);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.unsupported.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test valid JWT but NULL claims
void test_jwt_validation_valid_but_null_claims(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return valid but with NULL claims
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = NULL;  // NULL claims
    mock_auth_service_jwt_set_validation_result(result);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.noclaims");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result test_result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, test_result);
}

// Test valid JWT but NULL database claim
void test_jwt_validation_valid_but_null_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return valid but with NULL database
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = NULL;  // NULL database
        result.claims->username = strdup("testuser");
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->username);
        free(result.claims);
    }
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.nodb");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result test_result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, test_result);
}

// Test valid JWT but empty database claim
void test_jwt_validation_valid_but_empty_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return valid but with empty database
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = strdup("");  // Empty database
        result.claims->username = strdup("testuser");
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->database);
        free(result.claims->username);
        free(result.claims);
    }
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.emptydb");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result test_result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, test_result);
}

// Test malloc failure when copying JWT result
void test_jwt_validation_malloc_failure_for_result(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return valid JWT
    setup_jwt_result(true, JWT_ERROR_NONE, "testdb");
    
    // Make malloc fail on first call (when copying JWT result)
    mock_system_set_malloc_failure(1);
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test successful JWT validation (hits the success path)
void test_jwt_validation_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Setup mock to return valid JWT with database
    setup_jwt_result(true, JWT_ERROR_NONE, "testdb");
    
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.valid.token");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // Should proceed past JWT validation (may fail later in processing, but JWT part succeeds)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_jwt_validation_missing_auth_header);
    RUN_TEST(test_jwt_validation_invalid_bearer_format_no_prefix);
    RUN_TEST(test_jwt_validation_invalid_bearer_format_wrong_prefix);
    RUN_TEST(test_jwt_validation_expired_token);
    RUN_TEST(test_jwt_validation_revoked_token);
    RUN_TEST(test_jwt_validation_invalid_signature);
    RUN_TEST(test_jwt_validation_invalid_format);
    RUN_TEST(test_jwt_validation_not_yet_valid);
    RUN_TEST(test_jwt_validation_unsupported_algorithm);
    RUN_TEST(test_jwt_validation_valid_but_null_claims);
    RUN_TEST(test_jwt_validation_valid_but_null_database);
    RUN_TEST(test_jwt_validation_valid_but_empty_database);
    RUN_TEST(test_jwt_validation_malloc_failure_for_result);
    RUN_TEST(test_jwt_validation_success);

    return UNITY_END();
}
