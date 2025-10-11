/*
 * Unity Test File: cleanup_openssl Function Tests
 * This file contains unit tests for the cleanup_openssl() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void test_cleanup_openssl_basic(void);
void test_cleanup_openssl_multiple_calls(void);
void test_cleanup_openssl_repeated_sequence(void);
void cleanup_openssl(void);

void setUp(void) {
    // No setup needed for cleanup_openssl tests
}

void tearDown(void) {
    // No teardown needed for cleanup_openssl tests
}

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

void test_cleanup_openssl_repeated_sequence(void) {
    // Test repeated cleanup sequences
    for (int i = 0; i < 5; i++) {
        cleanup_openssl();
        cleanup_openssl();
    }
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cleanup_openssl_basic);
    RUN_TEST(test_cleanup_openssl_multiple_calls);
    RUN_TEST(test_cleanup_openssl_repeated_sequence);
    
    return UNITY_END();
}
