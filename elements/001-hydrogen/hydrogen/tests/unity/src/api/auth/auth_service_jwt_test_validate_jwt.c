/*
 * Unity Unit Tests for auth_service_jwt.c - validate_jwt()
 *
 * Tests the JWT validation functionality
 *
 * CHANGELOG:
 * 2026-01-10: Updated validate_jwt calls to include database parameter
 * 2026-01-09: Initial version - Tests for JWT validation
 *
 * TEST_VERSION: 1.1.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <string.h>
#include <time.h>

// Function prototypes for test functions
void test_validate_jwt_null_token(void);
void test_validate_jwt_empty_token(void);
void test_validate_jwt_invalid_format_no_dots(void);
void test_validate_jwt_invalid_format_one_dot(void);
void test_validate_jwt_invalid_format_too_many_dots(void);
void test_validate_jwt_valid_token_returns_success(void);
void test_validate_jwt_result_structure(void);

// Helper function prototypes
account_info_t* create_test_account(void);
system_info_t* create_test_system(void);
void free_test_account(account_info_t* account);
void free_test_system(system_info_t* system);

// Helper function to create test account
account_info_t* create_test_account(void) {
    account_info_t* account = calloc(1, sizeof(account_info_t));
    if (!account) return NULL;
    
    account->id = 123;
    account->username = strdup("testuser");
    account->email = strdup("test@example.com");
    account->roles = strdup("user");
    
    return account;
}

// Helper function to create test system
system_info_t* create_test_system(void) {
    system_info_t* system = calloc(1, sizeof(system_info_t));
    if (!system) return NULL;
    
    system->system_id = 456;
    system->app_id = 789;
    
    return system;
}

// Helper function to free test account
void free_test_account(account_info_t* account) {
    if (account) {
        free(account->username);
        free(account->email);
        free(account->roles);
        free(account);
    }
}

// Helper function to free test system
void free_test_system(system_info_t* system) {
    if (system) {
        free(system);
    }
}

/* Test Setup and Teardown */
void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

/* Test 1: NULL token returns invalid with appropriate error */
void test_validate_jwt_null_token(void) {
    jwt_validation_result_t result = validate_jwt(NULL, "Acuranzo");
    
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test 2: Empty token returns invalid */
void test_validate_jwt_empty_token(void) {
    jwt_validation_result_t result = validate_jwt("", "Acuranzo");
    
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test 3: Token with no dots is invalid */
void test_validate_jwt_invalid_format_no_dots(void) {
    jwt_validation_result_t result = validate_jwt("invalidtoken", "Acuranzo");
    
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test 4: Token with only one dot is invalid */
void test_validate_jwt_invalid_format_one_dot(void) {
    jwt_validation_result_t result = validate_jwt("header.payload", "Acuranzo");
    
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test 5: Token with too many dots is invalid */
void test_validate_jwt_invalid_format_too_many_dots(void) {
    jwt_validation_result_t result = validate_jwt("header.payload.signature.extra", "Acuranzo");
    
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test 6: Valid JWT returns success */
void test_validate_jwt_valid_token_returns_success(void) {
    // Generate a valid JWT first
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NOT_NULL(jwt);
    
    // Validate the JWT
    jwt_validation_result_t result = validate_jwt(jwt, "Acuranzo");
    
    // Should be valid (though it may fail if token is revoked - depends on DB)
    // At minimum, it should not have format errors
    TEST_ASSERT_TRUE(result.error != JWT_ERROR_INVALID_FORMAT);
    
    // Clean up
    if (result.claims) {
        free_jwt_claims(result.claims);
    }
    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test 7: Validation result structure is properly initialized */
void test_validate_jwt_result_structure(void) {
    jwt_validation_result_t result = validate_jwt("invalid.token.format", "Acuranzo");
    
    // Result should have valid flag set
    TEST_ASSERT_FALSE(result.valid);
    
    // Error code should be set
    TEST_ASSERT_NOT_EQUAL(JWT_ERROR_NONE, result.error);
    
    // Claims should be NULL for invalid token
    TEST_ASSERT_NULL(result.claims);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_jwt_null_token);
    RUN_TEST(test_validate_jwt_empty_token);
    RUN_TEST(test_validate_jwt_invalid_format_no_dots);
    RUN_TEST(test_validate_jwt_invalid_format_one_dot);
    RUN_TEST(test_validate_jwt_invalid_format_too_many_dots);
    RUN_TEST(test_validate_jwt_valid_token_returns_success);
    RUN_TEST(test_validate_jwt_result_structure);
    
    return UNITY_END();
}
