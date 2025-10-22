/*
 * Unity Test File: handle_method_validation
 * This file contains unit tests for handle_method_validation function in query.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <microhttpd.h>
#include <src/api/conduit/query/query.h>

// Forward declaration for the function under test
enum MHD_Result handle_method_validation(struct MHD_Connection *connection, const char* method);

// Dummy MHD_Connection for testing
typedef struct MHD_Connection MHD_Connection;

void setUp(void) {
    // No specific setup needed
}

void tearDown(void) {
    // Clean up if needed
}

// Test valid method (GET) - should return MHD_YES
static void test_handle_method_validation_get(void) {
    struct MHD_Connection *dummy_connection = (struct MHD_Connection *)0x1;
    const char* method = "GET";

    enum MHD_Result result = handle_method_validation(dummy_connection, method);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test valid method (POST) - should return MHD_YES
static void test_handle_method_validation_post(void) {
    struct MHD_Connection *dummy_connection = (struct MHD_Connection *)0x1;
    const char* method = "POST";

    enum MHD_Result result = handle_method_validation(dummy_connection, method);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test invalid method (PUT) - should return MHD_NO and send error response
static void test_handle_method_validation_invalid_method(void) {
    struct MHD_Connection *dummy_connection = (struct MHD_Connection *)0x1;
    const char* method = "PUT";

    enum MHD_Result result = handle_method_validation(dummy_connection, method);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    // Note: The actual response sending is mocked/handled by the build system
    // Coverage for the error path is achieved by taking the branch
}

// Test NULL method - should return MHD_NO
static void test_handle_method_validation_null_method(void) {
    struct MHD_Connection *dummy_connection = (struct MHD_Connection *)0x1;
    const char* method = NULL;

    enum MHD_Result result = handle_method_validation(dummy_connection, method);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_method_validation_get);
    RUN_TEST(test_handle_method_validation_post);
    RUN_TEST(test_handle_method_validation_invalid_method);
    RUN_TEST(test_handle_method_validation_null_method);

    return UNITY_END();
}