/*
 * Unity Test File: handle_method_validation
 * This file contains unit tests for handle_method_validation function
 * in src/api/conduit/helpers/request_parsing.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_api_utils.h>

// Local mock for api_send_json_response
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code);

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Mock implementation
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code) {
    (void)connection;
    (void)json_obj;
    (void)status_code;
    return MHD_YES; // Always succeed for tests
}

// Function prototypes
void test_handle_method_validation_post(void);
void test_handle_method_validation_get(void);
void test_handle_method_validation_null(void);

void setUp(void) {
    mock_api_utils_reset_all();
}

void tearDown(void) {
    // No cleanup needed
}

// Test POST method (should succeed)
void test_handle_method_validation_post(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "POST";

    enum MHD_Result result = handle_method_validation(connection, method);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test GET method (should send error response)
void test_handle_method_validation_get(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "GET";

    enum MHD_Result result = handle_method_validation(connection, method);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test NULL method (should send error response)
void test_handle_method_validation_null(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = NULL;

    enum MHD_Result result = handle_method_validation(connection, method);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_method_validation_post);
    RUN_TEST(test_handle_method_validation_get);
    RUN_TEST(test_handle_method_validation_null);

    return UNITY_END();
}