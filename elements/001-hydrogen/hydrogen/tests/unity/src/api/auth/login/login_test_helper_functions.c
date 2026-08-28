/*
  * Unity Test File: Login Helper Functions
  * This file contains unit tests for the error response helper functions in login.c
  *
  * Tests: login_send_*_error() - Error response building functions
  *
  * CHANGELOG:
  * 2026-01-12: Initial version - Tests for refactored error helper functions
  *
  * TEST_VERSION: 1.0.0
  */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

#include <src/api/auth/login/login.h>
#include <src/api/api_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_login_send_missing_params_error(void);
void test_login_send_validation_error(void);
void test_login_send_license_expired_error(void);
void test_login_send_client_ip_error(void);
void test_login_send_ip_blacklist_error(void);
void test_login_send_rate_limit_error(void);
void test_login_send_account_not_found_error(void);
void test_login_send_account_disabled_error(void);
void test_login_send_account_not_authorized_error(void);
void test_login_send_jwt_generation_error(void);
void test_login_send_jwt_hash_error(void);
void test_login_send_auth_unavailable_error(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
}

// ============================================================================
// Test Functions - Helper Functions
// ============================================================================

// Test: login_send_missing_params_error
void test_login_send_missing_params_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_missing_params_error(mock_connection);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_validation_error
void test_login_send_validation_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_validation_error(mock_connection, "testuser");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_license_expired_error
void test_login_send_license_expired_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_license_expired_error(mock_connection, 123);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_client_ip_error
void test_login_send_client_ip_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_client_ip_error(mock_connection);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_ip_blacklist_error
void test_login_send_ip_blacklist_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_ip_blacklist_error(mock_connection, "192.168.1.1");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_rate_limit_error
void test_login_send_rate_limit_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_rate_limit_error(mock_connection, "testuser", "192.168.1.1");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_account_not_found_error
void test_login_send_account_not_found_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_account_not_found_error(mock_connection, "testuser");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_account_disabled_error
void test_login_send_account_disabled_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_account_disabled_error(mock_connection, "testuser", 123);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_account_not_authorized_error
void test_login_send_account_not_authorized_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_account_not_authorized_error(mock_connection, "testuser", 123);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_jwt_generation_error
void test_login_send_jwt_generation_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_jwt_generation_error(mock_connection, 123);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_jwt_hash_error
void test_login_send_jwt_hash_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_jwt_hash_error(mock_connection, 123);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: login_send_auth_unavailable_error
// Returns 503 Service Unavailable with retry_after field
void test_login_send_auth_unavailable_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = login_send_auth_unavailable_error(mock_connection);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // Helper function tests
    RUN_TEST(test_login_send_missing_params_error);
    RUN_TEST(test_login_send_validation_error);
    RUN_TEST(test_login_send_license_expired_error);
    RUN_TEST(test_login_send_client_ip_error);
    RUN_TEST(test_login_send_ip_blacklist_error);
    RUN_TEST(test_login_send_rate_limit_error);
    RUN_TEST(test_login_send_account_not_found_error);
    RUN_TEST(test_login_send_account_disabled_error);
    RUN_TEST(test_login_send_account_not_authorized_error);
    RUN_TEST(test_login_send_jwt_generation_error);
    RUN_TEST(test_login_send_jwt_hash_error);
    RUN_TEST(test_login_send_auth_unavailable_error);

    return UNITY_END();
}
