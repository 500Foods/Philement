/*
 * Unity Test File: cleanup_swagger_support Function Tests
 * This file contains unit tests for the cleanup_swagger_support() function
 * from src/swagger/swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/swagger/swagger.h"

// Forward declaration for the function being tested
void cleanup_swagger_support(void);

// Function prototypes for test functions
void test_cleanup_swagger_support_basic(void);
void test_cleanup_swagger_support_multiple_calls(void);
void test_cleanup_swagger_support_without_init(void);

void setUp(void) {
    // No setup needed for cleanup tests
}

void tearDown(void) {
    // No teardown needed for cleanup tests
}

void test_cleanup_swagger_support_basic(void) {
    // Should not crash when called
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_cleanup_swagger_support_multiple_calls(void) {
    // Should handle multiple calls gracefully
    cleanup_swagger_support();
    cleanup_swagger_support();
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_cleanup_swagger_support_without_init(void) {
    // Should be safe to call cleanup without prior initialization
    cleanup_swagger_support();
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cleanup_swagger_support_basic);
    RUN_TEST(test_cleanup_swagger_support_multiple_calls);
    RUN_TEST(test_cleanup_swagger_support_without_init);
    
    return UNITY_END();
}
