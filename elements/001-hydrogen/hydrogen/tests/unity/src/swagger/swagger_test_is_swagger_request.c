/*
 * Unity Test File: Swagger is_swagger_request Function Tests
 * This file contains comprehensive unit tests for the is_swagger_request() function
 * from src/swagger/swagger.c
 * 
 * Coverage Goals:
 * - Test URL matching patterns and prefix handling
 * - Parameter validation and null checks
 * - Edge cases and boundary conditions
 * - Configuration state validation
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the swagger module
#include "../../../../src/swagger/swagger.h"

// Forward declaration for the function being tested
bool is_swagger_request(const char *url, const SwaggerConfig *config);

// Function prototypes for test functions
void test_is_swagger_request_null_url(void);
void test_is_swagger_request_null_config(void);
void test_is_swagger_request_both_null(void);
void test_is_swagger_request_disabled_config(void);
void test_is_swagger_request_payload_not_available(void);
void test_is_swagger_request_null_prefix(void);
void test_is_swagger_request_empty_url(void);
void test_is_swagger_request_exact_match(void);
void test_is_swagger_request_with_trailing_slash(void);
void test_is_swagger_request_with_path(void);
void test_is_swagger_request_with_nested_path(void);
void test_is_swagger_request_wrong_prefix(void);
void test_is_swagger_request_partial_match(void);
void test_is_swagger_request_prefix_as_substring(void);
void test_is_swagger_request_root_url(void);
void test_is_swagger_request_custom_prefix(void);
void test_is_swagger_request_long_prefix(void);
void test_is_swagger_request_boundary_conditions(void);
void test_is_swagger_request_complex_paths(void);
void test_is_swagger_request_special_characters(void);
void test_is_swagger_request_numeric_prefixes(void);
void test_is_swagger_request_case_sensitivity(void);
void test_is_swagger_request_query_parameters(void);
void test_is_swagger_request_fragments(void);
void test_config_validation_all_disabled(void);
void test_config_validation_enabled_but_no_payload(void);
void test_config_validation_payload_but_disabled(void);
void test_config_validation_empty_config(void);
void test_config_validation_minimal_config(void);
void test_parameter_validation_edge_cases(void);
void test_url_matching_comprehensive_edge_cases(void);
void test_url_matching_prefix_variations(void);
void test_stress_many_url_tests(void);

// Test fixtures
static SwaggerConfig test_config;
static SwaggerConfig empty_config;
static SwaggerConfig minimal_config;

void setUp(void) {
    // Initialize test config with safe defaults
    memset(&test_config, 0, sizeof(SwaggerConfig));
    test_config.enabled = true;
    test_config.payload_available = true;
    test_config.prefix = strdup("/swagger");
    
    // Initialize metadata
    test_config.metadata.title = strdup("Test API");
    test_config.metadata.description = strdup("Test Description");
    test_config.metadata.version = strdup("1.0.0");
    
    // Initialize contact info
    test_config.metadata.contact.name = strdup("Test Contact");
    test_config.metadata.contact.email = strdup("test@example.com");
    test_config.metadata.contact.url = strdup("https://example.com");
    
    // Initialize license info
    test_config.metadata.license.name = strdup("MIT");
    test_config.metadata.license.url = strdup("https://opensource.org/licenses/MIT");
    
    // Initialize UI options
    test_config.ui_options.try_it_enabled = true;
    test_config.ui_options.display_operation_id = false;
    test_config.ui_options.default_models_expand_depth = 1;
    test_config.ui_options.default_model_expand_depth = 1;
    test_config.ui_options.show_extensions = true;
    test_config.ui_options.show_common_extensions = true;
    test_config.ui_options.doc_expansion = strdup("list");
    test_config.ui_options.syntax_highlight_theme = strdup("agate");
    
    // Initialize empty config (all zeros)
    memset(&empty_config, 0, sizeof(SwaggerConfig));
    
    // Initialize minimal config (minimal valid configuration)
    memset(&minimal_config, 0, sizeof(SwaggerConfig));
    minimal_config.enabled = true;
    minimal_config.payload_available = true;
    minimal_config.prefix = strdup("/api-docs");
}

void tearDown(void) {
    // Clean up test config
    free(test_config.prefix);
    free(test_config.metadata.title);
    free(test_config.metadata.description);
    free(test_config.metadata.version);
    free(test_config.metadata.contact.name);
    free(test_config.metadata.contact.email);
    free(test_config.metadata.contact.url);
    free(test_config.metadata.license.name);
    free(test_config.metadata.license.url);
    free(test_config.ui_options.doc_expansion);
    free(test_config.ui_options.syntax_highlight_theme);
    
    // Clean up minimal config
    free(minimal_config.prefix);
    
    // Reset all configs
    memset(&test_config, 0, sizeof(SwaggerConfig));
    memset(&empty_config, 0, sizeof(SwaggerConfig));
    memset(&minimal_config, 0, sizeof(SwaggerConfig));
}

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_is_swagger_request_null_url(void) {
    TEST_ASSERT_FALSE(is_swagger_request(NULL, &test_config));
}

void test_is_swagger_request_null_config(void) {
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", NULL));
}

void test_is_swagger_request_both_null(void) {
    TEST_ASSERT_FALSE(is_swagger_request(NULL, NULL));
}

void test_is_swagger_request_disabled_config(void) {
    test_config.enabled = false;
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_payload_not_available(void) {
    test_config.payload_available = false;
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_null_prefix(void) {
    free(test_config.prefix);
    test_config.prefix = NULL;
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_empty_url(void) {
    TEST_ASSERT_FALSE(is_swagger_request("", &test_config));
}

//=============================================================================
// Basic URL Matching Tests
//=============================================================================

void test_is_swagger_request_exact_match(void) {
    TEST_ASSERT_TRUE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_with_trailing_slash(void) {
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/", &test_config));
}

void test_is_swagger_request_with_path(void) {
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/index.html", &test_config));
}

void test_is_swagger_request_with_nested_path(void) {
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/css/style.css", &test_config));
}

void test_is_swagger_request_wrong_prefix(void) {
    TEST_ASSERT_FALSE(is_swagger_request("/api", &test_config));
}

void test_is_swagger_request_partial_match(void) {
    TEST_ASSERT_FALSE(is_swagger_request("/swag", &test_config));
}

void test_is_swagger_request_prefix_as_substring(void) {
    TEST_ASSERT_FALSE(is_swagger_request("/notswagger", &test_config));
}

void test_is_swagger_request_root_url(void) {
    TEST_ASSERT_FALSE(is_swagger_request("/", &test_config));
}

//=============================================================================
// Custom Prefix Tests
//=============================================================================

void test_is_swagger_request_custom_prefix(void) {
    free(test_config.prefix);
    test_config.prefix = strdup("/docs");
    TEST_ASSERT_TRUE(is_swagger_request("/docs", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/docs/", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/docs/index.html", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_long_prefix(void) {
    free(test_config.prefix);
    test_config.prefix = strdup("/very/long/swagger/prefix");
    TEST_ASSERT_TRUE(is_swagger_request("/very/long/swagger/prefix", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/very/long/swagger/prefix/", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/very/long/swagger/prefix/index.html", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/very/long/swagger", &test_config));
}

void test_is_swagger_request_boundary_conditions(void) {
    // Test with single character prefix
    free(test_config.prefix);
    test_config.prefix = strdup("/s");
    TEST_ASSERT_TRUE(is_swagger_request("/s", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/s/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

//=============================================================================
// Complex Path Tests
//=============================================================================

void test_is_swagger_request_complex_paths(void) {
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/swagger-ui.js", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/swagger.json", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/swagger-initializer.js", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/css/swagger-ui.css", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/favicon-16x16.png", &test_config));
}

void test_is_swagger_request_special_characters(void) {
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger-ui");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger-ui", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger-ui/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
    
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger_ui");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger_ui", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger_ui/", &test_config));
    
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger.ui");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger.ui", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger.ui/", &test_config));
}

void test_is_swagger_request_numeric_prefixes(void) {
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger2");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger2", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger2/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
    
    free(test_config.prefix);
    test_config.prefix = strdup("/v1/swagger");
    TEST_ASSERT_TRUE(is_swagger_request("/v1/swagger", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/v1/swagger/", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/v1/swagger/index.html", &test_config));
}

void test_is_swagger_request_case_sensitivity(void) {
    // URL matching should be case-sensitive
    TEST_ASSERT_FALSE(is_swagger_request("/SWAGGER", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/Swagger", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/SwAgGeR", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger", &test_config));
}

void test_is_swagger_request_query_parameters(void) {
    // URLs with query parameters should NOT match as the function only checks for '/' or end of string
    TEST_ASSERT_FALSE(is_swagger_request("/swagger?param=value", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/?param=value", &test_config)); // This has '/' after prefix
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/index.html?param=value", &test_config)); // This has '/' after prefix
}

void test_is_swagger_request_fragments(void) {
    // URLs with fragments should NOT match as the function only checks for '/' or end of string
    TEST_ASSERT_FALSE(is_swagger_request("/swagger#section", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/#section", &test_config)); // This has '/' after prefix
    TEST_ASSERT_TRUE(is_swagger_request("/swagger/index.html#section", &test_config)); // This has '/' after prefix
}

//=============================================================================
// Configuration State Tests
//=============================================================================

void test_config_validation_all_disabled(void) {
    test_config.enabled = false;
    test_config.payload_available = false;
    free(test_config.prefix);
    test_config.prefix = NULL;
    
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/anything", &test_config));
}

void test_config_validation_enabled_but_no_payload(void) {
    test_config.enabled = true;
    test_config.payload_available = false;
    
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_config_validation_payload_but_disabled(void) {
    test_config.enabled = false;
    test_config.payload_available = true;
    
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_config_validation_empty_config(void) {
    // Test with completely empty config
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &empty_config));
    TEST_ASSERT_FALSE(is_swagger_request("/", &empty_config));
    TEST_ASSERT_FALSE(is_swagger_request("", &empty_config));
}

void test_config_validation_minimal_config(void) {
    // Test with minimal valid configuration
    TEST_ASSERT_TRUE(is_swagger_request("/api-docs", &minimal_config));
    TEST_ASSERT_TRUE(is_swagger_request("/api-docs/", &minimal_config));
    TEST_ASSERT_TRUE(is_swagger_request("/api-docs/index.html", &minimal_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &minimal_config));
}

//=============================================================================
// Edge Cases and Stress Tests
//=============================================================================

void test_parameter_validation_edge_cases(void) {
    // Test with very long URLs
    char long_url[1024];
    snprintf(long_url, sizeof(long_url), "/swagger/%0500d", 1);
    bool result = is_swagger_request(long_url, &test_config);
    TEST_ASSERT_TRUE(result); // Should handle long URLs
    
    // Test with URL containing null bytes (up to first null)
    const char url_with_null[] = "/swagger\0/hidden";
    TEST_ASSERT_TRUE(is_swagger_request(url_with_null, &test_config));
}

void test_url_matching_comprehensive_edge_cases(void) {
    // Test URL matching with various edge cases
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger-ui");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger-ui", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger-ui/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
    
    // Test URL matching with numbers
    free(test_config.prefix);
    test_config.prefix = strdup("/swagger2");
    TEST_ASSERT_TRUE(is_swagger_request("/swagger2", &test_config));
    TEST_ASSERT_TRUE(is_swagger_request("/swagger2/", &test_config));
    TEST_ASSERT_FALSE(is_swagger_request("/swagger", &test_config));
}

void test_url_matching_prefix_variations(void) {
    // Test various prefix patterns
    const char* prefixes[] = {
        "/",           // Root prefix
        "/api",        // Simple prefix
        "/v1/docs",    // Multi-level prefix
        "/swagger-ui", // Hyphenated prefix
        "/swagger_ui", // Underscored prefix
        "/SwaggerUI",  // Mixed case prefix
        "/docs/v2",    // Complex prefix
        NULL
    };
    
    for (int i = 0; prefixes[i] != NULL; i++) {
        free(test_config.prefix);
        test_config.prefix = strdup(prefixes[i]);
        
        // Test exact match
        TEST_ASSERT_TRUE(is_swagger_request(prefixes[i], &test_config));
        
        // Test with trailing slash
        char with_slash[256];
        snprintf(with_slash, sizeof(with_slash), "%s/", prefixes[i]);
        TEST_ASSERT_TRUE(is_swagger_request(with_slash, &test_config));
        
        // Test with nested path
        char with_path[256];
        snprintf(with_path, sizeof(with_path), "%s/index.html", prefixes[i]);
        TEST_ASSERT_TRUE(is_swagger_request(with_path, &test_config));
    }
}

void test_stress_many_url_tests(void) {
    // Test many different URL patterns
    const char* test_urls[] = {
        "/swagger", "/swagger/", "/swagger/index.html",
        "/swagger/css/style.css", "/swagger/js/app.js",
        "/swagger/swagger-ui.js", "/swagger/swagger.json",
        "/swagger/favicon.ico", "/swagger/fonts/font.woff",
        "/swagger/images/logo.png", "/swagger/docs/readme.md",
        NULL
    };
    
    for (int i = 0; test_urls[i] != NULL; i++) {
        TEST_ASSERT_TRUE(is_swagger_request(test_urls[i], &test_config));
    }
    
    const char* non_matching_urls[] = {
        "/", "/api", "/docs", "/swag", "/notswagger",
        "/api/swagger", "/prefix/swagger", "",
        "/SWAGGER", "/Swagger", "/swaggerui",
        NULL
    };
    
    for (int i = 0; non_matching_urls[i] != NULL; i++) {
        TEST_ASSERT_FALSE(is_swagger_request(non_matching_urls[i], &test_config));
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Basic parameter validation tests
    RUN_TEST(test_is_swagger_request_null_url);
    RUN_TEST(test_is_swagger_request_null_config);
    RUN_TEST(test_is_swagger_request_both_null);
    RUN_TEST(test_is_swagger_request_disabled_config);
    RUN_TEST(test_is_swagger_request_payload_not_available);
    RUN_TEST(test_is_swagger_request_null_prefix);
    RUN_TEST(test_is_swagger_request_empty_url);
    
    // Basic URL matching tests
    RUN_TEST(test_is_swagger_request_exact_match);
    RUN_TEST(test_is_swagger_request_with_trailing_slash);
    RUN_TEST(test_is_swagger_request_with_path);
    RUN_TEST(test_is_swagger_request_with_nested_path);
    RUN_TEST(test_is_swagger_request_wrong_prefix);
    RUN_TEST(test_is_swagger_request_partial_match);
    RUN_TEST(test_is_swagger_request_prefix_as_substring);
    RUN_TEST(test_is_swagger_request_root_url);
    
    // Custom prefix tests
    RUN_TEST(test_is_swagger_request_custom_prefix);
    RUN_TEST(test_is_swagger_request_long_prefix);
    RUN_TEST(test_is_swagger_request_boundary_conditions);
    
    // Complex path tests
    RUN_TEST(test_is_swagger_request_complex_paths);
    RUN_TEST(test_is_swagger_request_special_characters);
    RUN_TEST(test_is_swagger_request_numeric_prefixes);
    RUN_TEST(test_is_swagger_request_case_sensitivity);
    RUN_TEST(test_is_swagger_request_query_parameters);
    RUN_TEST(test_is_swagger_request_fragments);
    
    // Configuration state tests
    RUN_TEST(test_config_validation_all_disabled);
    RUN_TEST(test_config_validation_enabled_but_no_payload);
    RUN_TEST(test_config_validation_payload_but_disabled);
    RUN_TEST(test_config_validation_empty_config);
    RUN_TEST(test_config_validation_minimal_config);
    
    // Edge cases and stress tests
    RUN_TEST(test_parameter_validation_edge_cases);
    RUN_TEST(test_url_matching_comprehensive_edge_cases);
    RUN_TEST(test_url_matching_prefix_variations);
    RUN_TEST(test_stress_many_url_tests);
    
    return UNITY_END();
}
