/*
 * Unity Test File: Auth Query Handle Buffer Result
 * This file contains unit tests for the handle_auth_query_buffer_result function
 * in src/api/conduit/auth_query/auth_query.c
 *
 * This function handles the result of api_buffer_post_data and returns
 * appropriate error responses for different buffer result states.
 *
 * CHANGELOG:
 * 2026-01-28: Initial creation of unit tests for handle_auth_query_buffer_result
 *             Tests all switch cases: CONTINUE, ERROR, METHOD_ERROR, COMPLETE, default
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

// Include source header
#include <src/api/conduit/auth_query/auth_query.h>

// Function prototypes for test functions
void test_handle_auth_query_buffer_result_continue(void);
void test_handle_auth_query_buffer_result_error(void);
void test_handle_auth_query_buffer_result_method_error(void);
void test_handle_auth_query_buffer_result_complete(void);
void test_handle_auth_query_buffer_result_default(void);
void test_handle_auth_query_buffer_result_null_connection(void);
void test_handle_auth_query_buffer_result_null_con_cls(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
}

// Test API_BUFFER_CONTINUE returns MHD_YES
void test_handle_auth_query_buffer_result_continue(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, API_BUFFER_CONTINUE, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test API_BUFFER_ERROR returns error response
void test_handle_auth_query_buffer_result_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    // Mock api_send_error_and_cleanup to return MHD_YES
    mock_api_utils_set_send_error_result(MHD_YES);

    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, API_BUFFER_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test API_BUFFER_METHOD_ERROR returns method not allowed
void test_handle_auth_query_buffer_result_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    // Mock api_send_error_and_cleanup to return MHD_YES
    mock_api_utils_set_send_error_result(MHD_YES);

    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, API_BUFFER_METHOD_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test API_BUFFER_COMPLETE returns MHD_YES
void test_handle_auth_query_buffer_result_complete(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, API_BUFFER_COMPLETE, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test default case returns error response
void test_handle_auth_query_buffer_result_default(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    // Mock api_send_error_and_cleanup to return MHD_YES
    mock_api_utils_set_send_error_result(MHD_YES);

    // Cast an invalid value to test default case
    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, (ApiBufferResult)999, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test NULL connection handling
void test_handle_auth_query_buffer_result_null_connection(void) {
    void *con_cls = NULL;

    // Mock api_send_error_and_cleanup returns MHD_YES by default
    // The function passes through the mock's return value
    enum MHD_Result result = handle_auth_query_buffer_result(
        NULL, API_BUFFER_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test NULL con_cls handling
void test_handle_auth_query_buffer_result_null_con_cls(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Mock api_send_error_and_cleanup to return MHD_YES
    mock_api_utils_set_send_error_result(MHD_YES);

    enum MHD_Result result = handle_auth_query_buffer_result(
        mock_connection, API_BUFFER_ERROR, NULL
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_query_buffer_result_continue);
    RUN_TEST(test_handle_auth_query_buffer_result_error);
    RUN_TEST(test_handle_auth_query_buffer_result_method_error);
    RUN_TEST(test_handle_auth_query_buffer_result_complete);
    RUN_TEST(test_handle_auth_query_buffer_result_default);
    RUN_TEST(test_handle_auth_query_buffer_result_null_connection);
    RUN_TEST(test_handle_auth_query_buffer_result_null_con_cls);

    return UNITY_END();
}
