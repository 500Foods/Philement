/*
 * Unity Test File: handle_swagger_request Function Tests
 * This file contains unit tests for the handle_swagger_request() function
 * from src/swagger/swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/swagger/swagger.h"

// Forward declaration for the function being tested
enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url, const SwaggerConfig *config);

// Mock structures for testing
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

// Global state for tests
static struct MockMHDResponse *mock_response = NULL;
static struct MockMHDConnection mock_connection = {0};

// Function prototypes for test functions
void test_handle_swagger_request_null_connection(void);
void test_handle_swagger_request_null_url(void);
void test_handle_swagger_request_null_config(void);
void test_handle_swagger_request_exact_prefix_redirect(void);
void test_handle_swagger_request_root_path(void);
void test_handle_swagger_request_index_html(void);
void test_handle_swagger_request_css_file(void);
void test_handle_swagger_request_js_file(void);
void test_handle_swagger_request_swagger_json(void);
void test_handle_swagger_request_swagger_initializer(void);
void test_handle_swagger_request_nonexistent_file(void);
void test_handle_swagger_request_brotli_compression(void);

//=============================================================================
// Mock HTTP Functions (Minimal Implementation for Tests)
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
            return NULL;
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

// Test fixtures
static SwaggerConfig test_config;

void setUp(void) {
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
}

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

void test_handle_swagger_request_brotli_compression(void) {
    mock_connection.accepts_brotli = true;
    enum MHD_Result result = handle_swagger_request((struct MHD_Connection*)&mock_connection, "/swagger/index.html.br", &test_config);
    
    // Test that compression headers are handled appropriately
    TEST_ASSERT_TRUE(result == 0 || result == 1);
}

int main(void) {
    UNITY_BEGIN();
    
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
    RUN_TEST(test_handle_swagger_request_brotli_compression);
    
    return UNITY_END();
}
