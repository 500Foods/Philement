/*
 * Unity Unit Tests for renew_utils.c - create_renew_success_response
 * 
 * Tests the success response creation functionality
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for success response creation
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

void test_create_renew_success_response_basic(void);
void test_create_renew_success_response_null_token(void);
void test_create_renew_success_response_zero_expires(void);

// ============================================================================
// Test Functions
// ============================================================================

void test_create_renew_success_response_basic(void) {
    time_t expires_at = 1704830000;
    json_t* response = create_renew_success_response("new.jwt.token", expires_at);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_TRUE(json_boolean_value(success));
    
    json_t* token = json_object_get(response, "token");
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_TRUE(json_is_string(token));
    TEST_ASSERT_EQUAL_STRING("new.jwt.token", json_string_value(token));
    
    json_t* expires = json_object_get(response, "expires_at");
    TEST_ASSERT_NOT_NULL(expires);
    TEST_ASSERT_TRUE(json_is_integer(expires));
    TEST_ASSERT_EQUAL(expires_at, json_integer_value(expires));
    
    json_decref(response);
}

void test_create_renew_success_response_null_token(void) {
    time_t expires_at = 1704830000;
    json_t* response = create_renew_success_response(NULL, expires_at);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_TRUE(json_boolean_value(success));
    
    json_t* token = json_object_get(response, "token");
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_TRUE(json_is_string(token));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(token));
    
    json_t* expires = json_object_get(response, "expires_at");
    TEST_ASSERT_NOT_NULL(expires);
    TEST_ASSERT_TRUE(json_is_integer(expires));
    TEST_ASSERT_EQUAL(expires_at, json_integer_value(expires));
    
    json_decref(response);
}

void test_create_renew_success_response_zero_expires(void) {
    json_t* response = create_renew_success_response("new.jwt.token", 0);
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_boolean(success));
    TEST_ASSERT_TRUE(json_boolean_value(success));
    
    json_t* token = json_object_get(response, "token");
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_TRUE(json_is_string(token));
    TEST_ASSERT_EQUAL_STRING("new.jwt.token", json_string_value(token));
    
    json_t* expires = json_object_get(response, "expires_at");
    TEST_ASSERT_NOT_NULL(expires);
    TEST_ASSERT_TRUE(json_is_integer(expires));
    TEST_ASSERT_EQUAL(0, json_integer_value(expires));
    
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
    
    RUN_TEST(test_create_renew_success_response_basic);
    RUN_TEST(test_create_renew_success_response_null_token);
    RUN_TEST(test_create_renew_success_response_zero_expires);
    
    return UNITY_END();
}