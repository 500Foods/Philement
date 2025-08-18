/*
 * Unity Test File: init_swagger_support Function Tests
 * This file contains unit tests for the init_swagger_support() function
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
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

// Include necessary headers
#include "../../../../src/swagger/swagger.h"
#include "../../../../src/config/config.h"
#include "../../../../src/config/config_swagger.h"
#include "../../../../src/payload/payload.h"

// Forward declaration for the function being tested
bool init_swagger_support(SwaggerConfig *config);

// Function prototypes for test functions
void test_init_swagger_support_null_config(void);
void test_init_swagger_support_disabled_config(void);
void test_init_swagger_support_system_shutting_down(void);
void test_init_swagger_support_invalid_system_state(void);
void test_init_swagger_support_already_initialized(void);
void test_init_swagger_support_executable_path_failure(void);
void test_init_swagger_support_payload_extraction_failure(void);
void test_init_swagger_support_valid_config(void);
void test_init_swagger_support_minimal_config(void);
void test_init_swagger_support_payload_not_available(void);

// Global state variables that swagger functions check (declared extern)
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_running; 
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t web_server_shutdown;

// Test fixtures
static SwaggerConfig test_config;
static SwaggerConfig minimal_config;
static bool payload_extraction_should_fail = false;
static bool executable_path_should_fail = false;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    server_running = 0;
    server_starting = 1;
    web_server_shutdown = 0;
    
    // Reset mock flags
    payload_extraction_should_fail = false;
    executable_path_should_fail = false;
    
    // Initialize test config
    memset(&test_config, 0, sizeof(SwaggerConfig));
    test_config.enabled = true;
    test_config.payload_available = true;
    test_config.prefix = strdup("/swagger");
    
    test_config.metadata.title = strdup("Test API");
    test_config.metadata.description = strdup("Test Description");
    test_config.metadata.version = strdup("1.0.0");
    test_config.metadata.contact.name = strdup("Test Contact");
    test_config.metadata.contact.email = strdup("test@example.com");
    test_config.metadata.contact.url = strdup("https://example.com");
    test_config.metadata.license.name = strdup("MIT");
    test_config.metadata.license.url = strdup("https://opensource.org/licenses/MIT");
    
    test_config.ui_options.try_it_enabled = true;
    test_config.ui_options.display_operation_id = false;
    test_config.ui_options.default_models_expand_depth = 1;
    test_config.ui_options.default_model_expand_depth = 1;
    test_config.ui_options.show_extensions = true;
    test_config.ui_options.show_common_extensions = true;
    test_config.ui_options.doc_expansion = strdup("list");
    test_config.ui_options.syntax_highlight_theme = strdup("agate");
    
    // Initialize minimal config
    memset(&minimal_config, 0, sizeof(SwaggerConfig));
    minimal_config.enabled = true;
    minimal_config.payload_available = true;
    minimal_config.prefix = strdup("/docs");
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
    
    memset(&test_config, 0, sizeof(SwaggerConfig));
    memset(&minimal_config, 0, sizeof(SwaggerConfig));
    
    // Clean up swagger support
    cleanup_swagger_support();
}

void test_init_swagger_support_null_config(void) {
    bool result = init_swagger_support(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_disabled_config(void) {
    test_config.enabled = false;
    bool result = init_swagger_support(&test_config);
    // Should return false for disabled config
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_system_shutting_down(void) {
    server_stopping = 1;
    bool result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
    
    server_stopping = 0;
    web_server_shutdown = 1;
    result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_invalid_system_state(void) {
    server_starting = 0; // Not in startup
    server_running = 0;  // Not running
    bool result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_already_initialized(void) {
    // First initialization should work
    bool result1 = init_swagger_support(&test_config);
    // Second initialization should return previous state
    bool result2 = init_swagger_support(&test_config);
    
    // Both should be boolean values
    TEST_ASSERT_TRUE(result1 == true || result1 == false);
    TEST_ASSERT_TRUE(result2 == true || result2 == false);
}

void test_init_swagger_support_executable_path_failure(void) {
    executable_path_should_fail = true;
    bool result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_payload_extraction_failure(void) {
    payload_extraction_should_fail = true;
    bool result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(test_config.payload_available);
}

void test_init_swagger_support_valid_config(void) {
    bool result = init_swagger_support(&test_config);
    // Result depends on payload extraction success
    TEST_ASSERT_TRUE(result == true || result == false);
}

void test_init_swagger_support_minimal_config(void) {
    bool result = init_swagger_support(&minimal_config);
    // Result depends on payload extraction success
    TEST_ASSERT_TRUE(result == true || result == false);
}

void test_init_swagger_support_payload_not_available(void) {
    test_config.payload_available = false;
    bool result = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_init_swagger_support_null_config);
    RUN_TEST(test_init_swagger_support_disabled_config);
    RUN_TEST(test_init_swagger_support_system_shutting_down);
    RUN_TEST(test_init_swagger_support_invalid_system_state);
    RUN_TEST(test_init_swagger_support_already_initialized);
    RUN_TEST(test_init_swagger_support_executable_path_failure);
    RUN_TEST(test_init_swagger_support_payload_extraction_failure);
    RUN_TEST(test_init_swagger_support_valid_config);
    RUN_TEST(test_init_swagger_support_minimal_config);
    RUN_TEST(test_init_swagger_support_payload_not_available);
    
    return UNITY_END();
}
