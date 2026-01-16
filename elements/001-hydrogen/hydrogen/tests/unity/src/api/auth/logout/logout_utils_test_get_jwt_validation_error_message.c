/*
 * Unity Unit Tests for logout_utils.c - get_jwt_validation_error_message
 *
 * Tests the get_jwt_validation_error_message function
 *
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for get_jwt_validation_error_message
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
void test_get_jwt_validation_error_message_expired(void);
void test_get_jwt_validation_error_message_not_yet_valid(void);
void test_get_jwt_validation_error_message_invalid_signature(void);
void test_get_jwt_validation_error_message_unsupported_algorithm(void);
void test_get_jwt_validation_error_message_invalid_format(void);
void test_get_jwt_validation_error_message_revoked(void);
void test_get_jwt_validation_error_message_none(void);
void test_get_jwt_validation_error_message_unknown(void);

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

// Test: JWT_ERROR_EXPIRED
void test_get_jwt_validation_error_message_expired(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_EXPIRED);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Token has expired", message);
}

// Test: JWT_ERROR_NOT_YET_VALID
void test_get_jwt_validation_error_message_not_yet_valid(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_NOT_YET_VALID);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Token not yet valid", message);
}

// Test: JWT_ERROR_INVALID_SIGNATURE
void test_get_jwt_validation_error_message_invalid_signature(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_INVALID_SIGNATURE);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Invalid token signature", message);
}

// Test: JWT_ERROR_UNSUPPORTED_ALGORITHM
void test_get_jwt_validation_error_message_unsupported_algorithm(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_UNSUPPORTED_ALGORITHM);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Unsupported token algorithm", message);
}

// Test: JWT_ERROR_INVALID_FORMAT
void test_get_jwt_validation_error_message_invalid_format(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_INVALID_FORMAT);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Invalid token format", message);
}

// Test: JWT_ERROR_REVOKED
void test_get_jwt_validation_error_message_revoked(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_REVOKED);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Token already revoked", message);
}

// Test: JWT_ERROR_NONE (should return "Unknown error")
void test_get_jwt_validation_error_message_none(void) {
    const char* message = get_jwt_validation_error_message(JWT_ERROR_NONE);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Unknown error", message);
}

// Test: Unknown error code (should return "Invalid token")
void test_get_jwt_validation_error_message_unknown(void) {
    const char* message = get_jwt_validation_error_message((jwt_error_t)999);
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Invalid token", message);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_get_jwt_validation_error_message_expired);
    RUN_TEST(test_get_jwt_validation_error_message_not_yet_valid);
    RUN_TEST(test_get_jwt_validation_error_message_invalid_signature);
    RUN_TEST(test_get_jwt_validation_error_message_unsupported_algorithm);
    RUN_TEST(test_get_jwt_validation_error_message_invalid_format);
    RUN_TEST(test_get_jwt_validation_error_message_revoked);
    RUN_TEST(test_get_jwt_validation_error_message_none);
    RUN_TEST(test_get_jwt_validation_error_message_unknown);
    
    return UNITY_END();
}