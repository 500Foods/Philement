/*
 * Unity Unit Tests for register.c - handle_register_error function
 *
 * Tests the handle_register_error helper function which builds
 * JSON error responses for registration failures.
 *
 * CHANGELOG:
 * 2026-07-24: Initial version
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

#include <src/api/auth/register/register.h>
#include <src/api/api_utils.h>

// Forward declarations for functions being tested
enum MHD_Result handle_register_error(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status,
    json_t *request
);

// Forward declarations for test functions
void test_handle_register_error_with_null_request(void);
void test_handle_register_error_with_non_null_request(void);
void test_handle_register_error_response_structure(void);
void test_handle_register_error_different_status_codes(void);
void test_handle_register_error_empty_error_message(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    mock_api_utils_reset_all();
    mock_api_utils_set_capture_mode(true);
}

void tearDown(void) {
    mock_api_utils_reset_all();
}

// ============================================================================
// Test Functions
// ============================================================================

// Test: handle_register_error with NULL request
// Covers line 31: if (request) is false - request is not freed
void test_handle_register_error_with_null_request(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_register_error(
        mock_connection, &con_cls, "Test error", MHD_HTTP_BAD_REQUEST, NULL
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, mock_api_utils_get_captured_status());
    TEST_ASSERT_NOT_NULL(mock_api_utils_get_captured_response());

    TEST_ASSERT_TRUE(json_is_object(mock_api_utils_get_captured_response()));
    TEST_ASSERT_FALSE(json_boolean_value(json_object_get(mock_api_utils_get_captured_response(), "success")));
    TEST_ASSERT_EQUAL_STRING("Test error", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// Test: handle_register_error with non-NULL request
// Covers line 32: json_decref(request) is called
void test_handle_register_error_with_non_null_request(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    json_t *request = json_object();
    json_object_set_new(request, "username", json_string("testuser"));

    enum MHD_Result result = handle_register_error(
        mock_connection, &con_cls, "Registration failed", MHD_HTTP_CONFLICT, request
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_CONFLICT, mock_api_utils_get_captured_status());
    TEST_ASSERT_NOT_NULL(mock_api_utils_get_captured_response());

    TEST_ASSERT_TRUE(json_is_object(mock_api_utils_get_captured_response()));
    TEST_ASSERT_FALSE(json_boolean_value(json_object_get(mock_api_utils_get_captured_response(), "success")));
    TEST_ASSERT_EQUAL_STRING("Registration failed", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// Test: handle_register_error response structure verification
void test_handle_register_error_response_structure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_register_error(
        mock_connection, &con_cls, "Invalid input", MHD_HTTP_BAD_REQUEST, NULL
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(mock_api_utils_get_captured_response());

    TEST_ASSERT_TRUE(json_is_object(mock_api_utils_get_captured_response()));

    json_t *success = json_object_get(mock_api_utils_get_captured_response(), "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_FALSE(json_boolean_value(success));

    json_t *error = json_object_get(mock_api_utils_get_captured_response(), "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Invalid input", json_string_value(error));
}

// Test: handle_register_error with different HTTP status codes
void test_handle_register_error_different_status_codes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    // Test with 401 Unauthorized
    enum MHD_Result result = handle_register_error(
        mock_connection, &con_cls, "Invalid API key", MHD_HTTP_UNAUTHORIZED, NULL
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("Invalid API key", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));

    mock_api_utils_reset_capture();

    // Test with 403 Forbidden
    result = handle_register_error(
        mock_connection, &con_cls, "License has expired", MHD_HTTP_FORBIDDEN, NULL
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_FORBIDDEN, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("License has expired", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));

    mock_api_utils_reset_capture();

    // Test with 500 Internal Server Error
    result = handle_register_error(
        mock_connection, &con_cls, "Failed to create account", MHD_HTTP_INTERNAL_SERVER_ERROR, NULL
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("Failed to create account", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// Test: handle_register_error with empty error message
void test_handle_register_error_empty_error_message(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_register_error(
        mock_connection, &con_cls, "", MHD_HTTP_BAD_REQUEST, NULL
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_NOT_NULL(mock_api_utils_get_captured_response());

    TEST_ASSERT_TRUE(json_is_object(mock_api_utils_get_captured_response()));
    TEST_ASSERT_FALSE(json_boolean_value(json_object_get(mock_api_utils_get_captured_response(), "success")));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_register_error_with_null_request);
    RUN_TEST(test_handle_register_error_with_non_null_request);
    RUN_TEST(test_handle_register_error_response_structure);
    RUN_TEST(test_handle_register_error_different_status_codes);
    RUN_TEST(test_handle_register_error_empty_error_message);

    return UNITY_END();
}
