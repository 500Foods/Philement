/*
 * Unity Test File: Auth Service JWT Cleanup Functions Tests
 * This file contains unit tests for cleanup functions in auth_service_jwt.c
 *
 * Tests: free_jwt_claims(), free_jwt_validation_result()
 *
 * CHANGELOG:
 * 2026-01-12: Initial version - Tests for JWT cleanup functions
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <string.h>

// Function prototypes for test functions
void test_free_jwt_claims_null_pointer(void);
void test_free_jwt_claims_empty_structure(void);
void test_free_jwt_claims_with_allocated_strings(void);
void test_free_jwt_claims_with_all_fields(void);
void test_free_jwt_validation_result_null_pointer(void);
void test_free_jwt_validation_result_no_claims(void);
void test_free_jwt_validation_result_with_claims(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test that free_jwt_claims handles NULL pointer gracefully
void test_free_jwt_claims_null_pointer(void) {
    // Should not crash
    free_jwt_claims(NULL);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_claims handles empty structure
void test_free_jwt_claims_empty_structure(void) {
    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    
    // Should not crash with zeroed structure
    free_jwt_claims(claims);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_claims handles allocated strings
void test_free_jwt_claims_with_allocated_strings(void) {
    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    
    // Allocate some string fields
    claims->iss = strdup("hydrogen-auth");
    claims->sub = strdup("123");
    claims->username = strdup("testuser");
    claims->email = strdup("test@example.com");
    
    // Should free all strings and structure
    free_jwt_claims(claims);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_claims handles all fields
void test_free_jwt_claims_with_all_fields(void) {
    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    
    // Allocate all string fields
    claims->iss = strdup("hydrogen-auth");
    claims->sub = strdup("123");
    claims->aud = strdup("456");
    claims->jti = strdup("unique-id");
    claims->username = strdup("testuser");
    claims->email = strdup("test@example.com");
    claims->roles = strdup("user,admin");
    claims->ip = strdup("192.168.1.1");
    claims->tz = strdup("UTC");
    claims->database = strdup("Acuranzo");
    
    // Set numeric fields
    claims->exp = 1234567890;
    claims->iat = 1234567800;
    claims->nbf = 1234567800;
    claims->user_id = 123;
    claims->system_id = 456;
    claims->app_id = 789;
    claims->tzoffset = 0;
    
    // Should free all strings and structure
    free_jwt_claims(claims);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_validation_result handles NULL pointer
void test_free_jwt_validation_result_null_pointer(void) {
    // Should not crash
    free_jwt_validation_result(NULL);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_validation_result handles result without claims
void test_free_jwt_validation_result_no_claims(void) {
    jwt_validation_result_t* result = calloc(1, sizeof(jwt_validation_result_t));
    TEST_ASSERT_NOT_NULL(result);
    
    result->valid = false;
    result->error = JWT_ERROR_INVALID_FORMAT;
    result->claims = NULL;
    
    // Should not crash with NULL claims
    free_jwt_validation_result(result);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

// Test that free_jwt_validation_result handles result with claims
void test_free_jwt_validation_result_with_claims(void) {
    jwt_validation_result_t* result = calloc(1, sizeof(jwt_validation_result_t));
    TEST_ASSERT_NOT_NULL(result);
    
    result->valid = true;
    result->error = JWT_ERROR_NONE;
    
    // Allocate claims
    result->claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(result->claims);
    
    // Allocate some string fields in claims
    result->claims->username = strdup("testuser");
    result->claims->email = strdup("test@example.com");
    result->claims->database = strdup("Acuranzo");
    
    // Should free claims and result
    free_jwt_validation_result(result);
    
    // If we get here, test passed
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_free_jwt_claims_null_pointer);
    RUN_TEST(test_free_jwt_claims_empty_structure);
    RUN_TEST(test_free_jwt_claims_with_allocated_strings);
    RUN_TEST(test_free_jwt_claims_with_all_fields);
    RUN_TEST(test_free_jwt_validation_result_null_pointer);
    RUN_TEST(test_free_jwt_validation_result_no_claims);
    RUN_TEST(test_free_jwt_validation_result_with_claims);
    
    return UNITY_END();
}
