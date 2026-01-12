/*
 * Unity Test File: API Utils api_extract_jwt_claims Function Tests
 * This file contains unit tests for the api_extract_jwt_claims function in api_utils.c
 * 
 * Target coverage: Lines 127-137 in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mock for MHD functions
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // Reset mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test functions
void test_api_extract_jwt_claims_no_auth_header(void);
void test_api_extract_jwt_claims_not_bearer_token(void);
void test_api_extract_jwt_claims_valid_bearer_token(void);
void test_api_extract_jwt_claims_bearer_with_valid_token(void);
void test_api_extract_jwt_claims_bearer_case_sensitive(void);

// Test when no Authorization header is present
void test_api_extract_jwt_claims_no_auth_header(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Mock returns NULL for missing header
    mock_mhd_set_lookup_result(NULL);
    
    json_t *result = api_extract_jwt_claims(connection, "test_secret");
    
    TEST_ASSERT_NULL(result);
}

// Test when Authorization header doesn't start with "Bearer "
void test_api_extract_jwt_claims_not_bearer_token(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Mock returns header without "Bearer " prefix
    mock_mhd_set_lookup_result("Basic dGVzdDp0ZXN0");
    
    json_t *result = api_extract_jwt_claims(connection, "test_secret");
    
    // Line 132: should return NULL
    TEST_ASSERT_NULL(result);
}

// Test with valid Bearer token
void test_api_extract_jwt_claims_valid_bearer_token(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Mock returns valid Bearer token
    mock_mhd_set_lookup_result("Bearer test.jwt.token");
    
    json_t *result = api_extract_jwt_claims(connection, "test_secret");
    
    // Lines 134, 136: should process Bearer token
    TEST_ASSERT_NOT_NULL(result);
    
    // Verify it's a JSON object with expected claims
    TEST_ASSERT_TRUE(json_is_object(result));
    
    json_decref(result);
}

// Test Bearer token extraction and validation
void test_api_extract_jwt_claims_bearer_with_valid_token(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Mock returns Bearer with token
    mock_mhd_set_lookup_result("Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9");
    
    json_t *result = api_extract_jwt_claims(connection, "my_secret");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    
    // Verify expected claims are present
    json_t *sub = json_object_get(result, "sub");
    TEST_ASSERT_NOT_NULL(sub);
    
    json_decref(result);
}

// Test with different Bearer prefixes
void test_api_extract_jwt_claims_bearer_case_sensitive(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;
    
    // Test that "bearer" (lowercase) doesn't match
    mock_mhd_set_lookup_result("bearer test.token");
    
    json_t *result = api_extract_jwt_claims(connection, "test_secret");
    
    // Should return NULL because it's not "Bearer " with capital B
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_extract_jwt_claims_no_auth_header);
    RUN_TEST(test_api_extract_jwt_claims_not_bearer_token);
    RUN_TEST(test_api_extract_jwt_claims_valid_bearer_token);
    RUN_TEST(test_api_extract_jwt_claims_bearer_with_valid_token);
    RUN_TEST(test_api_extract_jwt_claims_bearer_case_sensitive);
    
    return UNITY_END();
}
