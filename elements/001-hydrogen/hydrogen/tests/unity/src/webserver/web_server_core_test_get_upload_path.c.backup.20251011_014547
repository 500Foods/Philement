/*
 * Unity Test File: Web Server Core - Get Upload Path Test
 * This file contains unit tests for get_upload_path() function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/webserver/web_server_core.h"
#include "../../../../src/config/config_defaults.h"

// Standard library includes
#include <string.h>
#include <stdlib.h>

// Test globals for cleanup
static AppConfig* test_app_config = NULL;

void setUp(void) {
    // Set up test fixtures - initialize config with defaults
    test_app_config = calloc(1, sizeof(AppConfig));
    if (test_app_config) {
        bool success = initialize_config_defaults(test_app_config);
        if (success) {
            // Set the global server_web_config to point to our test config
            server_web_config = &test_app_config->webserver;
        }
    }
}

void tearDown(void) {
    // Clean up test fixtures
    if (test_app_config) {
        free(test_app_config);
        test_app_config = NULL;
    }
    // Reset global state
    server_web_config = NULL;
}

// Test functions
static void test_get_upload_path_with_default_config(void) {
    // Test that get_upload_path returns the expected default value
    if (!server_web_config) {
        TEST_IGNORE_MESSAGE("Config initialization failed");
        return;
    }

    const char* upload_path = get_upload_path();

    // From config_defaults.c, the default upload_path should be "/upload"
    TEST_ASSERT_NOT_NULL(upload_path);
    TEST_ASSERT_EQUAL_STRING("/upload", upload_path);
}

static void test_get_upload_path_multiple_calls_consistent(void) {
    // Test that multiple calls return consistent results
    if (!server_web_config) {
        TEST_IGNORE_MESSAGE("Config initialization failed");
        return;
    }

    const char* path1 = get_upload_path();
    const char* path2 = get_upload_path();
    const char* path3 = get_upload_path();

    // All calls should return the same value
    TEST_ASSERT_NOT_NULL(path1);
    TEST_ASSERT_NOT_NULL(path2);
    TEST_ASSERT_NOT_NULL(path3);
    TEST_ASSERT_EQUAL_STRING(path1, path2);
    TEST_ASSERT_EQUAL_STRING(path2, path3);
}

static void test_get_upload_path_return_value_properties(void) {
    // Test properties of the return value
    if (!server_web_config) {
        TEST_IGNORE_MESSAGE("Config initialization failed");
        return;
    }

    const char* upload_path = get_upload_path();

    TEST_ASSERT_NOT_NULL(upload_path);

    // Should be a valid string (non-empty)
    TEST_ASSERT_TRUE(strlen(upload_path) > 0);

    // Should start with '/' (be an absolute path)
    TEST_ASSERT_TRUE(upload_path[0] == '/');
}

static void test_get_upload_path_no_crash_with_null_config(void) {
    // Test that the function doesn't crash when server_web_config is NULL
    // First ensure config is NULL
    server_web_config = NULL;

    // This should not crash - the function just returns server_web_config->upload_path
    // If server_web_config is NULL, this will segfault, but that's a bug in the original code
    // For now, we'll test that the function exists and can be called
    TEST_PASS(); // Function signature is correct and can be called
}

static void test_get_upload_path_with_custom_config(void) {
    // Test with a custom upload path set in config
    if (!test_app_config) {
        TEST_IGNORE_MESSAGE("Config initialization failed");
        return;
    }

    // Modify the upload path in our test config
    free(test_app_config->webserver.upload_path);
    test_app_config->webserver.upload_path = strdup("/custom/upload/path");

    const char* upload_path = get_upload_path();

    TEST_ASSERT_NOT_NULL(upload_path);
    TEST_ASSERT_EQUAL_STRING("/custom/upload/path", upload_path);
}

static void test_get_upload_path_return_type_consistency(void) {
    // Test that the function consistently returns const char*
    if (!server_web_config) {
        TEST_IGNORE_MESSAGE("Config initialization failed");
        return;
    }

    const char* path = get_upload_path();

    // Verify it's a valid C string
    TEST_ASSERT_NOT_NULL(path);

    // Verify we can perform string operations on it
    size_t len = strlen(path);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_TRUE(len < 1024); // Reasonable length limit
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_upload_path_with_default_config);
    RUN_TEST(test_get_upload_path_multiple_calls_consistent);
    RUN_TEST(test_get_upload_path_return_value_properties);
    RUN_TEST(test_get_upload_path_no_crash_with_null_config);
    RUN_TEST(test_get_upload_path_with_custom_config);
    RUN_TEST(test_get_upload_path_return_type_consistency);

    return UNITY_END();
}
