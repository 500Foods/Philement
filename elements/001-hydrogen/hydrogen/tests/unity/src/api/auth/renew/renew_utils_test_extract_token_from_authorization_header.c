/*
 * Unity Unit Tests for renew_utils.c - extract_token_from_authorization_header
 * 
 * Tests the token extraction functionality from Authorization headers
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for token extraction using mocks
 * 
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Mock definitions must come before source includes
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/auth/renew/renew_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

void test_extract_token_from_authorization_header_missing_header(void);
void test_extract_token_from_authorization_header_invalid_format(void);
void test_extract_token_from_authorization_header_empty_token(void);
void test_extract_token_from_authorization_header_valid_token(void);

// ============================================================================
// Test Functions
// ============================================================================

void test_extract_token_from_authorization_header_missing_header(void) {
    // Mock MHD to return NULL for Authorization header
    mock_mhd_set_lookup_result(NULL);
    
    char* token = extract_token_from_authorization_header(NULL);
    
    TEST_ASSERT_NULL(token);
}

void test_extract_token_from_authorization_header_invalid_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    
    // Mock MHD to return invalid Authorization header
    mock_mhd_set_lookup_result("InvalidFormat token123");
    
    char* token = extract_token_from_authorization_header(mock_connection);
    
    TEST_ASSERT_NULL(token);
}

void test_extract_token_from_authorization_header_empty_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    
    // Mock MHD to return Authorization header with empty token
    mock_mhd_set_lookup_result("Bearer ");
    
    char* token = extract_token_from_authorization_header(mock_connection);
    
    TEST_ASSERT_NULL(token);
}

void test_extract_token_from_authorization_header_valid_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    
    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");
    
    char* token = extract_token_from_authorization_header(mock_connection);
    
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL_STRING("valid.jwt.token", token);
    
    free(token);
}

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_extract_token_from_authorization_header_missing_header);
    RUN_TEST(test_extract_token_from_authorization_header_invalid_format);
    RUN_TEST(test_extract_token_from_authorization_header_empty_token);
    RUN_TEST(test_extract_token_from_authorization_header_valid_token);
    
    return UNITY_END();
}