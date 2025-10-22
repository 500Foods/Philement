/*
 * Unity Test File: Test create_processing_error_response function
 * This file contains unit tests for src/api/conduit/query/query.c create_processing_error_response function
 */

#include <src/hydrogen.h>
#include "unity.h"

// Forward declaration for the function being tested
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref);

// Function prototypes
void setUp(void);
void tearDown(void);
void test_create_processing_error_response_basic(void);
void test_create_processing_error_response_different_params(void);

void setUp(void) {
    // No specific setup
}

void tearDown(void) {
    // No specific teardown
}

// Test basic processing error response
void test_create_processing_error_response_basic(void) {
    json_t* response = create_processing_error_response("Processing failed", "test_db", 123);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_EQUAL_STRING("Processing failed", json_string_value(error));

    json_t* ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(ref));

    json_t* db = json_object_get(response, "database");
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(db));

    json_decref(response);
}

// Test with different parameters
void test_create_processing_error_response_different_params(void) {
    json_t* response = create_processing_error_response("Memory error", "memory_db", 456);

    TEST_ASSERT_NOT_NULL(response);

    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_EQUAL_STRING("Memory error", json_string_value(error));

    json_t* ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL_INT(456, json_integer_value(ref));

    json_t* db = json_object_get(response, "database");
    TEST_ASSERT_EQUAL_STRING("memory_db", json_string_value(db));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_processing_error_response_basic);
    RUN_TEST(test_create_processing_error_response_different_params);

    return UNITY_END();
}