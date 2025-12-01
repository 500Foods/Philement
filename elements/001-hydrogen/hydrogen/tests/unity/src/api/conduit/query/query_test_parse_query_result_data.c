/*
 * Unity Test File: Test parse_query_result_data function
 * This file contains unit tests for src/api/conduit/query/query.c parse_query_result_data function
 */

#include <src/hydrogen.h>
#include "unity.h"

// Include necessary headers for the module being tested
#include <src/database/database.h>

// Forward declaration for the function being tested
json_t* parse_query_result_data(const QueryResult* result);

// Test function prototypes
void test_parse_query_result_data_null_data(void);
void test_parse_query_result_data_valid_json(void);
void test_parse_query_result_data_invalid_json(void);
void test_parse_query_result_data_empty_string(void);

// Test function prototypes
void test_parse_query_result_data_null_data(void);
void test_parse_query_result_data_valid_json(void);
void test_parse_query_result_data_invalid_json(void);
void test_parse_query_result_data_empty_string(void);

void setUp(void) {
    // No specific setup
}

void tearDown(void) {
    // No specific teardown
}

// Test with NULL data_json - should return empty array
void test_parse_query_result_data_null_data(void) {
    QueryResult mock_result = { .data_json = NULL };

    json_t* data = parse_query_result_data(&mock_result);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE(json_is_array(data));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(data));

    json_decref(data);
}

// Test with valid JSON data
void test_parse_query_result_data_valid_json(void) {
    char data_json_valid[] = "[{\"id\":1,\"name\":\"test\"}]";
    QueryResult mock_result = { .data_json = data_json_valid };

    json_t* data = parse_query_result_data(&mock_result);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE(json_is_array(data));
    TEST_ASSERT_EQUAL_INT(1, json_array_size(data));

    json_t* first_row = json_array_get(data, 0);
    TEST_ASSERT_TRUE(json_is_object(first_row));

    json_t* id = json_object_get(first_row, "id");
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(id));

    json_t* name = json_object_get(first_row, "name");
    TEST_ASSERT_EQUAL_STRING("test", json_string_value(name));

    json_decref(data);
}

// Test with invalid JSON data - should return empty array
void test_parse_query_result_data_invalid_json(void) {
    char data_json_invalid[] = "invalid json {";
    QueryResult mock_result = { .data_json = data_json_invalid };

    json_t* data = parse_query_result_data(&mock_result);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE(json_is_array(data));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(data));

    json_decref(data);
}

// Test with empty string data_json
void test_parse_query_result_data_empty_string(void) {
    char data_json_empty[] = "";
    QueryResult mock_result = { .data_json = data_json_empty };

    json_t* data = parse_query_result_data(&mock_result);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE(json_is_array(data));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(data));  // json_loads("") returns NULL, so empty array

    json_decref(data);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_query_result_data_null_data);
    RUN_TEST(test_parse_query_result_data_valid_json);
    RUN_TEST(test_parse_query_result_data_invalid_json);
    RUN_TEST(test_parse_query_result_data_empty_string);

    return UNITY_END();
}