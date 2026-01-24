/*
 * Unity Test File: create_processing_error_response
 * This file contains unit tests for create_processing_error_response function
 * in src/api/conduit/helpers/error_handling.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Function prototypes
void test_create_processing_error_response_basic(void);
void test_create_processing_error_response_with_database(void);
void test_create_processing_error_response_with_query_ref(void);
void test_create_processing_error_response_all_fields(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test basic error response creation
void test_create_processing_error_response_basic(void) {
    json_t* response = create_processing_error_response("Test error", NULL, 0);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check success field
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error field
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Test error", json_string_value(error));

    // Check query_ref field
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(query_ref));

    // Check database field (should be empty string when NULL)
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(database));

    json_decref(response);
}

// Test with database field
void test_create_processing_error_response_with_database(void) {
    json_t* response = create_processing_error_response("Test error", "mydb", 0);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check database field
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("mydb", json_string_value(database));

    json_decref(response);
}

// Test with query ref field
void test_create_processing_error_response_with_query_ref(void) {
    json_t* response = create_processing_error_response("Test error", NULL, 123);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check query_ref field
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(query_ref));

    json_decref(response);
}

// Test with all fields
void test_create_processing_error_response_all_fields(void) {
    json_t* response = create_processing_error_response("Test error", "mydb", 123);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check all fields
    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* query_ref = json_object_get(response, "query_ref");
    json_t* database = json_object_get(response, "database");

    TEST_ASSERT_TRUE(json_is_false(success));
    TEST_ASSERT_EQUAL_STRING("Test error", json_string_value(error));
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(query_ref));
    TEST_ASSERT_EQUAL_STRING("mydb", json_string_value(database));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_processing_error_response_basic);
    RUN_TEST(test_create_processing_error_response_with_database);
    RUN_TEST(test_create_processing_error_response_with_query_ref);
    RUN_TEST(test_create_processing_error_response_all_fields);

    return UNITY_END();
}
