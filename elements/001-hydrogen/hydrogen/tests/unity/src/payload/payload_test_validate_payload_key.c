/*
 * Unity Test File: validate_payload_key Function Tests
 * This file contains unit tests for the validate_payload_key() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
bool validate_payload_key(const char *key);

// Function prototypes for test functions
void timeout_handler(int sig);
void test_validate_payload_key_null_key(void);
void test_validate_payload_key_empty_key(void);
void test_validate_payload_key_missing_key(void);
void test_validate_payload_key_valid_direct_key(void);
void test_validate_payload_key_valid_env_var_existing(void);
void test_validate_payload_key_invalid_env_var_missing(void);
void test_validate_payload_key_invalid_env_var_empty(void);
void test_validate_payload_key_malformed_env_var_no_closing_brace(void);
void test_validate_payload_key_malformed_env_var_no_name(void);
void test_validate_payload_key_malformed_env_var_too_long(void);
void test_validate_payload_key_boundary_length(void);
void test_parameter_validation_patterns(void);
void test_env_var_pattern_matching(void);

// Timeout handler for environment variable tests
static volatile sig_atomic_t test_timeout = 0;

void timeout_handler(int sig) {
    (void)sig;
    test_timeout = 1;
}

void setUp(void) {
    test_timeout = 0;
}

void tearDown(void) {
    // Reset timeout state
    test_timeout = 0;
    signal(SIGALRM, SIG_DFL);
    alarm(0);
}

// Basic parameter validation tests
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
    RUN_TEST(test_parameter_validation_patterns);
    RUN_TEST(test_env_var_pattern_matching);
    
    return UNITY_END();
}
