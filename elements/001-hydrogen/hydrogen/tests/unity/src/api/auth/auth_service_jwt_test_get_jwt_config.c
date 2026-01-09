/*
 * Unity Unit Tests for auth_service_jwt.c - get_jwt_config()
 *
 * Tests the JWT configuration loading functionality
 *
 * CHANGELOG:
 * 2026-01-09: Initial version - Tests for JWT config retrieval
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>

// Function prototypes for test functions
void test_get_jwt_config_returns_non_null(void);
void test_get_jwt_config_has_hmac_secret(void);
void test_get_jwt_config_default_use_rsa_false(void);
void test_get_jwt_config_has_rotation_interval(void);
void test_get_jwt_config_multiple_calls_independent(void);
void test_get_jwt_config_memory_allocation(void);

/* Test Setup and Teardown */
void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

/* Test 1: get_jwt_config returns a non-null config */
void test_get_jwt_config_returns_non_null(void) {
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    
    free_jwt_config(config);
}

/* Test 2: JWT config has HMAC secret set */
void test_get_jwt_config_has_hmac_secret(void) {
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    TEST_ASSERT_GREATER_THAN(0, strlen(config->hmac_secret));
    
    free_jwt_config(config);
}

/* Test 3: Default configuration has use_rsa = false */
void test_get_jwt_config_default_use_rsa_false(void) {
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_FALSE(config->use_rsa);
    
    free_jwt_config(config);
}

/* Test 4: JWT config has rotation interval set */
void test_get_jwt_config_has_rotation_interval(void) {
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    // Default is 90 days
    TEST_ASSERT_EQUAL(90, config->rotation_interval_days);
    
    free_jwt_config(config);
}

/* Test 5: Multiple calls return independent config structures */
void test_get_jwt_config_multiple_calls_independent(void) {
    jwt_config_t* config1 = get_jwt_config();
    jwt_config_t* config2 = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config1);
    TEST_ASSERT_NOT_NULL(config2);
    
    // They should be different pointers (independent allocations)
    TEST_ASSERT_NOT_EQUAL(config1, config2);
    
    // But should have the same values
    TEST_ASSERT_EQUAL_STRING(config1->hmac_secret, config2->hmac_secret);
    TEST_ASSERT_EQUAL(config1->use_rsa, config2->use_rsa);
    TEST_ASSERT_EQUAL(config1->rotation_interval_days, config2->rotation_interval_days);
    
    free_jwt_config(config1);
    free_jwt_config(config2);
}

/* Test 6: Verify memory allocation for config structure */
void test_get_jwt_config_memory_allocation(void) {
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    
    // Verify hmac_secret is allocated separately (can be freed)
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    
    // RSA keys should be NULL in default config
    TEST_ASSERT_NULL(config->rsa_private_key);
    TEST_ASSERT_NULL(config->rsa_public_key);
    
    free_jwt_config(config);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_get_jwt_config_returns_non_null);
    RUN_TEST(test_get_jwt_config_has_hmac_secret);
    RUN_TEST(test_get_jwt_config_default_use_rsa_false);
    RUN_TEST(test_get_jwt_config_has_rotation_interval);
    RUN_TEST(test_get_jwt_config_multiple_calls_independent);
    RUN_TEST(test_get_jwt_config_memory_allocation);
    
    return UNITY_END();
}
