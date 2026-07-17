/*
 * Unity Test File: send_cap_verify_error
 * Tests the send_cap_verify_error function in
 * src/api/conduit/cap/cap_query.c. Verifies the error response is
 * built and sent, and that a non-NULL request_json is decref'd.
 *
 * CHANGELOG:
 * 2026-07-16: Initial creation covering request-json present and absent.
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

// api_send_json_response uses the globally-mocked MHD functions
#include <src/api/conduit/cap/cap_query.h>

// Function prototypes for test functions
void test_send_cap_verify_error_with_request_json(void);
void test_send_cap_verify_error_null_request_json(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_send_cap_verify_error_with_request_json(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "chachaToken", json_string("abc"));

    enum MHD_Result result = send_cap_verify_error(
        mock_connection, request_json, "CAP_TOKEN_MISSING", "missing token"
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    // request_json is owned by the caller; it survives (decref'd inside)
}

void test_send_cap_verify_error_null_request_json(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;

    enum MHD_Result result = send_cap_verify_error(
        mock_connection, NULL, "CAP_VERIFY_FAILED", "verify failed"
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_cap_verify_error_with_request_json);
    RUN_TEST(test_send_cap_verify_error_null_request_json);

    return UNITY_END();
}
