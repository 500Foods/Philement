/*
 * Unity Test File: Web Server Request - Config Utils Test
 * This file contains unit tests for config utility functions
 * that can be tested without system dependencies
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_utils.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_is_sensitive_value_null_name(void) {
    // Test with null name parameter
    bool result = is_sensitive_value(NULL);
    TEST_ASSERT_FALSE(result);
}

static void test_is_sensitive_value_empty_name(void) {
    // Test with empty name
    bool result = is_sensitive_value("");
    TEST_ASSERT_FALSE(result);
}

static void test_is_sensitive_value_non_sensitive(void) {
    // Test with non-sensitive value names
    TEST_ASSERT_FALSE(is_sensitive_value("database_host"));
    TEST_ASSERT_FALSE(is_sensitive_value("server_port"));
    TEST_ASSERT_FALSE(is_sensitive_value("log_level"));
}

static void test_is_sensitive_value_sensitive_password(void) {
    // Test with sensitive password values
    TEST_ASSERT_TRUE(is_sensitive_value("password"));
    TEST_ASSERT_TRUE(is_sensitive_value("PASSWORD"));
    TEST_ASSERT_TRUE(is_sensitive_value("db_password"));
    TEST_ASSERT_TRUE(is_sensitive_value("admin_password"));
}

static void test_is_sensitive_value_sensitive_key(void) {
    // Test with sensitive key values
    TEST_ASSERT_TRUE(is_sensitive_value("key"));
    TEST_ASSERT_TRUE(is_sensitive_value("KEY"));
    TEST_ASSERT_TRUE(is_sensitive_value("secret_key"));
    TEST_ASSERT_TRUE(is_sensitive_value("api_key"));
    TEST_ASSERT_TRUE(is_sensitive_value("private_key"));
}

static void test_is_sensitive_value_sensitive_token(void) {
    // Test with sensitive token values
    TEST_ASSERT_TRUE(is_sensitive_value("token"));
    TEST_ASSERT_TRUE(is_sensitive_value("TOKEN"));
    TEST_ASSERT_TRUE(is_sensitive_value("auth_token"));
    TEST_ASSERT_TRUE(is_sensitive_value("access_token"));
}

static void test_is_sensitive_value_case_insensitive(void) {
    // Test case insensitive matching
    TEST_ASSERT_TRUE(is_sensitive_value("PASSWORD"));
    TEST_ASSERT_TRUE(is_sensitive_value("Password"));
    TEST_ASSERT_TRUE(is_sensitive_value("password"));
    TEST_ASSERT_TRUE(is_sensitive_value("PaSsWoRd"));
}

static void test_is_sensitive_value_partial_matches(void) {
    // Test partial matches in names
    TEST_ASSERT_TRUE(is_sensitive_value("user_password"));
    TEST_ASSERT_TRUE(is_sensitive_value("password_field"));
    TEST_ASSERT_TRUE(is_sensitive_value("my_secret_key"));
    TEST_ASSERT_TRUE(is_sensitive_value("token_value"));
}

static void test_is_sensitive_value_boundary_cases(void) {
    // Test boundary cases - based on actual implementation that looks for sensitive terms
    TEST_ASSERT_TRUE(is_sensitive_value("passwor")); // contains 'pass' which is sensitive
    TEST_ASSERT_FALSE(is_sensitive_value("assword")); // missing 'p', no sensitive terms
    TEST_ASSERT_TRUE(is_sensitive_value("pass")); // contains 'pass' which is sensitive
    TEST_ASSERT_TRUE(is_sensitive_value("key_")); // contains 'key' which is sensitive
    TEST_ASSERT_TRUE(is_sensitive_value("_key")); // contains 'key' which is sensitive
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_sensitive_value_null_name);
    RUN_TEST(test_is_sensitive_value_empty_name);
    RUN_TEST(test_is_sensitive_value_non_sensitive);
    RUN_TEST(test_is_sensitive_value_sensitive_password);
    RUN_TEST(test_is_sensitive_value_sensitive_key);
    RUN_TEST(test_is_sensitive_value_sensitive_token);
    RUN_TEST(test_is_sensitive_value_case_insensitive);
    RUN_TEST(test_is_sensitive_value_partial_matches);
    RUN_TEST(test_is_sensitive_value_boundary_cases);

    return UNITY_END();
}
