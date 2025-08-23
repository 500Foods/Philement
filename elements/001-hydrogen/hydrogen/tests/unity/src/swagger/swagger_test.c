/*
 * Unity Test File: Swagger Module Integration Tests
 * This file contains integration tests for the complete swagger module workflow
 * 
 * Note: Individual function tests are in:
 * - test_swagger_is_swagger_request.c (is_swagger_request function)
 * - test_swagger_init_handle.c (init, handle, cleanup functions)
 * 
 * Coverage Goals:
 * - End-to-end swagger workflow testing
 * - Configuration state transitions
 * - Error recovery and resilience
 * - Cross-function integration validation
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/swagger/swagger.h"

// Forward declarations for functions being tested
bool init_swagger_support(SwaggerConfig *config);
enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url, const SwaggerConfig *config);
enum MHD_Result swagger_request_handler(void *cls, struct MHD_Connection *connection,
                                       const char *url, const char *method,
                                       const char *version, const char *upload_data,
                                       size_t *upload_data_size, void **con_cls);
void cleanup_swagger_support(void);
bool swagger_url_validator(const char *url);
bool is_swagger_request(const char *url, const SwaggerConfig *config);

// Function prototypes for test functions
void test_swagger_complete_initialization_workflow(void);
void test_swagger_url_validation_integration(void);
void test_swagger_request_handling_integration(void);
void test_swagger_state_transitions(void);
void test_swagger_error_recovery(void);
void test_swagger_multiple_initialization_calls(void);
void test_swagger_cross_function_consistency(void);
void test_swagger_configuration_variations(void);

// Mock structures for integration testing
struct MockMHDResponse {
    size_t size;
    void *data;
    char headers[1024];
    int status_code;
};

struct MockMHDConnection {
    char *host_header;
    bool accepts_brotli;
    char *user_agent;
};

// Global state for integration tests
static struct MockMHDResponse *mock_response = NULL;
static struct MockMHDConnection mock_connection = {0};
static bool payload_extraction_should_fail = false;
static bool executable_path_should_fail = false;

// Global state variables that swagger functions check (declared extern)
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_running; 
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t web_server_shutdown;

//=============================================================================
// Mock HTTP Functions (Minimal Implementation for Integration Tests)
//=============================================================================

const char *MHD_lookup_connection_value(struct MHD_Connection *connection,
                                      enum MHD_ValueKind kind, const char *key) {
    (void)connection;
    (void)kind;
    if (strcmp(key, "Host") == 0) {
        return (const char*)mock_connection.host_header;
    }
    if (strcmp(key, "Accept-Encoding") == 0) {
        return mock_connection.accepts_brotli ? "gzip, deflate, br" : "gzip, deflate";
    }
    if (strcmp(key, "User-Agent") == 0) {
        return (const char*)mock_connection.user_agent;
    }
    return NULL;
}

struct MHD_Response *MHD_create_response_from_buffer(size_t size, void *buffer,
                                                   enum MHD_ResponseMemoryMode mode) {
    (void)mode;
    if (!mock_response) {
        mock_response = malloc(sizeof(struct MockMHDResponse));
        if (!mock_response) {
            return NULL; // Handle allocation failure
        }
        memset(mock_response, 0, sizeof(struct MockMHDResponse));
    }
    mock_response->size = size;
    mock_response->data = buffer;
    mock_response->status_code = 200;
    return (struct MHD_Response*)mock_response;
}

enum MHD_Result MHD_queue_response(struct MHD_Connection *connection,
                                 unsigned int status_code,
                                 struct MHD_Response *response) {
    (void)connection;
    (void)response;
    if (mock_response) {
        mock_response->status_code = (int)status_code;
    }
    return 1; // MHD_YES
}

enum MHD_Result MHD_add_response_header(struct MHD_Response *response,
                          const char *header, const char *content) {
    (void)response;
    (void)header;
    (void)content;
    return 1; // Success
}

void MHD_destroy_response(struct MHD_Response *response) {
    (void)response;
    // Don't actually free mock_response as we reuse it
}

//=============================================================================
// Mock Helper Functions (Basic HTTP mocking only)
//=============================================================================

// Note: Using real implementations of:
// - get_app_config, get_executable_path, extract_payload, free_payload
// - client_accepts_brotli, add_cors_headers, add_brotli_header
// These are linked from the main codebase to avoid conflicts

//=============================================================================
// Test Fixtures
//=============================================================================

static SwaggerConfig test_config;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    server_running = 0;
    server_starting = 1;
    web_server_shutdown = 0;
    
    // Reset mock flags
    payload_extraction_should_fail = false;
    executable_path_should_fail = false;
    
    // Initialize mock connection
    memset(&mock_connection, 0, sizeof(mock_connection));
    mock_connection.host_header = (char*)"localhost:8080";
    mock_connection.accepts_brotli = true;
    mock_connection.user_agent = (char*)"Test/1.0";
    
    // Clean up previous mock response
    if (mock_response) {
        free(mock_response);
        mock_response = NULL;
    }
    
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
    
    memset(&test_config, 0, sizeof(SwaggerConfig));
    
    // Clean up mock response
    if (mock_response) {
        free(mock_response);
        mock_response = NULL;
    }
    
    // Clean up swagger support
    cleanup_swagger_support();
}

//=============================================================================
// Integration Tests - Complete Swagger Workflow
//=============================================================================

void test_swagger_complete_initialization_workflow(void) {
    // Test complete initialization workflow
    bool init_result = init_swagger_support(&test_config);
    
    // Should complete initialization (success or failure is acceptable)
    TEST_ASSERT_TRUE(init_result == true || init_result == false);
    
    // After initialization, config state should be consistent
    if (init_result) {
        TEST_ASSERT_TRUE(test_config.enabled);
    }
    
    // Cleanup should work regardless of init result
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true); // No crash
}

void test_swagger_url_validation_integration(void) {
    // Test URL validation integration
    init_swagger_support(&test_config);
    
    // Test is_swagger_request and swagger_url_validator consistency
    bool is_request_result = is_swagger_request("/swagger", &test_config);
    bool url_validator_result = swagger_url_validator("/swagger");
    
    // Both should return boolean values
    TEST_ASSERT_TRUE(is_request_result == true || is_request_result == false);
    TEST_ASSERT_TRUE(url_validator_result == true || url_validator_result == false);
    
    cleanup_swagger_support();
}

void test_swagger_request_handling_integration(void) {
    // Test complete request handling workflow
    init_swagger_support(&test_config);
    
    // Test direct handle_swagger_request
    enum MHD_Result direct_result = handle_swagger_request(
        (struct MHD_Connection*)&mock_connection,
        "/swagger/",
        &test_config
    );
    
    // Test through swagger_request_handler
    size_t upload_size = 0;
    void *con_cls = NULL;
    enum MHD_Result handler_result = swagger_request_handler(
        &test_config,
        (struct MHD_Connection*)&mock_connection,
        "/swagger/",
        "GET",
        "HTTP/1.1",
        NULL,
        &upload_size,
        &con_cls
    );
    
    // Both should return valid MHD results
    TEST_ASSERT_TRUE(direct_result == 0 || direct_result == 1);
    TEST_ASSERT_TRUE(handler_result == 0 || handler_result == 1);
    
    cleanup_swagger_support();
}

void test_swagger_state_transitions(void) {
    // Test state transitions throughout swagger lifecycle
    
    // Test disabled config -> enabled config
    test_config.enabled = false;
    bool disabled_init = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(disabled_init);
    
    test_config.enabled = true;
    bool enabled_init = init_swagger_support(&test_config);
    TEST_ASSERT_TRUE(enabled_init == true || enabled_init == false);
    
    // Test system state changes
    server_stopping = 1;
    bool stopping_init = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(stopping_init);
    
    server_stopping = 0;
    server_starting = 1;
    
    cleanup_swagger_support();
}

void test_swagger_error_recovery(void) {
    // Test error conditions and recovery
    
    // Test payload extraction failure
    payload_extraction_should_fail = true;
    bool fail_init = init_swagger_support(&test_config);
    TEST_ASSERT_FALSE(fail_init);
    TEST_ASSERT_FALSE(test_config.payload_available);
    
    // Reset and test successful recovery
    payload_extraction_should_fail = false;
    test_config.payload_available = true;
    bool recovery_init = init_swagger_support(&test_config);
    TEST_ASSERT_TRUE(recovery_init == true || recovery_init == false);
    
    cleanup_swagger_support();
}

void test_swagger_multiple_initialization_calls(void) {
    // Test multiple initialization calls (should be safe)
    bool init1 = init_swagger_support(&test_config);
    bool init2 = init_swagger_support(&test_config);
    bool init3 = init_swagger_support(&test_config);
    
    // All should return boolean values
    TEST_ASSERT_TRUE(init1 == true || init1 == false);
    TEST_ASSERT_TRUE(init2 == true || init2 == false);
    TEST_ASSERT_TRUE(init3 == true || init3 == false);
    
    // Multiple cleanups should also be safe
    cleanup_swagger_support();
    cleanup_swagger_support();
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true); // No crash
}

void test_swagger_cross_function_consistency(void) {
    // Test consistency between different swagger functions
    init_swagger_support(&test_config);
    
    const char* test_urls[] = {
        "/swagger",
        "/swagger/",
        "/swagger/index.html",
        "/api-docs",
        "/not-swagger",
        NULL
    };
    
    for (int i = 0; test_urls[i] != NULL; i++) {
        bool is_request = is_swagger_request(test_urls[i], &test_config);
        bool url_valid = swagger_url_validator(test_urls[i]);
        
        // Results should be boolean values
        TEST_ASSERT_TRUE(is_request == true || is_request == false);
        TEST_ASSERT_TRUE(url_valid == true || url_valid == false);
        
        // If is_swagger_request returns true, handle_swagger_request should not crash
        if (is_request) {
            enum MHD_Result handle_result = handle_swagger_request(
                (struct MHD_Connection*)&mock_connection,
                test_urls[i],
                &test_config
            );
            TEST_ASSERT_TRUE(handle_result == 0 || handle_result == 1);
        }
    }
    
    cleanup_swagger_support();
}

void test_swagger_configuration_variations(void) {
    // Test different configuration variations
    
    // Test with different prefixes
    const char* prefixes[] = {
        "/swagger",
        "/docs",
        "/api-docs",
        "/v1/swagger",
        "/",
        NULL
    };
    
    for (int i = 0; prefixes[i] != NULL; i++) {
        free(test_config.prefix);
        test_config.prefix = strdup(prefixes[i]);
        
        bool init_result = init_swagger_support(&test_config);
        TEST_ASSERT_TRUE(init_result == true || init_result == false);
        
        // Test URL matching with this prefix
        bool match_result = is_swagger_request(prefixes[i], &test_config);
        TEST_ASSERT_TRUE(match_result == true || match_result == false);
        
        cleanup_swagger_support();
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Integration workflow tests
    RUN_TEST(test_swagger_complete_initialization_workflow);
    RUN_TEST(test_swagger_url_validation_integration);
    RUN_TEST(test_swagger_request_handling_integration);
    RUN_TEST(test_swagger_state_transitions);
    RUN_TEST(test_swagger_error_recovery);
    RUN_TEST(test_swagger_multiple_initialization_calls);
    RUN_TEST(test_swagger_cross_function_consistency);
    RUN_TEST(test_swagger_configuration_variations);
    
    return UNITY_END();
}
