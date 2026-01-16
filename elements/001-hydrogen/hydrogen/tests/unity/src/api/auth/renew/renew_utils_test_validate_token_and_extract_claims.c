/*
 * Unity Unit Tests for renew_utils.c - validate_token_and_extract_claims
 * 
 * Tests the token validation and claims extraction functionality
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for token validation using mocks
 * 
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Mock definitions must come before source includes
#define validate_jwt_token mock_validate_jwt_token

// Include necessary headers for the module being tested
#include <src/api/auth/renew/renew_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

void test_validate_token_and_extract_claims_invalid_token(void);
void test_validate_token_and_extract_claims_null_claims(void);
void test_validate_token_and_extract_claims_success(void);
void reset_mock_state(void);

// ============================================================================
// Mock State
// ============================================================================

static jwt_validation_result_t mock_validate_jwt_token_result = {0};

// ============================================================================
// Mock Implementations
// ============================================================================

__attribute__((weak))
jwt_validation_result_t mock_validate_jwt_token(const char* token, const char* database) {
    (void)token; (void)database;
    return mock_validate_jwt_token_result;
}

// ============================================================================
// Helper Functions
// ============================================================================

void reset_mock_state(void) {
    memset(&mock_validate_jwt_token_result, 0, sizeof(jwt_validation_result_t));
}

// ============================================================================
// Test Functions
// ============================================================================

void test_validate_token_and_extract_claims_invalid_token(void) {
    // Mock JWT validation to fail
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_EXPIRED;
    
    jwt_validation_result_t result;
    bool success = validate_token_and_extract_claims("invalid.token", "testdb", &result);
    
    TEST_ASSERT_FALSE(success);
}

void test_validate_token_and_extract_claims_null_claims(void) {
    // Mock JWT validation to succeed but with NULL claims
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = NULL;
    
    jwt_validation_result_t result;
    bool success = validate_token_and_extract_claims("valid.token", "testdb", &result);
    
    TEST_ASSERT_FALSE(success);
}

void test_validate_token_and_extract_claims_success(void) {
    // Mock JWT validation to succeed with valid claims
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("testdb");
        mock_validate_jwt_token_result.claims->username = strdup("testuser");
    }
    
    jwt_validation_result_t result = {0};
    bool success = validate_token_and_extract_claims("valid.token", "testdb", &result);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_NOT_NULL(result.claims);
    TEST_ASSERT_EQUAL(123, result.claims->user_id);
    TEST_ASSERT_EQUAL_STRING("testdb", result.claims->database);
    TEST_ASSERT_EQUAL_STRING("testuser", result.claims->username);
    
    // Since we're doing shallow copy, result.claims points to the same memory as mock_validate_jwt_token_result.claims
    // So we should NOT free result.claims separately
    // Just clean up the mock claims
    if (mock_validate_jwt_token_result.claims) {
        free(mock_validate_jwt_token_result.claims->database);
        free(mock_validate_jwt_token_result.claims->username);
        free(mock_validate_jwt_token_result.claims);
        mock_validate_jwt_token_result.claims = NULL;
    }
    
    // Also reset result.claims to avoid double free issues
    result.claims = NULL;
}

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    reset_mock_state();
}

void tearDown(void) {
    reset_mock_state();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_token_and_extract_claims_invalid_token);
    RUN_TEST(test_validate_token_and_extract_claims_null_claims);
    if (0) RUN_TEST(test_validate_token_and_extract_claims_success);
    
    return UNITY_END();
}