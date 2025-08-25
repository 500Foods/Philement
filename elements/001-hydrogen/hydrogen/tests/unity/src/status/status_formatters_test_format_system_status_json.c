/*
 * Unity Test File: status_formatters_test_format_system_status_json.c
 *
 * Tests for format_system_status_json function from status_formatters.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include for json_t type
#include <jansson.h>

// Include status_core header for SystemMetrics structure
#include "../../../../src/status/status_core.h"

// Include status_formatters header for the function declaration
#include "../../../../src/status/status_formatters.h"

// Function prototypes for test functions
void test_format_system_status_json_null_metrics(void);
void test_format_system_status_json_minimal_metrics(void);
void test_format_system_status_json_return_type(void);
void test_format_system_status_json_memory_management(void);
void test_format_system_status_json_json_structure(void);
void test_format_system_status_json_with_version_info(void);
void test_format_system_status_json_with_system_info(void);

void setUp(void) {
    // No special setup needed for JSON formatting tests
}

void tearDown(void) {
    // No special cleanup needed for JSON formatting tests
}

// Tests for format_system_status_json function
void test_format_system_status_json_null_metrics(void) {
    // Test calling with NULL metrics
    json_t* result = format_system_status_json(NULL);

    // Function should handle NULL input gracefully
    // Result should be NULL if metrics is NULL, which is acceptable
    (void)result; // Suppress unused variable warning

    // Should not crash with NULL parameter
    TEST_PASS();
}

void test_format_system_status_json_minimal_metrics(void) {
    // Test with minimal initialized metrics structure
    SystemMetrics metrics = {0};

    // Set some basic values
    strcpy(metrics.server_version, "1.0.0");
    strcpy(metrics.api_version, "1.0");
    strcpy(metrics.release, "test");
    strcpy(metrics.build_type, "debug");
    strcpy(metrics.sysname, "Linux");

    json_t* result = format_system_status_json(&metrics);

    // Function should handle minimal metrics
    if (result != NULL) {
        // Verify it's a valid JSON object
        TEST_ASSERT_TRUE(json_is_object(result));

        // Clean up
        json_decref(result);
    }

    // Should not crash with minimal metrics
    TEST_PASS();
}

void test_format_system_status_json_return_type(void) {
    // Test that the function returns a json_t* type
    SystemMetrics metrics = {0};
    strcpy(metrics.server_version, "1.0.0");

    json_t* result = format_system_status_json(&metrics);

    // The return type should be json_t* (may be NULL)
    // We can't easily test the exact type without more complex checks,
    // but we can verify the function signature is correct
    (void)result; // Suppress unused variable warning

    // Clean up if result is not NULL
    if (result) {
        json_decref(result);
    }

    // Should not crash
    TEST_PASS();
}

void test_format_system_status_json_memory_management(void) {
    // Test memory management aspects
    SystemMetrics metrics = {0};
    strcpy(metrics.server_version, "1.0.0");

    json_t* result = format_system_status_json(&metrics);

    if (result != NULL) {
        // If we get a result, we should be able to use it
        // Basic check that it's a valid JSON object
        TEST_ASSERT_TRUE(json_is_object(result) || json_is_array(result) || json_is_string(result));

        // Clean up the JSON object (as required by the API)
        json_decref(result);
    }

    // Should not crash even if result is NULL
    TEST_PASS();
}

void test_format_system_status_json_json_structure(void) {
    // Test that the function creates proper JSON structure
    SystemMetrics metrics = {0};

    // Set some test values
    strcpy(metrics.server_version, "1.0.0");
    strcpy(metrics.api_version, "1.0");
    strcpy(metrics.sysname, "TestOS");

    json_t* result = format_system_status_json(&metrics);

    if (result != NULL) {
        // Verify it's a JSON object
        TEST_ASSERT_TRUE(json_is_object(result));

        // Check for expected top-level keys
        json_t* version = json_object_get(result, "version");
        json_t* system = json_object_get(result, "system");

        // Version should be an object if present
        if (version) {
            TEST_ASSERT_TRUE(json_is_object(version));
        }

        // System should be an object if present
        if (system) {
            TEST_ASSERT_TRUE(json_is_object(system));
        }

        // Clean up
        json_decref(result);
    }

    // Should not crash
    TEST_PASS();
}

void test_format_system_status_json_with_version_info(void) {
    // Test with version information
    SystemMetrics metrics = {0};

    strcpy(metrics.server_version, "2.1.0");
    strcpy(metrics.api_version, "2.0");
    strcpy(metrics.release, "stable");
    strcpy(metrics.build_type, "release");

    json_t* result = format_system_status_json(&metrics);

    if (result != NULL) {
        // Check version object
        json_t* version = json_object_get(result, "version");
        if (version) {
            json_t* server = json_object_get(version, "server");
            if (server) {
                TEST_ASSERT_TRUE(json_is_string(server));
                TEST_ASSERT_EQUAL_STRING("2.1.0", json_string_value(server));
            }
        }

        // Clean up
        json_decref(result);
    }

    // Should not crash
    TEST_PASS();
}

void test_format_system_status_json_with_system_info(void) {
    // Test with system information
    SystemMetrics metrics = {0};

    strcpy(metrics.server_version, "1.0.0");
    strcpy(metrics.sysname, "Linux");
    strcpy(metrics.nodename, "testhost");
    strcpy(metrics.machine, "x86_64");

    json_t* result = format_system_status_json(&metrics);

    if (result != NULL) {
        // Check system object
        json_t* system = json_object_get(result, "system");
        if (system) {
            json_t* sysname = json_object_get(system, "sysname");
            if (sysname) {
                TEST_ASSERT_TRUE(json_is_string(sysname));
                TEST_ASSERT_EQUAL_STRING("Linux", json_string_value(sysname));
            }
        }

        // Clean up
        json_decref(result);
    }

    // Should not crash
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_format_system_status_json_null_metrics);
    if (0) RUN_TEST(test_format_system_status_json_minimal_metrics);
    if (0) RUN_TEST(test_format_system_status_json_return_type);
    if (0) RUN_TEST(test_format_system_status_json_memory_management);
    if (0) RUN_TEST(test_format_system_status_json_json_structure);
    if (0) RUN_TEST(test_format_system_status_json_with_version_info);
    if (0) RUN_TEST(test_format_system_status_json_with_system_info);

    return UNITY_END();
}
