/*
 * Unity Test File: Deprecated Functions in auth_service_database.c
 * This file tests the deprecated functions to ensure they log warnings
 * and return appropriate failure values.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>

// Forward declarations for functions being tested
char* get_password_hash(int account_id, const char* database);
bool verify_password(const char* password, const char* stored_hash, int account_id);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

/**
 * Test: get_password_hash_returns_null
 * Purpose: Verify deprecated get_password_hash() returns NULL and logs warning
 */
void test_get_password_hash_returns_null(void) {
    char* result = get_password_hash(1, "test_db");
    TEST_ASSERT_NULL(result);
}

/**
 * Test: get_password_hash_with_zero_account_id
 * Purpose: Verify deprecated function handles zero account_id
 */
void test_get_password_hash_with_zero_account_id(void) {
    char* result = get_password_hash(0, "test_db");
    TEST_ASSERT_NULL(result);
}

/**
 * Test: get_password_hash_with_null_database
 * Purpose: Verify deprecated function handles null database
 */
void test_get_password_hash_with_null_database(void) {
    char* result = get_password_hash(1, NULL);
    TEST_ASSERT_NULL(result);
}

/**
 * Test: verify_password_returns_false
 * Purpose: Verify deprecated verify_password() returns false and logs warning
 */
void test_verify_password_returns_false(void) {
    bool result = verify_password("password123", "hashed_value", 1);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_with_null_password
 * Purpose: Verify deprecated function handles null password
 */
void test_verify_password_with_null_password(void) {
    bool result = verify_password(NULL, "hashed_value", 1);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_with_null_hash
 * Purpose: Verify deprecated function handles null hash
 */
void test_verify_password_with_null_hash(void) {
    bool result = verify_password("password123", NULL, 1);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_with_zero_account_id
 * Purpose: Verify deprecated function handles zero account_id
 */
void test_verify_password_with_zero_account_id(void) {
    bool result = verify_password("password123", "hashed_value", 0);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_with_all_null_params
 * Purpose: Verify deprecated function handles all null parameters
 */
void test_verify_password_with_all_null_params(void) {
    bool result = verify_password(NULL, NULL, 0);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test deprecated get_password_hash function
    RUN_TEST(test_get_password_hash_returns_null);
    RUN_TEST(test_get_password_hash_with_zero_account_id);
    RUN_TEST(test_get_password_hash_with_null_database);

    // Test deprecated verify_password function
    RUN_TEST(test_verify_password_returns_false);
    RUN_TEST(test_verify_password_with_null_password);
    RUN_TEST(test_verify_password_with_null_hash);
    RUN_TEST(test_verify_password_with_zero_account_id);
    RUN_TEST(test_verify_password_with_all_null_params);

    return UNITY_END();
}
