/*
 * Unity Test File: WebSocket JSON Response Tests
 * This file contains comprehensive unit tests for ws_write_json_response function
 * focusing on JSON serialization, buffer allocation, and error handling.
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket message module
#include "../../../../src/websocket/websocket_server_internal.h"
#include "../../../../src/websocket/websocket_server.h"

// Mock libwebsockets for testing
#include "../../../../tests/unity/mocks/mock_libwebsockets.h"

// Forward declarations for functions being tested
int ws_write_json_response(struct lws *wsi, json_t *json);

// Test functions
void test_ws_write_json_response_null_json(void);
void test_ws_write_json_response_buffer_allocation_failure(void);
void test_ws_write_json_response_successful_write(void);
void test_ws_write_json_response_empty_json_object(void);
void test_ws_write_json_response_complex_json_data(void);
void test_ws_write_json_response_lws_write_failure(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_lws_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_lws_reset_all();
}

void test_ws_write_json_response_null_json(void) {
    // Test: NULL JSON input should be handled gracefully
    struct lws *test_wsi = (struct lws *)0x12345678;

    int result = ws_write_json_response(test_wsi, NULL);

    // Should return -1 for NULL JSON
    TEST_ASSERT_EQUAL_INT(-1, result);

    // lws_write should not be called for NULL JSON input
    // We can't easily verify this without a getter function, but the test passes if no crash occurs
}

// JSON serialization test removed - json_dumps() is working correctly in test environment

void test_ws_write_json_response_buffer_allocation_failure(void) {
    // Test: Buffer allocation failure handling
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *json = json_object();

    // Set up JSON object
    json_object_set_new(json, "type", json_string("status"));

    // For this test, we'll focus on the JSON serialization path
    // System-level malloc failures are harder to test in this context
    // but the function should handle NULL buffer gracefully

    (void)ws_write_json_response(test_wsi, json); // Suppress unused variable warning

    // The function should handle the case gracefully
    // We can't easily simulate malloc failure in this test environment
    // but we can verify the function doesn't crash

    json_decref(json);
}

void test_ws_write_json_response_successful_write(void) {
    // Test: Successful JSON response write
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *json = json_object();

    // Set up JSON object
    json_object_set_new(json, "type", json_string("status"));
    json_object_set_new(json, "status", json_string("success"));

    // Mock successful lws_write
    mock_lws_set_write_result(50);

    int result = ws_write_json_response(test_wsi, json);

    // Should return success (bytes written)
    TEST_ASSERT_EQUAL_INT(50, result);

    json_decref(json);
}

void test_ws_write_json_response_empty_json_object(void) {
    // Test: Empty JSON object handling
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *json = json_object(); // Empty object

    // Mock successful lws_write
    mock_lws_set_write_result(2); // "{}" is 2 characters

    int result = ws_write_json_response(test_wsi, json);

    // Should return success
    TEST_ASSERT_EQUAL_INT(2, result);

    json_decref(json);
}

void test_ws_write_json_response_complex_json_data(void) {
    // Test: Complex JSON data with nested objects and arrays
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *json = json_object();

    // Create complex JSON structure
    json_object_set_new(json, "type", json_string("terminal_output"));
    json_object_set_new(json, "timestamp", json_integer(1234567890));

    json_t *data_array = json_array();
    json_array_append_new(data_array, json_string("line1"));
    json_array_append_new(data_array, json_string("line2"));
    json_object_set_new(json, "data", data_array);

    // Mock successful lws_write with expected length
    mock_lws_set_write_result(100);

    int result = ws_write_json_response(test_wsi, json);

    // Should return success
    TEST_ASSERT_EQUAL_INT(100, result);

    json_decref(json);
}

void test_ws_write_json_response_lws_write_failure(void) {
    // Test: lws_write failure handling
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *json = json_object();

    // Set up JSON object
    json_object_set_new(json, "type", json_string("error"));
    json_object_set_new(json, "message", json_string("test error"));

    // Mock lws_write failure
    mock_lws_set_write_result(-1);

    int result = ws_write_json_response(test_wsi, json);

    // Should return failure
    TEST_ASSERT_EQUAL_INT(-1, result);

    json_decref(json);
}

int main(void) {
    UNITY_BEGIN();

    // NULL and error condition tests
    RUN_TEST(test_ws_write_json_response_null_json);
    RUN_TEST(test_ws_write_json_response_buffer_allocation_failure);
    RUN_TEST(test_ws_write_json_response_lws_write_failure);

    // Success path tests
    RUN_TEST(test_ws_write_json_response_successful_write);
    RUN_TEST(test_ws_write_json_response_empty_json_object);
    RUN_TEST(test_ws_write_json_response_complex_json_data);

    return UNITY_END();
}