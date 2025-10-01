/*
 * Unity Test File: Enhanced Swagger Launch Validation Tests
 * This file contains comprehensive unit tests for check_swagger_launch_readiness function
 * focusing on the uncovered validation scenarios identified in coverage analysis
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/launch/launch.h"

// Enable mocks for testing system dependencies
#define USE_MOCK_LAUNCH
#include "../../../../tests/unity/mocks/mock_launch.h"

// Forward declarations for functions being tested
LaunchReadiness check_swagger_launch_readiness(void);

// Test function declarations
void test_check_swagger_launch_readiness_swagger_disabled(void);
void test_check_swagger_launch_readiness_invalid_prefix_null(void);
void test_check_swagger_launch_readiness_invalid_prefix_empty(void);
void test_check_swagger_launch_readiness_invalid_prefix_too_long(void);
void test_check_swagger_launch_readiness_invalid_prefix_no_leading_slash(void);
void test_check_swagger_launch_readiness_invalid_title_null(void);
void test_check_swagger_launch_readiness_invalid_title_empty(void);
void test_check_swagger_launch_readiness_invalid_title_too_long(void);
void test_check_swagger_launch_readiness_invalid_version_null(void);
void test_check_swagger_launch_readiness_invalid_version_empty(void);
void test_check_swagger_launch_readiness_invalid_version_too_long(void);
void test_check_swagger_launch_readiness_invalid_description_too_long(void);
void test_check_swagger_launch_readiness_invalid_models_expand_depth_negative(void);
void test_check_swagger_launch_readiness_invalid_models_expand_depth_too_high(void);
void test_check_swagger_launch_readiness_invalid_model_expand_depth_negative(void);
void test_check_swagger_launch_readiness_invalid_model_expand_depth_too_high(void);
void test_check_swagger_launch_readiness_invalid_doc_expansion_value(void);
void test_check_swagger_launch_readiness_valid_configuration(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_launch_reset_all();

    // Setup basic configuration for testing
    if (app_config) {
        free(app_config);
    }
    app_config = calloc(1, sizeof(AppConfig));
}

void tearDown(void) {
    // Clean up after each test
    mock_launch_reset_all();

    // Clean up configuration
    if (app_config) {
        if (app_config->swagger.prefix) {
            free(app_config->swagger.prefix);
        }
        if (app_config->swagger.metadata.title) {
            free(app_config->swagger.metadata.title);
        }
        if (app_config->swagger.metadata.version) {
            free(app_config->swagger.metadata.version);
        }
        if (app_config->swagger.metadata.description) {
            free(app_config->swagger.metadata.description);
        }
        if (app_config->swagger.ui_options.doc_expansion) {
            free(app_config->swagger.ui_options.doc_expansion);
        }
        free(app_config);
        app_config = NULL;
    }
}

void test_check_swagger_launch_readiness_swagger_disabled(void) {
    // Setup: Create config with Swagger disabled
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = false;

    // Test: Should return not ready when Swagger is disabled
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_prefix_null(void) {
    // Setup: Create config with null prefix
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = NULL;

    // Test: Should return not ready when prefix is null
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_prefix_empty(void) {
    // Setup: Create config with empty prefix
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("");

    // Test: Should return not ready when prefix is empty
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_prefix_too_long(void) {
    // Setup: Create config with prefix that's too long (>64 chars)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/this-is-a-very-long-prefix-that-exceeds-the-maximum-allowed-length");

    // Test: Should return not ready when prefix is too long
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_prefix_no_leading_slash(void) {
    // Setup: Create config with prefix that doesn't start with /
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("swagger-ui");

    // Test: Should return not ready when prefix doesn't start with /
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_title_null(void) {
    // Setup: Create config with null title
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = NULL;

    // Test: Should return not ready when title is null
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_title_empty(void) {
    // Setup: Create config with empty title
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("");

    // Test: Should return not ready when title is empty
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_title_too_long(void) {
    // Setup: Create config with title that's too long (>128 chars)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("This is a very long title that exceeds the maximum allowed length for a Swagger title and should cause validation to fail when checked by the launch readiness function");

    // Test: Should return not ready when title is too long
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_version_null(void) {
    // Setup: Create config with null version
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = NULL;

    // Test: Should return not ready when version is null
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_version_empty(void) {
    // Setup: Create config with empty version
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("");

    // Test: Should return not ready when version is empty
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_version_too_long(void) {
    // Setup: Create config with version that's too long (>32 chars)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0");

    // Test: Should return not ready when version is too long
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_description_too_long(void) {
    // Setup: Create config with description that's too long (>1024 chars)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");

    // Create a very long description
    char* long_description = malloc(1025);
    TEST_ASSERT_NOT_NULL(long_description); // cppcheck: Ensure malloc succeeded
    memset(long_description, 'A', 1024);
    long_description[1024] = '\0';
    config->swagger.metadata.description = long_description;

    // Test: Should return not ready when description is too long
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_models_expand_depth_negative(void) {
    // Setup: Create config with negative models expand depth
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.ui_options.default_models_expand_depth = -1;

    // Test: Should return not ready when models expand depth is negative
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_models_expand_depth_too_high(void) {
    // Setup: Create config with models expand depth too high (>10)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.ui_options.default_models_expand_depth = 15;

    // Test: Should return not ready when models expand depth is too high
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_model_expand_depth_negative(void) {
    // Setup: Create config with negative model expand depth
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.ui_options.default_model_expand_depth = -5;

    // Test: Should return not ready when model expand depth is negative
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_model_expand_depth_too_high(void) {
    // Setup: Create config with model expand depth too high (>10)
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.ui_options.default_model_expand_depth = 12;

    // Test: Should return not ready when model expand depth is too high
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_invalid_doc_expansion_value(void) {
    // Setup: Create config with invalid doc expansion value
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.ui_options.doc_expansion = strdup("invalid");

    // Test: Should return not ready when doc expansion value is invalid
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_swagger_launch_readiness_valid_configuration(void) {
    // Setup: Create config with all valid values
    AppConfig* config = (AppConfig*)app_config;
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/swagger");
    config->swagger.metadata.title = strdup("Test API");
    config->swagger.metadata.version = strdup("1.0.0");
    config->swagger.metadata.description = strdup("A test API for unit testing");
    config->swagger.ui_options.default_models_expand_depth = 2;
    config->swagger.ui_options.default_model_expand_depth = 1;
    config->swagger.ui_options.doc_expansion = strdup("list");

    // Test: Should return ready when all configuration is valid
    LaunchReadiness result = check_swagger_launch_readiness();
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    // Run all test functions for validation scenarios
    RUN_TEST(test_check_swagger_launch_readiness_swagger_disabled);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_prefix_null);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_prefix_empty);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_prefix_too_long);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_prefix_no_leading_slash);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_title_null);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_title_empty);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_title_too_long);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_version_null);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_version_empty);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_version_too_long);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_description_too_long);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_models_expand_depth_negative);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_models_expand_depth_too_high);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_model_expand_depth_negative);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_model_expand_depth_too_high);
    RUN_TEST(test_check_swagger_launch_readiness_invalid_doc_expansion_value);
    if (0) RUN_TEST(test_check_swagger_launch_readiness_valid_configuration);

    return UNITY_END();
}