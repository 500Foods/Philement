/*
 * Unity Test File: handle_request_parsing
 * This file contains unit tests for handle_request_parsing function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Enable mock for parse_request_data
#define USE_MOCK_PARSE_REQUEST_DATA

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for parse_request_data
json_t* mock_parse_request_data(struct MHD_Connection* connection, const char* method,
                               const char* upload_data, const size_t* upload_data_size);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static json_t* mock_parse_result = NULL;
static bool mock_parse_should_fail = false;

// Function prototypes
void test_handle_request_parsing_success(void);
void test_handle_request_parsing_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                      const char* upload_data, const size_t* upload_data_size,
                                      json_t** request_json);

void setUp(void) {
    mock_system_reset_all();
    mock_parse_result = NULL;
    mock_parse_should_fail = false;
}

void tearDown(void) {
    mock_system_reset_all();
    if (mock_parse_result) {
        json_decref(mock_parse_result);
        mock_parse_result = NULL;
    }
    mock_parse_should_fail = false;
}

// Mock implementations
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status) {
    (void)connection;
    (void)response;
    (void)status;
    return MHD_YES;
}

void mock_json_decref(json_t* json) {
    (void)json;
}

json_t* mock_parse_request_data(struct MHD_Connection* connection, const char* method,
                               const char* upload_data, const size_t* upload_data_size) {
    (void)connection;
    (void)method;
    (void)upload_data;
    (void)upload_data_size;

    if (mock_parse_should_fail) {
        return NULL;
    }

    if (mock_parse_result) {
        return json_copy(mock_parse_result);
    }

    // Return a default valid JSON object
    return json_object();
}

// Test successful request parsing
void test_handle_request_parsing_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to succeed
    mock_parse_should_fail = false;
    mock_parse_result = json_object();
    json_object_set_new(mock_parse_result, "test", json_string("value"));

    json_t* request_json = NULL;
    const char* upload_data = "{\"test\":\"value\"}";
    size_t upload_data_size = strlen(upload_data);

    enum MHD_Result result = handle_request_parsing((struct MHD_Connection*)&mock_connection, "POST",
                                                   upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(request_json);

    // Cleanup
    json_decref(request_json);
}

// Test request parsing failure
void test_handle_request_parsing_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail
    mock_parse_should_fail = true;

    json_t* request_json = NULL;
    const char* upload_data = "invalid json";
    size_t upload_data_size = strlen(upload_data);

    enum MHD_Result result = handle_request_parsing((struct MHD_Connection*)&mock_connection, "POST",
                                                   upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_request_parsing_success);
    RUN_TEST(test_handle_request_parsing_failure);

    return UNITY_END();
}