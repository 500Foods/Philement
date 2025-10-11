/*
 * Unity Test File: API Utils api_validate_jwt Function Tests
 * This file contains unit tests for the api_validate_jwt function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No teardown needed for this function
}

// Test basic JWT validation with valid inputs
void test_api_validate_jwt_valid_inputs(void);
void test_api_validate_jwt_null_token(void);
void test_api_validate_jwt_null_secret(void);
void test_api_validate_jwt_both_null(void);
void test_api_validate_jwt_empty_token(void);
void test_api_validate_jwt_empty_secret(void);
void test_api_validate_jwt_realistic_token(void);
void test_api_validate_jwt_long_token(void);
void test_api_validate_jwt_long_secret(void);
void test_api_validate_jwt_claim_structure(void);
void test_api_validate_jwt_time_consistency(void);
void test_api_validate_jwt_consistent_results(void);
void test_api_validate_jwt_valid_inputs(void) {
    json_t *result = api_validate_jwt("dummy.jwt.token", "secret123");
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Verify the expected claims are present
    json_t *sub = json_object_get(result, "sub");
    json_t *iss = json_object_get(result, "iss");
    json_t *exp = json_object_get(result, "exp");
    json_t *iat = json_object_get(result, "iat");
    
    TEST_ASSERT_NOT_NULL(sub);
    TEST_ASSERT_NOT_NULL(iss);
    TEST_ASSERT_NOT_NULL(exp);
    TEST_ASSERT_NOT_NULL(iat);
    
    // Verify claim values
    TEST_ASSERT_EQUAL_STRING("system_user", json_string_value(sub));
    TEST_ASSERT_EQUAL_STRING("hydrogen", json_string_value(iss));
    
    // Verify expiration is in the future (within 1 hour)
    time_t now = time(NULL);
    json_int_t exp_time = json_integer_value(exp);
    TEST_ASSERT_TRUE(exp_time > now);
    TEST_ASSERT_TRUE(exp_time <= now + 3600);  // Should be 1 hour from now
    
    // Verify issued at time is reasonable (within last minute)
    json_int_t iat_time = json_integer_value(iat);
    TEST_ASSERT_TRUE(iat_time <= now);
    TEST_ASSERT_TRUE(iat_time >= now - 60);  // Should be recent
    
    json_decref(result);
}

// Test with null token
void test_api_validate_jwt_null_token(void) {
    json_t *result = api_validate_jwt(NULL, "secret123");
    
    TEST_ASSERT_NULL(result);
}

// Test with null secret
void test_api_validate_jwt_null_secret(void) {
    json_t *result = api_validate_jwt("dummy.jwt.token", NULL);
    
    TEST_ASSERT_NULL(result);
}

// Test with both null parameters
void test_api_validate_jwt_both_null(void) {
    json_t *result = api_validate_jwt(NULL, NULL);
    
    TEST_ASSERT_NULL(result);
}

// Test with empty token
void test_api_validate_jwt_empty_token(void) {
    json_t *result = api_validate_jwt("", "secret123");
    
    TEST_ASSERT_NOT_NULL(result);  // Function should still create default claims
    
    // Verify basic claims are still present
    json_t *sub = json_object_get(result, "sub");
    TEST_ASSERT_NOT_NULL(sub);
    TEST_ASSERT_EQUAL_STRING("system_user", json_string_value(sub));
    
    json_decref(result);
}

// Test with empty secret
void test_api_validate_jwt_empty_secret(void) {
    json_t *result = api_validate_jwt("dummy.jwt.token", "");
    
    TEST_ASSERT_NOT_NULL(result);  // Function should still create default claims
    
    // Verify basic claims are still present
    json_t *iss = json_object_get(result, "iss");
    TEST_ASSERT_NOT_NULL(iss);
    TEST_ASSERT_EQUAL_STRING("hydrogen", json_string_value(iss));
    
    json_decref(result);
}

// Test with realistic JWT token format
void test_api_validate_jwt_realistic_token(void) {
    const char *jwt_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";
    
    json_t *result = api_validate_jwt(jwt_token, "secret123");
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Verify the function returns default claims regardless of actual token content
    json_t *sub = json_object_get(result, "sub");
    TEST_ASSERT_EQUAL_STRING("system_user", json_string_value(sub));
    
    json_decref(result);
}

// Test with very long token
void test_api_validate_jwt_long_token(void) {
    char long_token[1000];
    memset(long_token, 'a', sizeof(long_token) - 1);
    long_token[sizeof(long_token) - 1] = '\0';
    
    json_t *result = api_validate_jwt(long_token, "secret123");
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Should still return default claims
    json_t *iss = json_object_get(result, "iss");
    TEST_ASSERT_EQUAL_STRING("hydrogen", json_string_value(iss));
    
    json_decref(result);
}

// Test with very long secret
void test_api_validate_jwt_long_secret(void) {
    char long_secret[1000];
    memset(long_secret, 's', sizeof(long_secret) - 1);
    long_secret[sizeof(long_secret) - 1] = '\0';
    
    json_t *result = api_validate_jwt("dummy.token", long_secret);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Should still return default claims
    json_t *sub = json_object_get(result, "sub");
    TEST_ASSERT_EQUAL_STRING("system_user", json_string_value(sub));
    
    json_decref(result);
}

// Test claim structure integrity
void test_api_validate_jwt_claim_structure(void) {
    json_t *result = api_validate_jwt("test.token", "test_secret");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    
    // Should have exactly 4 claims
    size_t claim_count = json_object_size(result);
    TEST_ASSERT_EQUAL_UINT(4, claim_count);
    
    // Verify all required claims exist and are correct types
    json_t *sub = json_object_get(result, "sub");
    json_t *iss = json_object_get(result, "iss");
    json_t *exp = json_object_get(result, "exp");
    json_t *iat = json_object_get(result, "iat");
    
    TEST_ASSERT_TRUE(json_is_string(sub));
    TEST_ASSERT_TRUE(json_is_string(iss));
    TEST_ASSERT_TRUE(json_is_integer(exp));
    TEST_ASSERT_TRUE(json_is_integer(iat));
    
    json_decref(result);
}

// Test time consistency (exp should be after iat)
void test_api_validate_jwt_time_consistency(void) {
    json_t *result = api_validate_jwt("test.token", "secret");
    
    TEST_ASSERT_NOT_NULL(result);
    
    json_t *exp = json_object_get(result, "exp");
    json_t *iat = json_object_get(result, "iat");
    
    json_int_t exp_time = json_integer_value(exp);
    json_int_t iat_time = json_integer_value(iat);
    
    // Expiration should be after issued time
    TEST_ASSERT_TRUE(exp_time > iat_time);
    
    // Time difference should be approximately 1 hour (3600 seconds)
    json_int_t time_diff = exp_time - iat_time;
    TEST_ASSERT_TRUE(time_diff >= 3599 && time_diff <= 3601);  // Allow 1 second tolerance
    
    json_decref(result);
}

// Test multiple calls return consistent results
void test_api_validate_jwt_consistent_results(void) {
    json_t *result1 = api_validate_jwt("token1", "secret");
    json_t *result2 = api_validate_jwt("token2", "secret");
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Both should have same subject and issuer
    json_t *sub1 = json_object_get(result1, "sub");
    json_t *sub2 = json_object_get(result2, "sub");
    json_t *iss1 = json_object_get(result1, "iss");
    json_t *iss2 = json_object_get(result2, "iss");
    
    TEST_ASSERT_EQUAL_STRING(json_string_value(sub1), json_string_value(sub2));
    TEST_ASSERT_EQUAL_STRING(json_string_value(iss1), json_string_value(iss2));
    
    json_decref(result1);
    json_decref(result2);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_validate_jwt_valid_inputs);
    RUN_TEST(test_api_validate_jwt_null_token);
    RUN_TEST(test_api_validate_jwt_null_secret);
    RUN_TEST(test_api_validate_jwt_both_null);
    RUN_TEST(test_api_validate_jwt_empty_token);
    RUN_TEST(test_api_validate_jwt_empty_secret);
    RUN_TEST(test_api_validate_jwt_realistic_token);
    RUN_TEST(test_api_validate_jwt_long_token);
    RUN_TEST(test_api_validate_jwt_long_secret);
    RUN_TEST(test_api_validate_jwt_claim_structure);
    RUN_TEST(test_api_validate_jwt_time_consistency);
    RUN_TEST(test_api_validate_jwt_consistent_results);
    
    return UNITY_END();
}
