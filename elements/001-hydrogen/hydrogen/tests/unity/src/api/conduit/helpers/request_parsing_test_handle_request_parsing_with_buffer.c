/*
 * Unity Test File: handle_request_parsing_with_buffer
 * This file contains unit tests for handle_request_parsing_with_buffer function
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
void test_handle_request_parsing_with_buffer_post_valid(void);
void test_handle_request_parsing_with_buffer_post_invalid_json(void);
void test_handle_request_parsing_with_buffer_post_empty(void);
void test_handle_request_parsing_with_buffer_get(void);

void setUp(void) {
    mock_api_utils_reset_all();
}

void tearDown(void) {
    // No cleanup needed
}

// Test POST with valid JSON in buffer
void test_handle_request_parsing_with_buffer_post_valid(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    // Create buffer with valid JSON
    ApiPostBuffer buffer;
    buffer.magic = 0x12345678; // API_POST_BUFFER_MAGIC
    const char *json_data = "{\"query_ref\": 789, \"database\": \"bufferdb\"}";
    buffer.data = (char *)json_data; // Cast away const for test
    buffer.size = strlen(buffer.data);
    buffer.capacity = buffer.size + 1;
    buffer.http_method = 'P'; // POST

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing_with_buffer(connection, &buffer, &request_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(request_json);

    // Verify parsed content
    json_t *query_ref = json_object_get(request_json, "query_ref");
    json_t *database = json_object_get(request_json, "database");

    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(789, json_integer_value(query_ref));
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("bufferdb", json_string_value(database));

    json_decref(request_json);
}

// Test POST with invalid JSON in buffer
void test_handle_request_parsing_with_buffer_post_invalid_json(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    // Create buffer with invalid JSON
    ApiPostBuffer buffer;
    buffer.magic = 0x12345678;
    const char *invalid_json = "{\"invalid\": json}";
    buffer.data = (char *)invalid_json; // Cast away const for test
    buffer.size = strlen(buffer.data);
    buffer.capacity = buffer.size + 1;
    buffer.http_method = 'P';

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing_with_buffer(connection, &buffer, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

// Test POST with empty buffer
void test_handle_request_parsing_with_buffer_post_empty(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    // Create empty buffer
    ApiPostBuffer buffer;
    buffer.magic = 0x12345678;
    const char *empty_data = "";
    buffer.data = (char *)empty_data; // Cast away const for test
    buffer.size = 0;
    buffer.capacity = 1;
    buffer.http_method = 'P';

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing_with_buffer(connection, &buffer, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

// Test GET method (should work but buffer not used)
void test_handle_request_parsing_with_buffer_get(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    // Create buffer for GET (data not used)
    ApiPostBuffer buffer;
    buffer.magic = 0x12345678;
    buffer.data = NULL;
    buffer.size = 0;
    buffer.capacity = 0;
    buffer.http_method = 'G'; // GET

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing_with_buffer(connection, &buffer, &request_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(request_json);
    TEST_ASSERT_TRUE(json_is_object(request_json));
    TEST_ASSERT_EQUAL(0, json_object_size(request_json)); // Empty for GET with no params

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_request_parsing_with_buffer_post_valid);
    RUN_TEST(test_handle_request_parsing_with_buffer_post_invalid_json);
    RUN_TEST(test_handle_request_parsing_with_buffer_post_empty);
    RUN_TEST(test_handle_request_parsing_with_buffer_get);

    return UNITY_END();
}