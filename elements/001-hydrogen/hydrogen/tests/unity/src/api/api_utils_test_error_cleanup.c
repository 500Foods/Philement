/*
 * Unity Test File: API Utils api_send_error_and_cleanup Function Tests
 * This file contains unit tests for the api_send_error_and_cleanup function in api_utils.c
 * 
 * Target coverage: Lines 532-545 in api_utils.c (Error response with cleanup)
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mock for MHD functions
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // Reset mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test functions
void test_api_send_error_and_cleanup_basic(void);
void test_api_send_error_and_cleanup_with_buffer(void);
void test_api_send_error_and_cleanup_null_con_cls(void);
void test_api_send_error_and_cleanup_various_statuses(void);
void test_api_send_error_and_cleanup_message_content(void);
void test_api_send_error_and_cleanup_with_data_buffer(void);

// Test basic error and cleanup
void test_api_send_error_and_cleanup_basic(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Lines 532-545: Send error and cleanup
    enum MHD_Result result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Test error message",
        MHD_HTTP_BAD_REQUEST
    );
    
    // Should return MHD_YES (or whatever api_send_json_response returns)
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // con_cls should be freed (set to NULL)
    TEST_ASSERT_NULL(con_cls);
}

// Test with actual buffer to cleanup
void test_api_send_error_and_cleanup_with_buffer(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First create a buffer
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Verify buffer was created
    TEST_ASSERT_NOT_NULL(con_cls);
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Now send error and cleanup
    // Lines 539: api_free_post_buffer called
    // Lines 542-545: Error response created and sent
    enum MHD_Result result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Internal server error",
        MHD_HTTP_INTERNAL_SERVER_ERROR
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Buffer should be freed
    TEST_ASSERT_NULL(con_cls);
}

// Test with NULL con_cls pointer
void test_api_send_error_and_cleanup_null_con_cls(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Call with NULL con_cls pointer
    // This should still work - api_free_post_buffer handles NULL
    enum MHD_Result result = api_send_error_and_cleanup(
        connection,
        NULL,  // NULL pointer to con_cls
        "Error message",
        MHD_HTTP_NOT_FOUND
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test with various HTTP status codes
void test_api_send_error_and_cleanup_various_statuses(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test 400 Bad Request
    enum MHD_Result result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Bad request",
        400
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Test 401 Unauthorized
    result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Unauthorized",
        401
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Test 403 Forbidden
    result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Forbidden",
        403
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Test 404 Not Found
    result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Not found",
        404
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Test 500 Internal Server Error
    result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Internal server error",
        500
    );
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test error message content
void test_api_send_error_and_cleanup_message_content(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test with different error messages
    const char *messages[] = {
        "Simple error",
        "Error with special chars: @#$%",
        "Very long error message that contains a lot of text to ensure proper handling",
        ""  // Empty message
    };
    
    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        enum MHD_Result result = api_send_error_and_cleanup(
            connection,
            &con_cls,
            messages[i],
            MHD_HTTP_BAD_REQUEST
        );
        TEST_ASSERT_EQUAL(MHD_YES, result);
    }
}

// Test with buffer containing data
void test_api_send_error_and_cleanup_with_data_buffer(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // Create buffer with POST method
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Add some data to the buffer
    const char *data = "test data content";
    upload_data_size = strlen(data);
    api_buffer_post_data("POST", data, &upload_data_size, &con_cls, &buffer_out);
    
    ApiPostBuffer *buffer = (ApiPostBuffer *)con_cls;
    TEST_ASSERT_GREATER_THAN(0, buffer->size);
    
    // Mock the response functions
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Now cleanup - should free the buffer including its data
    enum MHD_Result result = api_send_error_and_cleanup(
        connection,
        &con_cls,
        "Processing failed",
        MHD_HTTP_UNPROCESSABLE_ENTITY
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NULL(con_cls);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_send_error_and_cleanup_basic);
    RUN_TEST(test_api_send_error_and_cleanup_with_buffer);
    RUN_TEST(test_api_send_error_and_cleanup_null_con_cls);
    RUN_TEST(test_api_send_error_and_cleanup_various_statuses);
    RUN_TEST(test_api_send_error_and_cleanup_message_content);
    RUN_TEST(test_api_send_error_and_cleanup_with_data_buffer);
    
    return UNITY_END();
}
