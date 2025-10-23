/*
 * Unity Test File: parse_request_data
 * This file contains unit tests for parse_request_data function
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
void test_parse_request_data_post_valid(void);
void test_parse_request_data_post_invalid_json(void);
void test_parse_request_data_post_missing_body(void);
void test_parse_request_data_get_valid(void);
void test_parse_request_data_get_invalid_params_json(void);

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test POST request with valid JSON
void test_parse_request_data_post_valid(void) {
    const char* json_str = "{\"query_ref\":123,\"database\":\"test_db\"}";
    size_t data_size = strlen(json_str);

    json_t* result = parse_request_data(NULL, "POST", json_str, &data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Check that fields are parsed correctly
    json_t* query_ref = json_object_get(result, "query_ref");
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));

    json_t* database = json_object_get(result, "database");
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    json_decref(result);
}

// Test POST request with invalid JSON
void test_parse_request_data_post_invalid_json(void) {
    const char* invalid_json = "{\"query_ref\":123,\"database\":";
    size_t data_size = strlen(invalid_json);

    json_t* result = parse_request_data(NULL, "POST", invalid_json, &data_size);

    TEST_ASSERT_NULL(result);
}

// Test POST request with missing body
void test_parse_request_data_post_missing_body(void) {
    size_t data_size = 0;

    json_t* result = parse_request_data(NULL, "POST", NULL, &data_size);

    TEST_ASSERT_NULL(result);
}

// Test GET request with valid parameters
void test_parse_request_data_get_valid(void) {
    // Mock MHD lookup results
    mock_mhd_add_lookup("query_ref", "456");
    mock_mhd_add_lookup("database", "test_db");
    mock_mhd_add_lookup("params", "{\"key\":\"value\"}");

    struct MHD_Connection* mock_connection = NULL; // Use NULL for mock
    size_t data_size = 0;

    json_t* result = parse_request_data(mock_connection, "GET", NULL, &data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Check that fields are parsed correctly
    json_t* query_ref = json_object_get(result, "query_ref");
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(456, json_integer_value(query_ref));

    json_t* database = json_object_get(result, "database");
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    json_t* params = json_object_get(result, "params");
    TEST_ASSERT_TRUE(json_is_object(params));

    json_decref(result);
}

// Test GET request with invalid params JSON (this should cover lines 119-124)
void test_parse_request_data_get_invalid_params_json(void) {
    // Mock MHD lookup results with invalid params JSON
    mock_mhd_add_lookup("query_ref", "789");
    mock_mhd_add_lookup("database", "test_db");
    mock_mhd_add_lookup("params", "{\"invalid\":json}");

    struct MHD_Connection* mock_connection = NULL; // Use NULL for mock
    size_t data_size = 0;

    json_t* result = parse_request_data(mock_connection, "GET", NULL, &data_size);

    // Should return NULL due to invalid params JSON
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_request_data_post_valid);
    RUN_TEST(test_parse_request_data_post_invalid_json);
    RUN_TEST(test_parse_request_data_post_missing_body);
    RUN_TEST(test_parse_request_data_get_valid);
    RUN_TEST(test_parse_request_data_get_invalid_params_json);

    return UNITY_END();
}