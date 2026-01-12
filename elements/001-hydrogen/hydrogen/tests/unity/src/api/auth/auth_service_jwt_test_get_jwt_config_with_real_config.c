/*
 * Unity Test File: Auth Service JWT get_jwt_config with Real Config Tests
 * This file contains unit tests for get_jwt_config with actual app_config in auth_service_jwt.c
 *
 * Tests: get_jwt_config() with real app_config settings
 *
 * CHANGELOG:
 * 2026-01-12: Initial version - Tests for get_jwt_config with app_config
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <string.h>

// Function prototypes for test functions
void test_get_jwt_config_uses_app_config_secret(void);
void test_get_jwt_config_with_custom_secret(void);
void test_get_jwt_config_logs_secret_length(void);
void test_get_jwt_config_defaults_when_no_app_config(void);

// Store original app_config to restore
static AppConfig* original_app_config = NULL;
static char* test_jwt_secret = NULL;

void setUp(void) {
    // Save original app_config
    original_app_config = app_config;
    test_jwt_secret = NULL;
}

void tearDown(void) {
    // Restore original app_config
    app_config = original_app_config;
    
    // Free test secret if allocated
    if (test_jwt_secret) {
        free(test_jwt_secret);
        test_jwt_secret = NULL;
    }
}

// Test that get_jwt_config uses JWT secret from app_config when available
void test_get_jwt_config_uses_app_config_secret(void) {
    // Create a temporary app_config
    AppConfig temp_config = {0};
    
    // Set a custom JWT secret
    test_jwt_secret = strdup("my-custom-jwt-secret-for-testing");
    temp_config.api.jwt_secret = test_jwt_secret;
    
    // Point app_config to our test config
    app_config = &temp_config;
    
    // Get JWT config
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    
    // Should use our custom secret
    TEST_ASSERT_EQUAL_STRING("my-custom-jwt-secret-for-testing", config->hmac_secret);
    
    // Cleanup
    free_jwt_config(config);
}

// Test with a different custom secret to ensure different values work
void test_get_jwt_config_with_custom_secret(void) {
    // Create a temporary app_config
    AppConfig temp_config = {0};
    
    // Set a different custom JWT secret
    test_jwt_secret = strdup("another-secret-value-123456");
    temp_config.api.jwt_secret = test_jwt_secret;
    
    // Point app_config to our test config
    app_config = &temp_config;
    
    // Get JWT config
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    
    // Should use our custom secret
    TEST_ASSERT_EQUAL_STRING("another-secret-value-123456", config->hmac_secret);
    
    // Cleanup
    free_jwt_config(config);
}

// Test that get_jwt_config logs the secret length (line 75-76)
void test_get_jwt_config_logs_secret_length(void) {
    // Create a temporary app_config
    AppConfig temp_config = {0};
    
    // Set a JWT secret of known length
    test_jwt_secret = strdup("exactly-32-characters-long--");  // 28 chars, let's use something
    temp_config.api.jwt_secret = test_jwt_secret;
    
    // Point app_config to our test config
    app_config = &temp_config;
    
    // Get JWT config - this should log the secret length
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    
    // The log message at line 75-76 should have been called:
    // log_this(SR_AUTH, "Using JWT secret from configuration (length: %zu)", ...)
    // We can't directly verify the log, but we can verify the secret is correct
    TEST_ASSERT_EQUAL(strlen(test_jwt_secret), strlen(config->hmac_secret));
    
    // Cleanup
    free_jwt_config(config);
}

// Test default behavior when app_config is NULL or has no secret
void test_get_jwt_config_defaults_when_no_app_config(void) {
    // Set app_config to NULL
    app_config = NULL;
    
    // Get JWT config
    jwt_config_t* config = get_jwt_config();
    
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    
    // Should use default secret
    TEST_ASSERT_EQUAL_STRING("default-jwt-secret-change-me-in-production", config->hmac_secret);
    
    // Cleanup
    free_jwt_config(config);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_get_jwt_config_uses_app_config_secret);
    RUN_TEST(test_get_jwt_config_with_custom_secret);
    RUN_TEST(test_get_jwt_config_logs_secret_length);
    RUN_TEST(test_get_jwt_config_defaults_when_no_app_config);
    
    return UNITY_END();
}
