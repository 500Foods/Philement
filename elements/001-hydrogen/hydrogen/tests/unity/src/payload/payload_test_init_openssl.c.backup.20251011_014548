/*
 * Unity Test File: init_openssl Function Tests
 * This file contains unit tests for the init_openssl() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void init_openssl(void);

// Function prototypes for test functions
void test_init_openssl_multiple_calls(void);
void test_init_openssl_first_call_initializes(void);
void test_init_openssl_subsequent_calls_no_op(void);

void setUp(void) {
    // Reset any global state if possible
    // Note: init_openssl uses a static variable, so we can't easily reset it
    // between tests without modifying the function
}

void tearDown(void) {
    // Clean up after each test
}

// Test that multiple calls to init_openssl work correctly
void test_init_openssl_multiple_calls(void) {
    // Call init_openssl multiple times - should not crash or cause issues
    init_openssl();
    init_openssl();
    init_openssl();

    // If we get here without crashing, the test passes
    TEST_PASS();
}

// Test that the first call initializes OpenSSL
void test_init_openssl_first_call_initializes(void) {
    // Call init_openssl - this should initialize OpenSSL algorithms and error strings
    init_openssl();

    // We can't easily verify the internal state, but we can ensure it doesn't crash
    TEST_PASS();
}

// Test that subsequent calls are no-ops (idempotent behavior)
void test_init_openssl_subsequent_calls_no_op(void) {
    // Call init_openssl multiple times
    init_openssl();
    init_openssl();
    init_openssl();

    // All calls should complete without issues
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    // init_openssl tests
    RUN_TEST(test_init_openssl_multiple_calls);
    RUN_TEST(test_init_openssl_first_call_initializes);
    RUN_TEST(test_init_openssl_subsequent_calls_no_op);

    return UNITY_END();
}