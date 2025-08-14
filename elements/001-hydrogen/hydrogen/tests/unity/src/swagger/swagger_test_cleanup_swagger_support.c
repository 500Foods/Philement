/*
 * Unity Test File: cleanup_swagger_support Function Tests
 * This file contains unit tests for the cleanup_swagger_support() function
 * from src/swagger/swagger.c
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

// Include necessary headers
#include "../../../../src/swagger/swagger.h"
#include "../../../../src/config/config.h"
#include "../../../../src/config/config_swagger.h"

// Forward declaration for the function being tested
void cleanup_swagger_support(void);

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

void test_cleanup_swagger_support_after_init(void) {
    // Test cleanup after initialization
    SwaggerConfig test_config = {0};
    test_config.enabled = true;
    test_config.payload_available = true;
    test_config.prefix = strdup("/swagger");
    
    init_swagger_support(&test_config);
    cleanup_swagger_support();
    
    // Should be able to call again
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true);
    
    free(test_config.prefix);
}

void test_cleanup_swagger_support_repeated_sequence(void) {
    // Test repeated init/cleanup sequences
    for (int i = 0; i < 3; i++) {
        SwaggerConfig test_config = {0};
        test_config.enabled = true;
        test_config.payload_available = true;
        test_config.prefix = strdup("/swagger");
        
        init_swagger_support(&test_config);
        cleanup_swagger_support();
        
        free(test_config.prefix);
    }
    TEST_ASSERT_TRUE(true);
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
    RUN_TEST(test_cleanup_swagger_support_after_init);
    RUN_TEST(test_cleanup_swagger_support_repeated_sequence);
    RUN_TEST(test_cleanup_swagger_support_without_init);
    
    return UNITY_END();
}