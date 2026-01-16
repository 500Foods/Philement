/*
 * Unity Unit Tests for logout_utils.c - create_logout_error_response
 *
 * Tests the create_logout_error_response function
 *
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for create_logout_error_response
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/logout/logout_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_create_logout_error_response_valid_error_message(void);
void test_create_logout_error_response_null_error_message(void);
void test_create_logout_error_response_empty_error_message(void);
void test_create_logout_error_response_json_structure(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No teardown needed for these tests
}

// ============================================================================
// Test Functions
// ============================================================================

// Test: Valid error message
void test_create_logout_error_response_valid_error_message(void) {
    const char* error_msg = "Invalid token";
    json_t* response = create_logout_error_response(error_msg);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_false(success));
    
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING(error_msg, json_string_value(error));
    
    json_decref(response);
}


// Test: Empty error message
void test_create_logout_error_response_empty_error_message(void) {
    const char* error_msg = "";
    json_t* response = create_logout_error_response(error_msg);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_false(success));
    
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(error));
    
    json_decref(response);
}

// Test: JSON structure validation
void test_create_logout_error_response_json_structure(void) {
    const char* error_msg = "Token expired";
    json_t* response = create_logout_error_response(error_msg);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    // Check that response has exactly 2 fields
    TEST_ASSERT_EQUAL(2, json_object_size(response));
    
    // Check field names
    const char* key;
    json_t* value;
    void* iter = json_object_iter(response);
    
    key = json_object_iter_key(iter);
    TEST_ASSERT_EQUAL_STRING("success", key);
    value = json_object_iter_value(iter);
    TEST_ASSERT_TRUE(json_is_false(value));
    
    iter = json_object_iter_next(response, iter);
    key = json_object_iter_key(iter);
    TEST_ASSERT_EQUAL_STRING("error", key);
    value = json_object_iter_value(iter);
    TEST_ASSERT_TRUE(json_is_string(value));
    
    json_decref(response);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_logout_error_response_valid_error_message);
    RUN_TEST(test_create_logout_error_response_empty_error_message);
    RUN_TEST(test_create_logout_error_response_json_structure);
    
    return UNITY_END();
}