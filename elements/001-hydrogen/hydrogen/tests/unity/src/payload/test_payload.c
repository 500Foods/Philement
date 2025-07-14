/*
 * Unity Test File: Payload Module Tests
 * This file contains unit tests for src/payload/payload.c functionality
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>

// Include necessary headers for the payload module
#include "../../../../src/payload/payload.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool validate_payload_key(const char *key);
void free_payload(PayloadData *payload);
void cleanup_openssl(void);

// Test fixtures
static AppConfig test_config;
static PayloadData test_payload;

// Global state variables that payload functions check (declared extern)
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t web_server_shutdown;

// Simple timeout mechanism using alarm
static volatile sig_atomic_t test_timeout = 0;

void timeout_handler(int sig) {
    (void)sig;
    test_timeout = 1;
}

void setUp(void) {
    // Reset timeout flag
    test_timeout = 0;
    
    // Initialize test config
    memset(&test_config, 0, sizeof(AppConfig));
    test_config.server.payload_key = strdup("test_key_12345");
    test_config.server.server_name = strdup("test_server");
    test_config.server.log_file = strdup("/tmp/test.log");
    
    // Initialize test payload
    memset(&test_payload, 0, sizeof(PayloadData));
}

void tearDown(void) {
    // Cancel any pending alarms
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    
    // Clean up test config
    free(test_config.server.payload_key);
    free(test_config.server.server_name);
    free(test_config.server.log_file);
    memset(&test_config, 0, sizeof(AppConfig));
    
    // Clean up test payload
    free_payload(&test_payload);
}

// Test functions for validate_payload_key
void test_validate_payload_key_null_key(void) {
    TEST_ASSERT_FALSE(validate_payload_key(NULL));
}

void test_validate_payload_key_empty_key(void) {
    TEST_ASSERT_FALSE(validate_payload_key(""));
}

void test_validate_payload_key_missing_key(void) {
    TEST_ASSERT_FALSE(validate_payload_key("Missing Key"));
}

void test_validate_payload_key_valid_direct_key(void) {
    TEST_ASSERT_TRUE(validate_payload_key("valid_key_12345"));
}

void test_validate_payload_key_valid_env_var_existing(void) {
    // Set an environment variable for testing
    setenv("TEST_PAYLOAD_KEY", "test_value", 1);
    
    // Set up timeout
    signal(SIGALRM, timeout_handler);
    alarm(2);
    
    bool result = validate_payload_key("${env.TEST_PAYLOAD_KEY}");
    
    // Cancel alarm
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    
    TEST_ASSERT_FALSE(test_timeout);
    TEST_ASSERT_TRUE(result);
    
    unsetenv("TEST_PAYLOAD_KEY");
}

void test_validate_payload_key_invalid_env_var_missing(void) {
    // Ensure the environment variable doesn't exist
    unsetenv("NONEXISTENT_PAYLOAD_KEY");
    
    // Set up timeout
    signal(SIGALRM, timeout_handler);
    alarm(2);
    
    bool result = validate_payload_key("${env.NONEXISTENT_PAYLOAD_KEY}");
    
    // Cancel alarm
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    
    TEST_ASSERT_FALSE(test_timeout);
    TEST_ASSERT_FALSE(result);
}

void test_validate_payload_key_invalid_env_var_empty(void) {
    // Set an empty environment variable
    setenv("EMPTY_PAYLOAD_KEY", "", 1);
    
    // Set up timeout
    signal(SIGALRM, timeout_handler);
    alarm(2);
    
    bool result = validate_payload_key("${env.EMPTY_PAYLOAD_KEY}");
    
    // Cancel alarm
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    
    TEST_ASSERT_FALSE(test_timeout);
    TEST_ASSERT_FALSE(result);
    
    unsetenv("EMPTY_PAYLOAD_KEY");
}

void test_validate_payload_key_malformed_env_var_no_closing_brace(void) {
    TEST_ASSERT_FALSE(validate_payload_key("${env.TEST_KEY"));
}

void test_validate_payload_key_malformed_env_var_no_name(void) {
    TEST_ASSERT_FALSE(validate_payload_key("${env.}"));
}

void test_validate_payload_key_malformed_env_var_too_long(void) {
    // Create a string with a variable name longer than 255 characters
    char long_var[300];
    memset(long_var, 'A', 270);
    long_var[270] = '\0';
    char long_key[320];
    snprintf(long_key, sizeof(long_key), "${env.%s}", long_var);
    TEST_ASSERT_FALSE(validate_payload_key(long_key));
}

void test_validate_payload_key_boundary_length(void) {
    // Test with a 255-character variable name (boundary condition)
    char boundary_var[256];
    memset(boundary_var, 'A', 255);
    boundary_var[255] = '\0';
    
    char boundary_key[270];
    snprintf(boundary_key, sizeof(boundary_key), "${env.%s}", boundary_var);
    
    // This should be false because the environment variable doesn't exist
    TEST_ASSERT_FALSE(validate_payload_key(boundary_key));
}

// Test functions for free_payload
void test_free_payload_null_payload(void) {
    // Should not crash with NULL pointer
    free_payload(NULL);
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_free_payload_empty_payload(void) {
    PayloadData empty_payload = {0};
    free_payload(&empty_payload);
    TEST_ASSERT_NULL(empty_payload.data);
    TEST_ASSERT_EQUAL(0, empty_payload.size);
    TEST_ASSERT_FALSE(empty_payload.is_compressed);
}

void test_free_payload_with_data(void) {
    PayloadData test_payload_local = {0};
    test_payload_local.data = malloc(100);
    test_payload_local.size = 100;
    test_payload_local.is_compressed = true;
    
    free_payload(&test_payload_local);
    
    TEST_ASSERT_NULL(test_payload_local.data);
    TEST_ASSERT_EQUAL(0, test_payload_local.size);
    TEST_ASSERT_FALSE(test_payload_local.is_compressed);
}

// Test functions for cleanup_openssl
void test_cleanup_openssl_basic(void) {
    // Should not crash when called
    cleanup_openssl();
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_cleanup_openssl_multiple_calls(void) {
    // Should handle multiple calls gracefully
    cleanup_openssl();
    cleanup_openssl();
    cleanup_openssl();
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

// Test PayloadData structure
void test_payload_data_structure_initialization(void) {
    PayloadData payload = {0};
    TEST_ASSERT_NULL(payload.data);
    TEST_ASSERT_EQUAL(0, payload.size);
    TEST_ASSERT_FALSE(payload.is_compressed);
}

void test_payload_data_structure_assignment(void) {
    PayloadData payload = {0};
    uint8_t test_data[] = {1, 2, 3, 4, 5};
    
    payload.data = malloc(sizeof(test_data));
    memcpy(payload.data, test_data, sizeof(test_data));
    payload.size = sizeof(test_data);
    payload.is_compressed = true;
    
    TEST_ASSERT_NOT_NULL(payload.data);
    TEST_ASSERT_EQUAL(sizeof(test_data), payload.size);
    TEST_ASSERT_TRUE(payload.is_compressed);
    TEST_ASSERT_EQUAL_MEMORY(test_data, payload.data, sizeof(test_data));
    
    free(payload.data);
}

// Test PAYLOAD_MARKER constant
void test_payload_marker_constant(void) {
    TEST_ASSERT_NOT_NULL(PAYLOAD_MARKER);
    TEST_ASSERT_TRUE(strlen(PAYLOAD_MARKER) > 0);
    TEST_ASSERT_EQUAL_STRING("<<< HERE BE ME TREASURE >>>", PAYLOAD_MARKER);
}

// Test parameter validation patterns
void test_parameter_validation_patterns(void) {
    // Test that null parameters are consistently rejected
    TEST_ASSERT_FALSE(validate_payload_key(NULL));
    
    // Test that empty strings are consistently rejected
    TEST_ASSERT_FALSE(validate_payload_key(""));
    
    // Test that the "Missing Key" sentinel is rejected
    TEST_ASSERT_FALSE(validate_payload_key("Missing Key"));
}

// Test environment variable pattern matching
void test_env_var_pattern_matching(void) {
    // These malformed env var patterns are treated as direct keys (and thus valid)
    TEST_ASSERT_TRUE(validate_payload_key("${env"));      // Treated as direct key
    TEST_ASSERT_TRUE(validate_payload_key("env.TEST}"));   // Treated as direct key
    TEST_ASSERT_TRUE(validate_payload_key("${TEST}"));     // Treated as direct key
    TEST_ASSERT_TRUE(validate_payload_key("$env.TEST"));   // Treated as direct key
    TEST_ASSERT_TRUE(validate_payload_key("{env.TEST}"));  // Treated as direct key
    
    // Properly formatted env var patterns (will fail because vars don't exist)
    TEST_ASSERT_FALSE(validate_payload_key("${env.NONEXISTENT_VAR}"));
}

int main(void) {
    UNITY_BEGIN();
    
    // validate_payload_key tests
    RUN_TEST(test_validate_payload_key_null_key);
    RUN_TEST(test_validate_payload_key_empty_key);
    RUN_TEST(test_validate_payload_key_missing_key);
    RUN_TEST(test_validate_payload_key_valid_direct_key);
    RUN_TEST(test_validate_payload_key_valid_env_var_existing);
    RUN_TEST(test_validate_payload_key_invalid_env_var_missing);
    RUN_TEST(test_validate_payload_key_invalid_env_var_empty);
    RUN_TEST(test_validate_payload_key_malformed_env_var_no_closing_brace);
    RUN_TEST(test_validate_payload_key_malformed_env_var_no_name);
    RUN_TEST(test_validate_payload_key_malformed_env_var_too_long);
    RUN_TEST(test_validate_payload_key_boundary_length);
    
    // free_payload tests
    RUN_TEST(test_free_payload_null_payload);
    RUN_TEST(test_free_payload_empty_payload);
    RUN_TEST(test_free_payload_with_data);
    
    // cleanup_openssl tests
    RUN_TEST(test_cleanup_openssl_basic);
    RUN_TEST(test_cleanup_openssl_multiple_calls);
    
    // PayloadData structure tests
    RUN_TEST(test_payload_data_structure_initialization);
    RUN_TEST(test_payload_data_structure_assignment);
    
    // Additional tests
    RUN_TEST(test_payload_marker_constant);
    RUN_TEST(test_parameter_validation_patterns);
    RUN_TEST(test_env_var_pattern_matching);
    
    return UNITY_END();
}
