/*
 * Unity Test File: Swagger Configuration Tests
 * This file contains unit tests for the load_swagger_config function
 * from src/config/config_swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_swagger.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_swagger_config(json_t* root, AppConfig* config);
void cleanup_swagger_config(SwaggerConfig* config);
void dump_swagger_config(const SwaggerConfig* config);

// Forward declarations for test functions
void test_load_swagger_config_null_root(void);
void test_load_swagger_config_empty_json(void);
void test_load_swagger_config_basic_fields(void);
void test_load_swagger_config_metadata_fields(void);
void test_load_swagger_config_ui_options(void);
void test_cleanup_swagger_config_null_pointer(void);
void test_cleanup_swagger_config_empty_config(void);
void test_cleanup_swagger_config_with_data(void);
void test_dump_swagger_config_null_pointer(void);
void test_dump_swagger_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_swagger_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_swagger_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.swagger.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/apidocs", config.swagger.prefix);  // Default value
    TEST_ASSERT_EQUAL_STRING("Hydrogen API", config.swagger.metadata.title);  // Default value

    cleanup_swagger_config(&config.swagger);
}

void test_load_swagger_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_swagger_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.swagger.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/apidocs", config.swagger.prefix);  // Default value
    TEST_ASSERT_EQUAL_STRING("PAYLOAD:/swagger", config.swagger.webroot);  // Default value
    TEST_ASSERT_EQUAL_STRING("*", config.swagger.cors_origin);  // Default value
    TEST_ASSERT_EQUAL_STRING("Hydrogen API", config.swagger.metadata.title);  // Default value

    json_decref(root);
    cleanup_swagger_config(&config.swagger);
}

// ===== BASIC FIELD TESTS =====

void test_load_swagger_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* swagger_section = json_object();

    // Set up basic swagger configuration
    json_object_set(swagger_section, "Enabled", json_false());
    json_object_set(swagger_section, "Prefix", json_string("/docs"));
    json_object_set(swagger_section, "WebRoot", json_string("/var/www/swagger"));
    json_object_set(swagger_section, "CORSOrigin", json_string("https://example.com"));

    json_object_set(root, "Swagger", swagger_section);

    bool result = load_swagger_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.swagger.enabled);
    TEST_ASSERT_EQUAL_STRING("/docs", config.swagger.prefix);
    TEST_ASSERT_EQUAL_STRING("/var/www/swagger", config.swagger.webroot);
    TEST_ASSERT_EQUAL_STRING("https://example.com", config.swagger.cors_origin);

    json_decref(root);
    cleanup_swagger_config(&config.swagger);
}

// ===== METADATA FIELD TESTS =====

void test_load_swagger_config_metadata_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* swagger_section = json_object();
    json_t* metadata_section = json_object();
    json_t* contact_section = json_object();
    json_t* license_section = json_object();

    // Set up metadata configuration
    json_object_set(metadata_section, "Title", json_string("My Custom API"));
    json_object_set(metadata_section, "Description", json_string("Custom API Description"));
    json_object_set(metadata_section, "Version", json_string("2.0.0"));

    json_object_set(contact_section, "Name", json_string("John Doe"));
    json_object_set(contact_section, "Email", json_string("john@example.com"));
    json_object_set(contact_section, "URL", json_string("https://example.com"));

    json_object_set(license_section, "Name", json_string("MIT"));
    json_object_set(license_section, "URL", json_string("https://opensource.org/licenses/MIT"));

    json_object_set(metadata_section, "Contact", contact_section);
    json_object_set(metadata_section, "License", license_section);
    json_object_set(swagger_section, "Metadata", metadata_section);
    json_object_set(root, "Swagger", swagger_section);

    bool result = load_swagger_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("My Custom API", config.swagger.metadata.title);
    TEST_ASSERT_EQUAL_STRING("Custom API Description", config.swagger.metadata.description);
    TEST_ASSERT_EQUAL_STRING("2.0.0", config.swagger.metadata.version);
    TEST_ASSERT_EQUAL_STRING("John Doe", config.swagger.metadata.contact.name);
    TEST_ASSERT_EQUAL_STRING("john@example.com", config.swagger.metadata.contact.email);
    TEST_ASSERT_EQUAL_STRING("MIT", config.swagger.metadata.license.name);

    json_decref(root);
    cleanup_swagger_config(&config.swagger);
}

// ===== UI OPTIONS TESTS =====

void test_load_swagger_config_ui_options(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* swagger_section = json_object();
    json_t* ui_section = json_object();

    // Set up UI options configuration
    json_object_set(ui_section, "TryItEnabled", json_false());
    json_object_set(ui_section, "AlwaysExpanded", json_true());
    json_object_set(ui_section, "DisplayOperationId", json_true());
    json_object_set(ui_section, "DefaultModelsExpandDepth", json_integer(2));
    json_object_set(ui_section, "DefaultModelExpandDepth", json_integer(3));
    json_object_set(ui_section, "ShowExtensions", json_true());
    json_object_set(ui_section, "ShowCommonExtensions", json_false());
    json_object_set(ui_section, "DocExpansion", json_string("full"));
    json_object_set(ui_section, "SyntaxHighlightTheme", json_string("arta"));

    json_object_set(swagger_section, "UIOptions", ui_section);
    json_object_set(root, "Swagger", swagger_section);

    bool result = load_swagger_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.swagger.ui_options.try_it_enabled);
    TEST_ASSERT_TRUE(config.swagger.ui_options.always_expanded);
    TEST_ASSERT_TRUE(config.swagger.ui_options.display_operation_id);
    TEST_ASSERT_EQUAL(2, config.swagger.ui_options.default_models_expand_depth);
    TEST_ASSERT_EQUAL(3, config.swagger.ui_options.default_model_expand_depth);
    TEST_ASSERT_TRUE(config.swagger.ui_options.show_extensions);
    TEST_ASSERT_FALSE(config.swagger.ui_options.show_common_extensions);
    TEST_ASSERT_EQUAL_STRING("full", config.swagger.ui_options.doc_expansion);
    TEST_ASSERT_EQUAL_STRING("arta", config.swagger.ui_options.syntax_highlight_theme);

    json_decref(root);
    cleanup_swagger_config(&config.swagger);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_swagger_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_swagger_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_swagger_config_empty_config(void) {
    SwaggerConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_swagger_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.prefix);
    TEST_ASSERT_NULL(config.webroot);
    TEST_ASSERT_NULL(config.cors_origin);
    TEST_ASSERT_NULL(config.metadata.title);
}

void test_cleanup_swagger_config_with_data(void) {
    SwaggerConfig config = {0};

    // Initialize with some test data
    config.enabled = true;
    config.prefix = strdup("/docs");
    config.webroot = strdup("/var/www");
    config.cors_origin = strdup("*");
    config.metadata.title = strdup("Test API");
    config.metadata.description = strdup("Test Description");
    config.metadata.contact.name = strdup("Test Contact");
    config.ui_options.doc_expansion = strdup("list");
    config.ui_options.syntax_highlight_theme = strdup("agate");

    // Cleanup should free all allocated memory
    cleanup_swagger_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.prefix);
    TEST_ASSERT_NULL(config.webroot);
    TEST_ASSERT_NULL(config.cors_origin);
    TEST_ASSERT_NULL(config.metadata.title);
    TEST_ASSERT_NULL(config.metadata.description);
    TEST_ASSERT_NULL(config.metadata.contact.name);
    TEST_ASSERT_NULL(config.ui_options.doc_expansion);
    TEST_ASSERT_NULL(config.ui_options.syntax_highlight_theme);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_swagger_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_swagger_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_swagger_config_basic(void) {
    SwaggerConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.prefix = strdup("/docs");
    config.webroot = strdup("/var/www");
    config.cors_origin = strdup("*");
    config.metadata.title = strdup("Test API");
    config.metadata.description = strdup("Test Description");
    config.ui_options.try_it_enabled = true;
    config.ui_options.doc_expansion = strdup("list");

    // Dump should not crash and handle the data properly
    dump_swagger_config(&config);

    // Clean up
    cleanup_swagger_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_swagger_config_null_root);
    RUN_TEST(test_load_swagger_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_swagger_config_basic_fields);
    RUN_TEST(test_load_swagger_config_metadata_fields);
    RUN_TEST(test_load_swagger_config_ui_options);

    // Cleanup function tests
    RUN_TEST(test_cleanup_swagger_config_null_pointer);
    RUN_TEST(test_cleanup_swagger_config_empty_config);
    RUN_TEST(test_cleanup_swagger_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_swagger_config_null_pointer);
    RUN_TEST(test_dump_swagger_config_basic);

    return UNITY_END();
}