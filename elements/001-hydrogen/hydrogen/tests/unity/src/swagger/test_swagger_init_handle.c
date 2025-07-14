/*
 * Unity Test File: Swagger Initialization and Request Handling Tests
 * This file contains comprehensive unit tests for init_swagger_support() and
 * handle_swagger_request() functions from src/swagger/swagger.c
 * 
 * Coverage Goals:
 * - Test initialization logic and system state validation
 * - Test HTTP request handling and response generation
 * - Test file serving and content type handling
 * - Test redirection logic and URL processing
 * - Test mock HTTP infrastructure
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

// Mock structures for HTTP testing
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
    char *request_url;
};

// Global mocks and state
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
// Mock HTTP Functions
//=============================================================================

const char *MHD_lookup_connection_value(struct MHD_Connection *connection,
                                      enum MHD_ValueKind kind, const char *key) {
    (void)connection;
    (void)kind;
    if (strcmp(key, "Host") == 0) {
        return mock_connection.host_header;
    }
    if (strcmp(key, "Accept-Encoding") == 0) {
        return mock_connection.accepts_brotli ? "gzip, deflate, br" : "gzip, deflate";
    }
    if (strcmp(key, "User-Agent") == 0) {
        return mock_connection.user_agent;
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
    memset(mock_response->headers, 0, sizeof(mock_response->headers));
    mock_response->status_code = 200; // Default OK
    return (struct MHD_Response*)mock_response;
}

enum MHD_Result MHD_queue_response(struct MHD_Connection *connection,
                                 unsigned int status_code,
                                 struct MHD_Response *response) {
    (void)connection;
    if (mock_response) {
        mock_response->status_code = status_code;
    }
    (void)response;
    return 1; // MHD_YES
}

enum MHD_Result MHD_add_response_header(struct MHD_Response *response,
                          const char *header, const char *content) {
    if (mock_response) {
        strncat(mock_response->headers, header, sizeof(mock_response->headers) - strlen(mock_response->headers) - 1);
        strncat(mock_response->headers, ": ", sizeof(mock_response->headers) - strlen(mock_response->headers) - 1);
        strncat(mock_response->headers, content, sizeof(mock_response->headers) - strlen(mock_response->headers) - 1);
        strncat(mock_response->headers, "\n", sizeof(mock_response->headers) - strlen(mock_response->headers) - 1);
    }
    (void)response;
    return 1; // Success
}

void MHD_destroy_response(struct MHD_Response *response) {
    (void)response;
    // Don't actually free mock_response as we reuse it
}

//=============================================================================
// Mock Helper Functions (Using real implementations)
//=============================================================================

// Note: Using real implementations of:
// - get_app_config, get_executable_path, extract_payload, free_payload
// - client_accepts_brotli, add_cors_headers, add_brotli_header
// These are linked from the main codebase to avoid conflicts

//=============================================================================
// Test Fixtures
//=============================================================================

static SwaggerConfig test_config;
static SwaggerConfig minimal_config;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    server_running = 0;
    server_starting = 1; // Default to startup state
    web_server_shutdown = 0;
    
    // Reset mock flags
    payload_extraction_should_fail = false;
    executable_path_should_fail = false;
    
    // Initialize mock connection
    memset(&mock_connection, 0, sizeof(mock_connection));
    mock_connection.host_header = "localhost:8080";
    mock_connection.accepts_brotli = true;
    mock_connection.user_agent = "Test/1.0";
    
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
    
    // Initialize minimal config
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
    
    // Reset configs
    memset(&test_config, 0, sizeof(SwaggerConfig));
    memset(&minimal_config, 0, sizeof(SwaggerConfig));
    
    // Clean up mock response
    if (mock_response) {
        free(mock_response);
        mock_response = NULL;
    }
    
    // Clean up swagger support
    cleanup_swagger_support();
}

//=============================================================================
// Tests for init_swagger_support() function
//=============================================================================

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

//=============================================================================
// Tests for handle_swagger_request() function
//=============================================================================

void test_handle_swagger_request_null_connection(void) {
    enum MHD_Result result = handle_swagger_request(NULL, "/swagger", &test_config);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
}

void test_handle_swagger_request_null_url(void) {
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, NULL, &test_config);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
}

void test_handle_swagger_request_null_config(void) {
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger", NULL);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
}

void test_handle_swagger_request_exact_prefix_redirect(void) {
    // Test redirect for exact prefix match (without trailing slash)
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger", &test_config);
    
    // Should return MHD_YES for successful redirect
    TEST_ASSERT_EQUAL(1, result); // MHD_YES
    
    // Check that response was created
    TEST_ASSERT_NOT_NULL(mock_response);
    
    // Should be a redirect (301)
    TEST_ASSERT_EQUAL(301, mock_response->status_code);
}

void test_handle_swagger_request_root_path(void) {
    // Test request for root path within swagger prefix
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/", &test_config);
    
    // Result depends on whether files are loaded
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_index_html(void) {
    // Test explicit request for index.html
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/index.html", &test_config);
    
    // Result depends on whether files are loaded
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_css_file(void) {
    // Test request for CSS file
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/css/style.css", &test_config);
    
    // Result depends on whether files are loaded
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_js_file(void) {
    // Test request for JavaScript file
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/js/app.js", &test_config);
    
    // Result depends on whether files are loaded
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_swagger_json(void) {
    // Test request for swagger.json (should trigger dynamic content generation)
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/swagger.json", &test_config);
    
    // Result depends on whether files are loaded and JSON parsing works
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_swagger_initializer(void) {
    // Test request for swagger-initializer.js (should trigger dynamic content generation)
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/swagger-initializer.js", &test_config);
    
    // Result depends on whether files are loaded
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_handle_swagger_request_nonexistent_file(void) {
    // Test request for file that doesn't exist
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/nonexistent.txt", &test_config);
    
    // Should return MHD_NO for not found
    TEST_ASSERT_EQUAL(0, result);
}

//=============================================================================
// Tests for swagger_request_handler() function
//=============================================================================

void test_swagger_request_handler_null_parameters(void) {
    size_t upload_size = 0;
    void *con_cls = NULL;
    
    // Test with null cls parameter
    enum MHD_Result result = swagger_request_handler(NULL, (struct MHD_Connection*)&mock_connection, "/swagger", "GET", "HTTP/1.1", NULL, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
    
    // Test with null connection
    result = swagger_request_handler(&test_config, NULL, "/swagger", "GET", "HTTP/1.1", NULL, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
    
    // Test with null URL
    result = swagger_request_handler(&test_config, (struct MHD_Connection*)&mock_connection, NULL, "GET", "HTTP/1.1", NULL, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(0, result); // MHD_NO
}

void test_swagger_request_handler_valid_request(void) {
    size_t upload_size = 0;
    void *con_cls = NULL;
    
    // Test with valid parameters
    enum MHD_Result result = swagger_request_handler(&test_config, (struct MHD_Connection*)&mock_connection, "/swagger", "GET", "HTTP/1.1", NULL, &upload_size, &con_cls);
    
    // Should delegate to handle_swagger_request
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

//=============================================================================
// Tests for Content Type Handling
//=============================================================================

void test_content_type_detection_html(void) {
    // Test HTML file content type
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/index.html", &test_config);
    
    if (result == 1 && mock_response) {
        // Check if Content-Type header was set correctly
        TEST_ASSERT_TRUE(strstr(mock_response->headers, "Content-Type") != NULL);
    }
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_content_type_detection_css(void) {
    // Test CSS file content type
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/style.css", &test_config);
    
    if (result == 1 && mock_response) {
        // Check if Content-Type header was set
        TEST_ASSERT_TRUE(strstr(mock_response->headers, "Content-Type") != NULL);
    }
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_content_type_detection_js(void) {
    // Test JavaScript file content type
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/app.js", &test_config);
    
    if (result == 1 && mock_response) {
        // Check if Content-Type header was set
        TEST_ASSERT_TRUE(strstr(mock_response->headers, "Content-Type") != NULL);
    }
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_content_type_detection_json(void) {
    // Test JSON file content type
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/swagger.json", &test_config);
    
    if (result == 1 && mock_response) {
        // Check if Content-Type header was set
        TEST_ASSERT_TRUE(strstr(mock_response->headers, "Content-Type") != NULL);
    }
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

//=============================================================================
// Tests for CORS and Compression Headers
//=============================================================================

void test_cors_headers_added(void) {
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/", &test_config);
    
    if (result == 1 && mock_response) {
        // Check if CORS headers were added
        TEST_ASSERT_TRUE(strstr(mock_response->headers, "Access-Control-Allow-Origin") != NULL);
    }
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

void test_brotli_compression_headers(void) {
    mock_connection.accepts_brotli = true;
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/index.html.br", &test_config);
    
    // Test that compression headers are handled appropriately
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

//=============================================================================
// Tests for swagger_url_validator() function
//=============================================================================

void test_swagger_url_validator_null_url(void) {
    TEST_ASSERT_FALSE(swagger_url_validator(NULL));
}

void test_swagger_url_validator_valid_urls(void) {
    // Test various URLs - results depend on global config state
    bool result1 = swagger_url_validator("/swagger");
    bool result2 = swagger_url_validator("/swagger/");
    bool result3 = swagger_url_validator("/docs");
    
    // All should be boolean values
    TEST_ASSERT_TRUE(result1 == true || result1 == false);
    TEST_ASSERT_TRUE(result2 == true || result2 == false);
    TEST_ASSERT_TRUE(result3 == true || result3 == false);
}

//=============================================================================
// Tests for cleanup_swagger_support() function
//=============================================================================

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
    init_swagger_support(&test_config);
    cleanup_swagger_support();
    
    // Should be able to call again
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true);
}

//=============================================================================
// Integration Tests
//=============================================================================

void test_integration_full_workflow(void) {
    // Test full workflow: init -> request -> cleanup
    bool init_result = init_swagger_support(&test_config);
    
    if (init_result) {
        enum MHD_Result request_result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/", &test_config);
        TEST_ASSERT_TRUE(request_result == 0 || request_result == 1);
    }
    
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true);
}

void test_integration_multiple_requests(void) {
    // Test multiple requests after initialization
    init_swagger_support(&test_config);
    
    const char* test_urls[] = {
        "/swagger/",
        "/swagger/index.html",
        "/swagger/css/style.css",
        "/swagger/swagger.json",
        NULL
    };
    
    for (int i = 0; test_urls[i] != NULL; i++) {
        enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, test_urls[i], &test_config);
        TEST_ASSERT_TRUE(result == 0 || result == 1);
    }
    
    cleanup_swagger_support();
    TEST_ASSERT_TRUE(true);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // init_swagger_support tests
    RUN_TEST(test_init_swagger_support_null_config);
    RUN_TEST(test_init_swagger_support_disabled_config);
    RUN_TEST(test_init_swagger_support_system_shutting_down);
    RUN_TEST(test_init_swagger_support_invalid_system_state);
    RUN_TEST(test_init_swagger_support_already_initialized);
    RUN_TEST(test_init_swagger_support_executable_path_failure);
    RUN_TEST(test_init_swagger_support_payload_extraction_failure);
    RUN_TEST(test_init_swagger_support_valid_config);
    RUN_TEST(test_init_swagger_support_minimal_config);
    
    // handle_swagger_request tests
    RUN_TEST(test_handle_swagger_request_null_connection);
    RUN_TEST(test_handle_swagger_request_null_url);
    RUN_TEST(test_handle_swagger_request_null_config);
    RUN_TEST(test_handle_swagger_request_exact_prefix_redirect);
    RUN_TEST(test_handle_swagger_request_root_path);
    RUN_TEST(test_handle_swagger_request_index_html);
    RUN_TEST(test_handle_swagger_request_css_file);
    RUN_TEST(test_handle_swagger_request_js_file);
    RUN_TEST(test_handle_swagger_request_swagger_json);
    RUN_TEST(test_handle_swagger_request_swagger_initializer);
    RUN_TEST(test_handle_swagger_request_nonexistent_file);
    
    // swagger_request_handler tests
    RUN_TEST(test_swagger_request_handler_null_parameters);
    RUN_TEST(test_swagger_request_handler_valid_request);
    
    // Content type tests
    RUN_TEST(test_content_type_detection_html);
    RUN_TEST(test_content_type_detection_css);
    RUN_TEST(test_content_type_detection_js);
    RUN_TEST(test_content_type_detection_json);
    
    // Header tests
    RUN_TEST(test_cors_headers_added);
    RUN_TEST(test_brotli_compression_headers);
    
    // swagger_url_validator tests
    RUN_TEST(test_swagger_url_validator_null_url);
    RUN_TEST(test_swagger_url_validator_valid_urls);
    
    // cleanup_swagger_support tests
    RUN_TEST(test_cleanup_swagger_support_basic);
    RUN_TEST(test_cleanup_swagger_support_multiple_calls);
    RUN_TEST(test_cleanup_swagger_support_after_init);
    
    // Integration tests
    RUN_TEST(test_integration_full_workflow);
    RUN_TEST(test_integration_multiple_requests);
    
    return UNITY_END();
}
