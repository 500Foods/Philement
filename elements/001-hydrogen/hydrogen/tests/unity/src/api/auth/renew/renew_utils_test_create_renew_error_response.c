/*
 * Unity Unit Tests for renew_utils.c - create_renew_error_response
 * 
 * Tests the error response creation functionality
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for error response creation
 * 
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/renew/renew_utils.h>
#include <jansson.h>

// ============================================================================
// Function Prototypes
// ============================================================================

void test_create_renew_error_response_basic(void);
void test_create_renew_error_response_null_message(void);
void test_create_renew_error_response_empty_message(void);

// ============================================================================
// Test Functions
// ============================================================================

void test_create_renew_error_response_basic(void) {
    json_t* response = create_renew_error_response("Test error message");
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_FALSE(json_boolean_value(success));
    
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Test error message", json_string_value(error));
    
    json_decref(response);
}

void test_create_renew_error_response_null_message(void) {
    json_t* response = create_renew_error_response(NULL);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_FALSE(json_boolean_value(success));
    
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(error));
    
    json_decref(response);
}

void test_create_renew_error_response_empty_message(void) {
    json_t* response = create_renew_error_response("");
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_FALSE(json_boolean_value(success));
    
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(error));
    
    json_decref(response);
}

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    // Nothing to set up
}

void tearDown(void) {
    // Nothing to tear down
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_renew_error_response_basic);
    RUN_TEST(test_create_renew_error_response_null_message);
    RUN_TEST(test_create_renew_error_response_empty_message);
    
    return UNITY_END();
}