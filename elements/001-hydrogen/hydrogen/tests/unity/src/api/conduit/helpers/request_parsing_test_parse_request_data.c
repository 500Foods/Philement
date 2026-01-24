/*
 * Unity Test File: parse_request_data
 * This file contains unit tests for parse_request_data function
 * in src/api/conduit/helpers/request_parsing.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Function prototypes
void test_parse_request_data_post_valid_json(void);
void test_parse_request_data_post_invalid_json(void);
void test_parse_request_data_post_empty_body(void);
void test_parse_request_data_post_null_body(void);
void test_parse_request_data_get_with_params(void);
void test_parse_request_data_get_no_params(void);

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    // No cleanup needed
}

// Test POST with valid JSON
void test_parse_request_data_post_valid_json(void) {
    struct MHD_Connection *connection = NULL; // Not used in POST
    const char *method = "POST";
    const char *upload_data = "{\"query_ref\": 123, \"database\": \"test\"}";
    size_t upload_data_size = strlen(upload_data);

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Check parsed values
    json_t *query_ref = json_object_get(result, "query_ref");
    json_t *database = json_object_get(result, "database");

    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("test", json_string_value(database));

    json_decref(result);
}

// Test POST with invalid JSON
void test_parse_request_data_post_invalid_json(void) {
    struct MHD_Connection *connection = NULL;
    const char *method = "POST";
    const char *upload_data = "{\"invalid\": json}";
    size_t upload_data_size = strlen(upload_data);

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test POST with empty body
void test_parse_request_data_post_empty_body(void) {
    struct MHD_Connection *connection = NULL;
    const char *method = "POST";
    const char *upload_data = "";
    size_t upload_data_size = 0;

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test POST with NULL body
void test_parse_request_data_post_null_body(void) {
    struct MHD_Connection *connection = NULL;
    const char *method = "POST";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test GET with query parameters
void test_parse_request_data_get_with_params(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234; // Mock connection

    // Set up mock expectations
    mock_mhd_add_lookup("query_ref", "456");
    mock_mhd_add_lookup("database", "testdb");
    mock_mhd_add_lookup("params", "{\"key\": \"value\"}");

    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Check parsed values
    json_t *query_ref = json_object_get(result, "query_ref");
    json_t *database = json_object_get(result, "database");
    json_t *params = json_object_get(result, "params");

    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(456, json_integer_value(query_ref));
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("testdb", json_string_value(database));
    TEST_ASSERT_TRUE(json_is_object(params));

    json_t *key = json_object_get(params, "key");
    TEST_ASSERT_TRUE(json_is_string(key));
    TEST_ASSERT_EQUAL_STRING("value", json_string_value(key));

    json_decref(result);
}

// Test GET with no parameters
void test_parse_request_data_get_no_params(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    // No mock expectations set - all lookups return NULL

    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;

    json_t *result = parse_request_data(connection, method, upload_data, &upload_data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Should be empty object
    TEST_ASSERT_EQUAL(0, json_object_size(result));

    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_request_data_post_valid_json);
    RUN_TEST(test_parse_request_data_post_invalid_json);
    RUN_TEST(test_parse_request_data_post_empty_body);
    RUN_TEST(test_parse_request_data_post_null_body);
    RUN_TEST(test_parse_request_data_get_with_params);
    RUN_TEST(test_parse_request_data_get_no_params);

    return UNITY_END();
}