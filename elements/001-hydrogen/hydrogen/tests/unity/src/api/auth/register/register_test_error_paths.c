/*
  * Unity Unit Tests for register.c - Error Path Testing
  *
  * Tests error conditions and failure paths in the register endpoint
  *
  * CHANGELOG:
  * 2026-01-15: Initial version - Tests for register error paths using mocks
  * 2026-07-24: Converted to non-weak mock_ prefixed functions for proper
  *             linker resolution via #define redirects in register.c,
  *             added password storage failure and success path tests
  * 2026-07-24: Migrated mock implementations to shared mock_api_utils and
  *             mock_auth_service libraries to resolve linker conflicts
  *
  * TEST_VERSION: 1.2.0
  */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service.h>

#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/register/register.h>

// Function prototypes for delegate functions we're testing
void test_handle_post_auth_register_api_buffer_error(void);
void test_handle_post_auth_register_api_buffer_method_error(void);
void test_handle_post_auth_register_empty_request_body(void);
void test_handle_post_auth_register_invalid_json(void);
void test_handle_post_auth_register_missing_required_parameters(void);
void test_handle_post_auth_register_validation_failed(void);
void test_handle_post_auth_register_api_key_verification_failed(void);
void test_handle_post_auth_register_license_expired(void);
void test_handle_post_auth_register_username_not_available(void);
void test_handle_post_auth_register_create_account_failed(void);
void test_handle_post_auth_register_password_hash_failed(void);
void test_handle_post_auth_register_password_store_failed(void);
void test_handle_post_auth_register_success(void);

// ============================================================================
// Helper Functions
// ============================================================================

void reset_all_mocks(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    reset_all_mocks();
}

void tearDown(void) {
    reset_all_mocks();
}

void reset_all_mocks(void) {
    mock_api_utils_reset_all();
    mock_auth_service_reset_all();
    mock_auth_service_set_create_account_record_result(123);
    mock_mhd_reset_all();
}

// ============================================================================
// Test Functions - Error Paths
// ============================================================================

void test_handle_post_auth_register_api_buffer_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_api_buffer_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_METHOD_ERROR);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "GET", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_empty_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data(NULL);
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_invalid_json(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("invalid json");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_missing_required_parameters(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_validation_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_validate_registration_input_result(false);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_api_key_verification_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"invalid_key\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_verify_api_key_result(false);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_license_expired(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_check_license_expiry_result(false);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_username_not_available(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"existinguser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_check_username_availability_result(false);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_create_account_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_create_account_record_result(0);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_post_auth_register_password_hash_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_compute_password_hash_result(NULL);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Password hash computed but execute_auth_query fails (lines 219-223)
void test_handle_post_auth_register_password_store_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_compute_password_hash_result("hashed_password");
    mock_auth_service_set_execute_auth_query_success(false);
    mock_auth_service_set_execute_auth_query_error("Database connection lost");

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Successful registration - all steps pass (lines 229-249)
void test_handle_post_auth_register_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_data("{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\",\"full_name\":\"Test User\"}");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    mock_auth_service_set_compute_password_hash_result("hashed_password");
    mock_auth_service_set_execute_auth_query_success(true);

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_post_auth_register_api_buffer_error);
    RUN_TEST(test_handle_post_auth_register_api_buffer_method_error);
    RUN_TEST(test_handle_post_auth_register_empty_request_body);
    RUN_TEST(test_handle_post_auth_register_invalid_json);
    RUN_TEST(test_handle_post_auth_register_missing_required_parameters);
    RUN_TEST(test_handle_post_auth_register_validation_failed);
    RUN_TEST(test_handle_post_auth_register_api_key_verification_failed);
    RUN_TEST(test_handle_post_auth_register_license_expired);
    RUN_TEST(test_handle_post_auth_register_username_not_available);
    RUN_TEST(test_handle_post_auth_register_create_account_failed);
    RUN_TEST(test_handle_post_auth_register_password_hash_failed);
    RUN_TEST(test_handle_post_auth_register_password_store_failed);
    RUN_TEST(test_handle_post_auth_register_success);

    return UNITY_END();
}
