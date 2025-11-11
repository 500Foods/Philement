/*
 * Unity Test File: Comprehensive Swagger Launch Tests
 * Tests for launch_swagger validation and error paths
 */

// Enable mocks BEFORE including ANY headers
// USE_MOCK_LAUNCH defined by CMake

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>
#include <src/config/config_swagger.h>

// Forward declarations for functions being tested
bool validate_swagger_configuration(const char*** messages, size_t* count, size_t* capacity);

// Forward declarations for test functions
void test_validate_swagger_invalid_prefix_null(void);
void test_validate_swagger_invalid_prefix_empty(void);
void test_validate_swagger_invalid_prefix_too_long(void);
void test_validate_swagger_invalid_prefix_no_slash(void);
void test_validate_swagger_invalid_title_null(void);
void test_validate_swagger_invalid_title_empty(void);
void test_validate_swagger_invalid_title_too_long(void);
void test_validate_swagger_invalid_version_null(void);
void test_validate_swagger_invalid_version_empty(void);
void test_validate_swagger_invalid_version_too_long(void);
void test_validate_swagger_invalid_description_too_long(void);
void test_validate_swagger_invalid_models_expand_depth_negative(void);
void test_validate_swagger_invalid_models_expand_depth_too_high(void);
void test_validate_swagger_invalid_model_expand_depth_negative(void);
void test_validate_swagger_invalid_model_expand_depth_too_high(void);
void test_validate_swagger_invalid_doc_expansion(void);
void test_validate_swagger_valid_configuration(void);

// External references
extern AppConfig* app_config;

// Test helper to create minimal valid configuration using config_defaults
static void setup_valid_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;
        }
    }
    
    // Use the standard initialization which sets all defaults properly
    initialize_config_defaults(app_config);
    
    // Enable Swagger subsystem
    app_config->swagger.enabled = true;
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        cleanup_swagger_config(&app_config->swagger);
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_launch_reset_all();
}

// Test 1: Invalid prefix - NULL
void test_validate_swagger_invalid_prefix_null(void) {
    setup_valid_config();
    free(app_config->swagger.prefix);
    app_config->swagger.prefix = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 2: Invalid prefix - empty string
void test_validate_swagger_invalid_prefix_empty(void) {
    setup_valid_config();
    free(app_config->swagger.prefix);
    app_config->swagger.prefix = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 3: Invalid prefix - too long (>64 chars)
void test_validate_swagger_invalid_prefix_too_long(void) {
    setup_valid_config();
    free(app_config->swagger.prefix);
    app_config->swagger.prefix = strdup("/this-is-a-very-long-prefix-that-exceeds-the-maximum-allowed-length-of-64-characters");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 4: Invalid prefix - no leading slash
void test_validate_swagger_invalid_prefix_no_slash(void) {
    setup_valid_config();
    free(app_config->swagger.prefix);
    app_config->swagger.prefix = strdup("apidocs");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 5: Invalid title - NULL
void test_validate_swagger_invalid_title_null(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.title);
    app_config->swagger.metadata.title = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 6: Invalid title - empty string
void test_validate_swagger_invalid_title_empty(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.title);
    app_config->swagger.metadata.title = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 7: Invalid title - too long (>128 chars)
void test_validate_swagger_invalid_title_too_long(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.title);
    app_config->swagger.metadata.title = strdup("This is a very long title that exceeds the maximum allowed length for a Swagger API title and should cause validation to fail when checking");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 8: Invalid version - NULL
void test_validate_swagger_invalid_version_null(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.version);
    app_config->swagger.metadata.version = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 9: Invalid version - empty string
void test_validate_swagger_invalid_version_empty(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.version);
    app_config->swagger.metadata.version = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 10: Invalid version - too long (>32 chars)
void test_validate_swagger_invalid_version_too_long(void) {
    setup_valid_config();
    free(app_config->swagger.metadata.version);
    app_config->swagger.metadata.version = strdup("1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 11: Invalid description - too long (>1024 chars)
void test_validate_swagger_invalid_description_too_long(void) {
    setup_valid_config();
    
    // Create a very long description - needs to be MORE than 1024 chars
    char* long_description = malloc(1050);
    TEST_ASSERT_NOT_NULL(long_description);
    memset(long_description, 'A', 1049);
    long_description[1049] = '\0';
    app_config->swagger.metadata.description = long_description;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 12: Invalid models expand depth - negative
void test_validate_swagger_invalid_models_expand_depth_negative(void) {
    setup_valid_config();
    app_config->swagger.ui_options.default_models_expand_depth = -1;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 13: Invalid models expand depth - too high (>10)
void test_validate_swagger_invalid_models_expand_depth_too_high(void) {
    setup_valid_config();
    app_config->swagger.ui_options.default_models_expand_depth = 15;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 14: Invalid model expand depth - negative
void test_validate_swagger_invalid_model_expand_depth_negative(void) {
    setup_valid_config();
    app_config->swagger.ui_options.default_model_expand_depth = -5;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 15: Invalid model expand depth - too high (>10)
void test_validate_swagger_invalid_model_expand_depth_too_high(void) {
    setup_valid_config();
    app_config->swagger.ui_options.default_model_expand_depth = 12;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 16: Invalid doc expansion value
void test_validate_swagger_invalid_doc_expansion(void) {
    setup_valid_config();
    free(app_config->swagger.ui_options.doc_expansion);
    app_config->swagger.ui_options.doc_expansion = strdup("invalid");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test 17: Valid configuration
void test_validate_swagger_valid_configuration(void) {
    setup_valid_config();
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_swagger_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

int main(void) {
    UNITY_BEGIN();
    
    // Prefix validation tests
    RUN_TEST(test_validate_swagger_invalid_prefix_null);
    RUN_TEST(test_validate_swagger_invalid_prefix_empty);
    RUN_TEST(test_validate_swagger_invalid_prefix_too_long);
    RUN_TEST(test_validate_swagger_invalid_prefix_no_slash);
    
    // Title validation tests
    RUN_TEST(test_validate_swagger_invalid_title_null);
    RUN_TEST(test_validate_swagger_invalid_title_empty);
    RUN_TEST(test_validate_swagger_invalid_title_too_long);
    
    // Version validation tests
    RUN_TEST(test_validate_swagger_invalid_version_null);
    RUN_TEST(test_validate_swagger_invalid_version_empty);
    RUN_TEST(test_validate_swagger_invalid_version_too_long);
    
    // Description validation test
    RUN_TEST(test_validate_swagger_invalid_description_too_long);
    
    // UI options validation tests
    RUN_TEST(test_validate_swagger_invalid_models_expand_depth_negative);
    RUN_TEST(test_validate_swagger_invalid_models_expand_depth_too_high);
    RUN_TEST(test_validate_swagger_invalid_model_expand_depth_negative);
    RUN_TEST(test_validate_swagger_invalid_model_expand_depth_too_high);
    RUN_TEST(test_validate_swagger_invalid_doc_expansion);
    
    // Valid configuration test
    RUN_TEST(test_validate_swagger_valid_configuration);
    
    return UNITY_END();
}