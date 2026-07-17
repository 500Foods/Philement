/*
 * Unity Test File: Conduit Status - JWT validation helper
 * Tests conduit_status_has_valid_jwt() in
 * src/api/conduit/status/status.c
 *
 * CHANGELOG:
 * 2026-07-16: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/status/status.h>

// Mock includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>

// Test function prototypes
void test_has_valid_jwt_no_header(void);
void test_has_valid_jwt_not_bearer(void);
void test_has_valid_jwt_empty_token(void);

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_mhd_set_lookup_result(NULL);
    mock_auth_service_jwt_set_validation_result((jwt_validation_result_t){false, NULL, JWT_ERROR_NONE});
}

void tearDown(void) {
    // No global state to clean
}

// No Authorization header -> no valid JWT
void test_has_valid_jwt_no_header(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result(NULL);
    TEST_ASSERT_FALSE(conduit_status_has_valid_jwt(mock_connection));
}

// Authorization header present but not "Bearer " -> no valid JWT
void test_has_valid_jwt_not_bearer(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result("Basic dXNlcjpwYXNz");
    TEST_ASSERT_FALSE(conduit_status_has_valid_jwt(mock_connection));
}

// "Bearer " prefix with empty token -> no valid JWT
void test_has_valid_jwt_empty_token(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result("Bearer ");
    TEST_ASSERT_FALSE(conduit_status_has_valid_jwt(mock_connection));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_has_valid_jwt_no_header);
    RUN_TEST(test_has_valid_jwt_not_bearer);
    RUN_TEST(test_has_valid_jwt_empty_token);

    return UNITY_END();
}
