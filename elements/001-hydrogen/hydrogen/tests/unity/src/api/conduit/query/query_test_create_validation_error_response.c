/*
 * Unity Test File: create_validation_error_response
 * This file contains unit tests for create_validation_error_response function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Function prototypes
void test_create_validation_error_response_basic(void);
void test_create_validation_error_response_with_details(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test basic error response creation
void test_create_validation_error_response_basic(void) {
    json_t* response = create_validation_error_response("Test error", "Test details");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check success field
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error field
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Test error", json_string_value(error));

    // Check message field
    json_t* message = json_object_get(response, "message");
    TEST_ASSERT_TRUE(json_is_string(message));
    TEST_ASSERT_EQUAL_STRING("Test details", json_string_value(message));

    json_decref(response);
}

// Test with different error messages
void test_create_validation_error_response_with_details(void) {
    json_t* response = create_validation_error_response("Invalid input", "Field 'name' is required");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check all fields
    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* message = json_object_get(response, "message");

    TEST_ASSERT_TRUE(json_is_false(success));
    TEST_ASSERT_EQUAL_STRING("Invalid input", json_string_value(error));
    TEST_ASSERT_EQUAL_STRING("Field 'name' is required", json_string_value(message));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_validation_error_response_basic);
    RUN_TEST(test_create_validation_error_response_with_details);

    return UNITY_END();
}