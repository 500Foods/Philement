/*
 * Unity Unit Tests for logout_utils.c - create_logout_success_response
 *
 * Tests the create_logout_success_response function
 *
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for create_logout_success_response
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
void test_create_logout_success_response_basic(void);
void test_create_logout_success_response_json_structure(void);
void test_create_logout_success_response_consistency(void);

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

// Test: Basic success response creation
void test_create_logout_success_response_basic(void) {
    json_t* response = create_logout_success_response();
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_true(success));
    
    json_t* message = json_object_get(response, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_TRUE(json_is_string(message));
    TEST_ASSERT_EQUAL_STRING("Logout successful", json_string_value(message));
    
    json_decref(response);
}

// Test: JSON structure validation
void test_create_logout_success_response_json_structure(void) {
    json_t* response = create_logout_success_response();
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    // Check that response has exactly 2 fields
    TEST_ASSERT_EQUAL(2, json_object_size(response));
    
    // Check field names and types
    const char* key;
    json_t* value;
    void* iter = json_object_iter(response);
    
    key = json_object_iter_key(iter);
    TEST_ASSERT_EQUAL_STRING("success", key);
    value = json_object_iter_value(iter);
    TEST_ASSERT_TRUE(json_is_true(value));
    
    iter = json_object_iter_next(response, iter);
    key = json_object_iter_key(iter);
    TEST_ASSERT_EQUAL_STRING("message", key);
    value = json_object_iter_value(iter);
    TEST_ASSERT_TRUE(json_is_string(value));
    
    json_decref(response);
}

// Test: Consistency across multiple calls
void test_create_logout_success_response_consistency(void) {
    json_t* response1 = create_logout_success_response();
    json_t* response2 = create_logout_success_response();
    
    TEST_ASSERT_NOT_NULL(response1);
    TEST_ASSERT_NOT_NULL(response2);
    
    json_t* message1 = json_object_get(response1, "message");
    json_t* message2 = json_object_get(response2, "message");
    
    TEST_ASSERT_EQUAL_STRING(
        json_string_value(message1),
        json_string_value(message2)
    );
    
    json_decref(response1);
    json_decref(response2);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_create_logout_success_response_basic);
    RUN_TEST(test_create_logout_success_response_json_structure);
    RUN_TEST(test_create_logout_success_response_consistency);
    
    return UNITY_END();
}