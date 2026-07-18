/*
 * Unity Test File: Mail Relay Status API Endpoint
 *
 * Tests GET /api/mailrelay/status handler covering method validation, JWT
 * validation (failure, missing-database, and success paths), and the status
 * response. Uses mock MHD and JWT helpers so the test runs without a live
 * database connection.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/api/api_utils.h>
#include <src/api/mailrelay/status/status.h>
#include <src/mailrelay/mailrelay.h>

void test_status_wrong_method(void);
void test_status_missing_auth_no_claims(void);
void test_status_missing_auth_with_claims(void);
void test_status_invalid_claims(void);
void test_status_success(void);

/* Build and register a mock JWT result. When valid=true the mock takes
 * ownership of a copy of the claims, so the local allocation is freed here.
 * When valid=false the mock stores the raw pointer, so we must leave it
 * allocated (the mock frees it on reset). */
static void set_jwt_result(bool valid, jwt_error_t error, bool with_database) {
    jwt_validation_result_t result = {0};
    result.valid = valid;
    result.error = error;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        if (with_database) {
            result.claims->database = strdup("testdb");
        }
        result.claims->roles = strdup("1");
        result.claims->email = strdup("user@example.com");
        result.claims->jti = strdup("request-123");
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);

    if (valid && result.claims) {
        free(result.claims->database);
        free(result.claims->roles);
        free(result.claims->email);
        free(result.claims->jti);
        free(result.claims->username);
        free(result.claims);
    }
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    mock_mhd_set_lookup_result("Bearer valid.token.here");
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
}

void test_status_wrong_method(void) {
    struct MHD_Connection* conn = (struct MHD_Connection*)0x123;
    enum MHD_Result result = handle_mailrelay_status_request(conn, "/api/mailrelay/status",
                                                              "POST", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_status_missing_auth_no_claims(void) {
    struct MHD_Connection* conn = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result(NULL);
    /* extract_and_validate_jwt fails with no claims. */
    set_jwt_result(false, JWT_ERROR_INVALID_FORMAT, false);
    enum MHD_Result result = handle_mailrelay_status_request(conn, "/api/mailrelay/status",
                                                              "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_status_missing_auth_with_claims(void) {
    struct MHD_Connection* conn = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result(NULL);
    /* extract fails but leaves claims populated (free path is exercised). */
    set_jwt_result(false, JWT_ERROR_INVALID_FORMAT, true);
    enum MHD_Result result = handle_mailrelay_status_request(conn, "/api/mailrelay/status",
                                                              "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_status_invalid_claims(void) {
    struct MHD_Connection* conn = (struct MHD_Connection*)0x123;
    /* extract succeeds, claims present, but no database claim -> validate fails. */
    set_jwt_result(true, JWT_ERROR_NONE, false);
    enum MHD_Result result = handle_mailrelay_status_request(conn, "/api/mailrelay/status",
                                                              "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_status_success(void) {
    struct MHD_Connection* conn = (struct MHD_Connection*)0x123;
    set_jwt_result(true, JWT_ERROR_NONE, true);
    enum MHD_Result result = handle_mailrelay_status_request(conn, "/api/mailrelay/status",
                                                              "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_status_wrong_method);
    RUN_TEST(test_status_missing_auth_no_claims);
    RUN_TEST(test_status_missing_auth_with_claims);
    RUN_TEST(test_status_invalid_claims);
    RUN_TEST(test_status_success);

    return UNITY_END();
}
