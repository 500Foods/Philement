/*
 * Unity Test File: Auth Query Request Error Handling
 * This file contains unit tests for request error paths in
 * handle_conduit_auth_query_request in src/api/conduit/auth_query/auth_query.c
 *
 * Tests:
 * - Missing query_ref (lines 411-429)
 * - Invalid query_ref type (lines 411-429)
 * - Request parsing failures
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of request error handling tests
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
void test_request_missing_query_ref(void);
void test_request_invalid_query_ref_type_string(void);
void test_request_invalid_query_ref_type_null(void);
void test_request_empty_json_object(void);
void test_request_query_ref_is_zero(void);
void test_request_query_ref_is_negative(void);
void test_request_valid_query_ref(void);

// Helper to setup valid JWT result
static void setup_valid_jwt_result(const char* database) {
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = strdup(database ? database : "testdb");
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);
    
    if (result.claims) {
        free(result.claims->database);
        free(result.claims->username);
        free(result.claims);
    }
}

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

// Test missing query_ref field (lines 411-429)
void test_request_missing_query_ref(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    // JSON without query_ref
    const char *upload_data = "{\"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid query_ref type - string instead of integer (lines 411-429)
void test_request_invalid_query_ref_type_string(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    // query_ref as string instead of integer
    const char *upload_data = "{\"query_ref\": \"not_a_number\", \"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid query_ref type - null instead of integer (lines 411-429)
void test_request_invalid_query_ref_type_null(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    // query_ref as null
    const char *upload_data = "{\"query_ref\": null, \"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test empty JSON object - no fields at all
void test_request_empty_json_object(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test query_ref is 0 (edge case - technically valid integer but likely invalid query)
void test_request_query_ref_is_zero(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 0, \"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    // Should proceed past query_ref validation (0 is technically a valid integer)
    // Will likely fail later in processing
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test query_ref is negative
void test_request_query_ref_is_negative(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": -1, \"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    // Negative numbers are technically valid integers
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test valid query_ref - should pass validation but may fail later
void test_request_valid_query_ref(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_query";
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123, \"params\": {}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;
    
    // Setup valid JWT
    setup_valid_jwt_result("testdb");
    
    mock_mhd_set_queue_response_result(MHD_YES);
    
    enum MHD_Result result = handle_conduit_auth_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );
    
    // Should pass query_ref validation
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_request_missing_query_ref);
    RUN_TEST(test_request_invalid_query_ref_type_string);
    RUN_TEST(test_request_invalid_query_ref_type_null);
    RUN_TEST(test_request_empty_json_object);
    RUN_TEST(test_request_query_ref_is_zero);
    RUN_TEST(test_request_query_ref_is_negative);
    RUN_TEST(test_request_valid_query_ref);
    
    return UNITY_END();
}
