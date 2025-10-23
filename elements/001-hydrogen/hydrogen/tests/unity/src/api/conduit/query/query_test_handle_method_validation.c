/*
 * Unity Test File: handle_method_validation
 * This file contains unit tests for handle_method_validation function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Function prototypes
void test_handle_method_validation_valid_get(void);
void test_handle_method_validation_valid_post(void);
void test_handle_method_validation_invalid_method(void);

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test valid GET method
void test_handle_method_validation_valid_get(void) {
    struct MHD_Connection* mock_connection = NULL; // Use NULL for mock

    enum MHD_Result result = handle_method_validation(mock_connection, "GET");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test valid POST method
void test_handle_method_validation_valid_post(void) {
    struct MHD_Connection* mock_connection = NULL; // Use NULL for mock

    enum MHD_Result result = handle_method_validation(mock_connection, "POST");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid method (this should trigger error response)
void test_handle_method_validation_invalid_method(void) {
    struct MHD_Connection* mock_connection = NULL; // Use NULL for mock

    enum MHD_Result result = handle_method_validation(mock_connection, "PUT");

    // Should return MHD_NO due to invalid method
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_method_validation_valid_get);
    RUN_TEST(test_handle_method_validation_valid_post);
    RUN_TEST(test_handle_method_validation_invalid_method);

    return UNITY_END();
}