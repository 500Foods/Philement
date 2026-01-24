/*
 * Unity Test File: handle_request_parsing
 * This file contains unit tests for handle_request_parsing function
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
void test_handle_request_parsing_post_valid(void);
void test_handle_request_parsing_post_invalid_json(void);
void test_handle_request_parsing_post_empty(void);
void test_handle_request_parsing_post_null(void);
void test_handle_request_parsing_get(void);

void setUp(void) {
    mock_api_utils_reset_all();
}

void tearDown(void) {
    // No cleanup needed
}

// Test POST with valid JSON
void test_handle_request_parsing_post_valid(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 999, \"database\": \"directdb\"}";
    size_t upload_data_size = strlen(upload_data);

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing(connection, method, upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(request_json);

    // Verify parsed content
    json_t *query_ref = json_object_get(request_json, "query_ref");
    json_t *database = json_object_get(request_json, "database");

    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(999, json_integer_value(query_ref));
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("directdb", json_string_value(database));

    json_decref(request_json);
}

// Test POST with invalid JSON
void test_handle_request_parsing_post_invalid_json(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "POST";
    const char *upload_data = "{\"broken\": json}";
    size_t upload_data_size = strlen(upload_data);

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing(connection, method, upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

// Test POST with empty data
void test_handle_request_parsing_post_empty(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "POST";
    const char *upload_data = "";
    size_t upload_data_size = 0;

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing(connection, method, upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

// Test POST with NULL data
void test_handle_request_parsing_post_null(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "POST";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing(connection, method, upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(request_json);
}

// Test GET method
void test_handle_request_parsing_get(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;

    json_t *request_json = NULL;

    enum MHD_Result result = handle_request_parsing(connection, method, upload_data, &upload_data_size, &request_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(request_json);
    TEST_ASSERT_TRUE(json_is_object(request_json));
    // For GET with no query params, should be empty object
    TEST_ASSERT_EQUAL(0, json_object_size(request_json));

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_request_parsing_post_valid);
    RUN_TEST(test_handle_request_parsing_post_invalid_json);
    RUN_TEST(test_handle_request_parsing_post_empty);
    RUN_TEST(test_handle_request_parsing_post_null);
    RUN_TEST(test_handle_request_parsing_get);

    return UNITY_END();
}