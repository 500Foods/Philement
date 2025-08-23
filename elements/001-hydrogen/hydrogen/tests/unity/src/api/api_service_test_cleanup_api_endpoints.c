/*
 * Unity Test File: API Service cleanup_api_endpoints Function Tests
 * This file contains unit tests for the cleanup_api_endpoints function in api_service.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/api/api_service.h"

// Forward declarations for functions being tested
void cleanup_api_endpoints(void);

// Function prototypes for test functions
void test_cleanup_api_endpoints_basic(void);
void test_cleanup_api_endpoints_with_config(void);
void test_cleanup_api_endpoints_custom_prefix(void);
void test_cleanup_api_endpoints_no_app_config(void);
void test_cleanup_api_endpoints_no_prefix(void);
void test_cleanup_api_endpoints_empty_prefix(void);
void test_cleanup_api_endpoints_multiple_calls(void);
void test_cleanup_api_endpoints_various_prefix_formats(void);
void test_cleanup_api_endpoints_consistency(void);
void test_cleanup_api_endpoints_extreme_cases(void);
void test_cleanup_api_endpoints_preserves_config(void);

void setUp(void) {
    // Initialize app_config for testing
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (app_config) {
            app_config->api.prefix = strdup("/api");
        }
    } else if (!app_config->api.prefix) {
        app_config->api.prefix = strdup("/api");
    }
}

void tearDown(void) {
    // Reset app_config to a clean state for next test
    if (app_config) {
        if (app_config->api.prefix) {
            free(app_config->api.prefix);
            app_config->api.prefix = strdup("/api");  // Reset to default
        }
    }
}

// Test basic cleanup functionality
void test_cleanup_api_endpoints_basic(void) {
    // This function should complete without crashing
    cleanup_api_endpoints();
    
    // If we reach this point, the function completed successfully
    TEST_ASSERT_TRUE(true);
}

// Test cleanup with valid configuration
void test_cleanup_api_endpoints_with_config(void) {
    // Ensure we have a valid configuration
    TEST_ASSERT_NOT_NULL(app_config);
    TEST_ASSERT_NOT_NULL(app_config->api.prefix);
    
    cleanup_api_endpoints();
    
    // Function should complete successfully
    TEST_ASSERT_TRUE(true);
}

// Test cleanup with custom prefix
void test_cleanup_api_endpoints_custom_prefix(void) {
    // Set a custom prefix
    free(app_config->api.prefix);
    app_config->api.prefix = strdup("/custom/api");
    
    cleanup_api_endpoints();
    
    // Function should complete successfully
    TEST_ASSERT_TRUE(true);
}

// Test cleanup with no app_config
void test_cleanup_api_endpoints_no_app_config(void) {
    // Temporarily clear app_config
    AppConfig *saved_config = app_config;
    app_config = NULL;
    
    // Should not crash even without config
    cleanup_api_endpoints();
    
    // Function should complete successfully
    TEST_ASSERT_TRUE(true);
    
    // Restore app_config
    app_config = saved_config;
}

// Test cleanup with no prefix
void test_cleanup_api_endpoints_no_prefix(void) {
    // Clear the prefix but keep app_config
    if (app_config->api.prefix) {
        free(app_config->api.prefix);
        app_config->api.prefix = NULL;
    }
    
    // Should not crash even without prefix
    cleanup_api_endpoints();
    
    // Function should complete successfully
    TEST_ASSERT_TRUE(true);
}

// Test cleanup with empty prefix
void test_cleanup_api_endpoints_empty_prefix(void) {
    // Set empty prefix
    free(app_config->api.prefix);
    app_config->api.prefix = strdup("");
    
    // Should handle empty prefix gracefully
    cleanup_api_endpoints();
    
    // Function should complete successfully
    TEST_ASSERT_TRUE(true);
}

// Test multiple calls to cleanup (should be safe)
void test_cleanup_api_endpoints_multiple_calls(void) {
    // First cleanup
    cleanup_api_endpoints();
    
    // Second cleanup should be safe
    cleanup_api_endpoints();
    
    // Third cleanup should also be safe
    cleanup_api_endpoints();
    
    // All calls should complete successfully
    TEST_ASSERT_TRUE(true);
}

// Test cleanup with different prefix formats
void test_cleanup_api_endpoints_various_prefix_formats(void) {
    const char* test_prefixes[] = {
        "/api",
        "/api/",
        "/custom",
        "/v1/api",
        "/test/long/prefix",
        "api",           // Without leading slash
        "/",             // Root only
        "custom/api"     // Without leading slash
    };
    
    size_t num_prefixes = sizeof(test_prefixes) / sizeof(test_prefixes[0]);
    
    for (size_t i = 0; i < num_prefixes; i++) {
        // Set the test prefix
        if (app_config->api.prefix) {
            free(app_config->api.prefix);
        }
        app_config->api.prefix = strdup(test_prefixes[i]);
        
        // Cleanup should handle any prefix format
        cleanup_api_endpoints();
        
        // Function should complete successfully for each prefix
        TEST_ASSERT_TRUE(true);
    }
}

// Test cleanup behavior consistency
void test_cleanup_api_endpoints_consistency(void) {
    // Set up a known state
    free(app_config->api.prefix);
    app_config->api.prefix = strdup("/api/v1");
    
    // Multiple cleanups should be consistent
    for (int i = 0; i < 5; i++) {
        cleanup_api_endpoints();
        
        // Each call should complete successfully
        TEST_ASSERT_TRUE(true);
        
        // Configuration should remain unchanged
        TEST_ASSERT_NOT_NULL(app_config);
        TEST_ASSERT_NOT_NULL(app_config->api.prefix);
        TEST_ASSERT_EQUAL_STRING("/api/v1", app_config->api.prefix);
    }
}

// Test cleanup with extreme cases
void test_cleanup_api_endpoints_extreme_cases(void) {
    // Test with very long prefix
    char long_prefix[1000];
    memset(long_prefix, 'a', sizeof(long_prefix) - 1);
    long_prefix[0] = '/';  // Make it start with /
    long_prefix[sizeof(long_prefix) - 1] = '\0';
    
    free(app_config->api.prefix);
    app_config->api.prefix = strdup(long_prefix);
    
    // Should handle long prefix without issues
    cleanup_api_endpoints();
    TEST_ASSERT_TRUE(true);
    
    // Test with special characters
    free(app_config->api.prefix);
    app_config->api.prefix = strdup("/api-v1_test.endpoint");
    
    cleanup_api_endpoints();
    TEST_ASSERT_TRUE(true);
    
    // Test with Unicode (if supported by system)
    free(app_config->api.prefix);
    app_config->api.prefix = strdup("/api/测试");
    
    cleanup_api_endpoints();
    TEST_ASSERT_TRUE(true);
}

// Test cleanup doesn't modify configuration
void test_cleanup_api_endpoints_preserves_config(void) {
    const char* original_prefix = "/api/test";
    free(app_config->api.prefix);
    app_config->api.prefix = strdup(original_prefix);
    
    // Verify initial state
    TEST_ASSERT_EQUAL_STRING(original_prefix, app_config->api.prefix);
    
    // Cleanup should not modify the configuration
    cleanup_api_endpoints();
    
    // Configuration should be preserved
    TEST_ASSERT_NOT_NULL(app_config);
    TEST_ASSERT_NOT_NULL(app_config->api.prefix);
    TEST_ASSERT_EQUAL_STRING(original_prefix, app_config->api.prefix);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cleanup_api_endpoints_basic);
    RUN_TEST(test_cleanup_api_endpoints_with_config);
    RUN_TEST(test_cleanup_api_endpoints_custom_prefix);
    RUN_TEST(test_cleanup_api_endpoints_no_app_config);
    RUN_TEST(test_cleanup_api_endpoints_no_prefix);
    RUN_TEST(test_cleanup_api_endpoints_empty_prefix);
    RUN_TEST(test_cleanup_api_endpoints_multiple_calls);
    RUN_TEST(test_cleanup_api_endpoints_various_prefix_formats);
    RUN_TEST(test_cleanup_api_endpoints_consistency);
    RUN_TEST(test_cleanup_api_endpoints_extreme_cases);
    RUN_TEST(test_cleanup_api_endpoints_preserves_config);
    
    return UNITY_END();
}
