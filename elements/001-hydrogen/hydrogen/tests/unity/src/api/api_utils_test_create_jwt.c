/*
 * Unity Test File: API Utils api_create_jwt Function Tests
 * This file contains unit tests for the api_create_jwt function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/api/api_utils.h"

// Function prototypes for test functions
void test_api_create_jwt_valid_inputs(void);
void test_api_create_jwt_null_claims(void);
void test_api_create_jwt_null_secret(void);
void test_api_create_jwt_both_null(void);
void test_api_create_jwt_empty_claims(void);
void test_api_create_jwt_empty_secret(void);
void test_api_create_jwt_complex_claims(void);
void test_api_create_jwt_long_secret(void);
void test_api_create_jwt_nested_claims(void);
void test_api_create_jwt_array_claims(void);
void test_api_create_jwt_consistency(void);
void test_api_create_jwt_special_characters(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No teardown needed for this function
}

// Test basic JWT creation with valid inputs
void test_api_create_jwt_valid_inputs(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("test_user"));
    json_object_set_new(claims, "iss", json_string("hydrogen"));
    
    char *result = api_create_jwt(claims, "secret123");
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Since this is currently a stub implementation, it should return a dummy token
    TEST_ASSERT_EQUAL_STRING("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkR1bW15IFRva2VuIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c", result);
    
    free(result);
    json_decref(claims);
}

// Test with null claims
void test_api_create_jwt_null_claims(void) {
    char *result = api_create_jwt(NULL, "secret123");
    
    TEST_ASSERT_NULL(result);
}

// Test with null secret
void test_api_create_jwt_null_secret(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("test_user"));
    
    char *result = api_create_jwt(claims, NULL);
    
    TEST_ASSERT_NULL(result);
    
    json_decref(claims);
}

// Test with both null parameters
void test_api_create_jwt_both_null(void) {
    char *result = api_create_jwt(NULL, NULL);
    
    TEST_ASSERT_NULL(result);
}

// Test with empty claims object
void test_api_create_jwt_empty_claims(void) {
    json_t *claims = json_object();
    
    char *result = api_create_jwt(claims, "secret123");
    
    TEST_ASSERT_NOT_NULL(result);  // Should still return dummy token
    
    free(result);
    json_decref(claims);
}

// Test with empty secret
void test_api_create_jwt_empty_secret(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("test_user"));
    
    char *result = api_create_jwt(claims, "");
    
    TEST_ASSERT_NOT_NULL(result);  // Should still return dummy token
    
    free(result);
    json_decref(claims);
}

// Test with complex claims object
void test_api_create_jwt_complex_claims(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("user123"));
    json_object_set_new(claims, "iss", json_string("hydrogen"));
    json_object_set_new(claims, "exp", json_integer(1234567890));
    json_object_set_new(claims, "iat", json_integer(1234567890));
    json_object_set_new(claims, "aud", json_string("api.example.com"));
    json_object_set_new(claims, "scope", json_string("read write"));
    
    char *result = api_create_jwt(claims, "complex_secret_key");
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Should return the same dummy token regardless of claims complexity
    TEST_ASSERT_EQUAL_STRING("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkR1bW15IFRva2VuIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c", result);
    
    free(result);
    json_decref(claims);
}

// Test with very long secret
void test_api_create_jwt_long_secret(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("test"));
    
    char long_secret[1000];
    memset(long_secret, 's', sizeof(long_secret) - 1);
    long_secret[sizeof(long_secret) - 1] = '\0';
    
    char *result = api_create_jwt(claims, long_secret);
    
    TEST_ASSERT_NOT_NULL(result);
    
    free(result);
    json_decref(claims);
}

// Test with nested JSON in claims
void test_api_create_jwt_nested_claims(void) {
    json_t *claims = json_object();
    json_t *nested = json_object();
    
    json_object_set_new(nested, "role", json_string("admin"));
    json_object_set_new(nested, "permissions", json_string("all"));
    
    json_object_set_new(claims, "sub", json_string("admin_user"));
    json_object_set_new(claims, "custom", nested);
    
    char *result = api_create_jwt(claims, "secret");
    
    TEST_ASSERT_NOT_NULL(result);
    
    free(result);
    json_decref(claims);
}

// Test with array values in claims
void test_api_create_jwt_array_claims(void) {
    json_t *claims = json_object();
    json_t *roles = json_array();
    
    json_array_append_new(roles, json_string("admin"));
    json_array_append_new(roles, json_string("user"));
    json_array_append_new(roles, json_string("viewer"));
    
    json_object_set_new(claims, "sub", json_string("multi_role_user"));
    json_object_set_new(claims, "roles", roles);
    
    char *result = api_create_jwt(claims, "array_secret");
    
    TEST_ASSERT_NOT_NULL(result);
    
    free(result);
    json_decref(claims);
}

// Test multiple calls return same result (consistent behavior)
void test_api_create_jwt_consistency(void) {
    json_t *claims1 = json_object();
    json_object_set_new(claims1, "sub", json_string("user1"));
    
    json_t *claims2 = json_object();
    json_object_set_new(claims2, "sub", json_string("user2"));
    
    char *result1 = api_create_jwt(claims1, "secret1");
    char *result2 = api_create_jwt(claims2, "secret2");
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Since it's a stub, both should return the same dummy token
    TEST_ASSERT_EQUAL_STRING(result1, result2);
    
    free(result1);
    free(result2);
    json_decref(claims1);
    json_decref(claims2);
}

// Test with claims containing special characters
void test_api_create_jwt_special_characters(void) {
    json_t *claims = json_object();
    json_object_set_new(claims, "sub", json_string("user@example.com"));
    json_object_set_new(claims, "name", json_string("John Doe & Jane Smith"));
    json_object_set_new(claims, "note", json_string("Special chars: !@#$%^&*()"));
    
    char *result = api_create_jwt(claims, "special_secret!@#");
    
    TEST_ASSERT_NOT_NULL(result);
    
    free(result);
    json_decref(claims);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_create_jwt_valid_inputs);
    RUN_TEST(test_api_create_jwt_null_claims);
    RUN_TEST(test_api_create_jwt_null_secret);
    RUN_TEST(test_api_create_jwt_both_null);
    RUN_TEST(test_api_create_jwt_empty_claims);
    RUN_TEST(test_api_create_jwt_empty_secret);
    RUN_TEST(test_api_create_jwt_complex_claims);
    RUN_TEST(test_api_create_jwt_long_secret);
    RUN_TEST(test_api_create_jwt_nested_claims);
    RUN_TEST(test_api_create_jwt_array_claims);
    RUN_TEST(test_api_create_jwt_consistency);
    RUN_TEST(test_api_create_jwt_special_characters);
    
    return UNITY_END();
}
