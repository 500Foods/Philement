/*
 * Unity Unit Tests for renew_utils.c - get_jwt_validation_error_message
 * 
 * Tests the JWT error message mapping functionality
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for error message mapping
 * 
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/renew/renew_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

void test_get_jwt_validation_error_message_none(void);
void test_get_jwt_validation_error_message_expired(void);
void test_get_jwt_validation_error_message_not_yet_valid(void);
void test_get_jwt_validation_error_message_invalid_signature(void);
void test_get_jwt_validation_error_message_unsupported_algorithm(void);
void test_get_jwt_validation_error_message_invalid_format(void);
void test_get_jwt_validation_error_message_revoked(void);
void test_get_jwt_validation_error_message_unknown(void);

// ============================================================================
// Test Functions
// ============================================================================

void test_get_jwt_validation_error_message_none(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_NONE);
    TEST_ASSERT_EQUAL_STRING("Unknown error", message);
}

void test_get_jwt_validation_error_message_expired(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_EXPIRED);
    TEST_ASSERT_EQUAL_STRING("Token has expired", message);
}

void test_get_jwt_validation_error_message_not_yet_valid(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_NOT_YET_VALID);
    TEST_ASSERT_EQUAL_STRING("Token not yet valid", message);
}

void test_get_jwt_validation_error_message_invalid_signature(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_INVALID_SIGNATURE);
    TEST_ASSERT_EQUAL_STRING("Invalid token signature", message);
}

void test_get_jwt_validation_error_message_unsupported_algorithm(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_UNSUPPORTED_ALGORITHM);
    TEST_ASSERT_EQUAL_STRING("Unsupported token algorithm", message);
}

void test_get_jwt_validation_error_message_invalid_format(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_INVALID_FORMAT);
    TEST_ASSERT_EQUAL_STRING("Invalid token format", message);
}

void test_get_jwt_validation_error_message_revoked(void) {
    const char* message = get_jwt_validation_error_message_renew(JWT_ERROR_REVOKED);
    TEST_ASSERT_EQUAL_STRING("Token has been revoked", message);
}

void test_get_jwt_validation_error_message_unknown(void) {
    // Test with an unknown error code
    const char* message = get_jwt_validation_error_message_renew((jwt_error_t)999);
    TEST_ASSERT_EQUAL_STRING("Invalid or expired token", message);
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
    
    RUN_TEST(test_get_jwt_validation_error_message_none);
    RUN_TEST(test_get_jwt_validation_error_message_expired);
    RUN_TEST(test_get_jwt_validation_error_message_not_yet_valid);
    RUN_TEST(test_get_jwt_validation_error_message_invalid_signature);
    RUN_TEST(test_get_jwt_validation_error_message_unsupported_algorithm);
    RUN_TEST(test_get_jwt_validation_error_message_invalid_format);
    RUN_TEST(test_get_jwt_validation_error_message_revoked);
    RUN_TEST(test_get_jwt_validation_error_message_unknown);
    
    return UNITY_END();
}