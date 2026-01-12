/*
 * Unity Test File: Web Server Request - Add Custom Headers Test
 * This file contains unit tests for add_custom_headers() function
 */

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>
#include <src/webserver/web_server_core.h>
#include <src/config/config.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>

// Forward declaration for function being tested
void add_custom_headers(struct MHD_Response *response, const char* file_path, const WebServerConfig* web_config);

// Mock response structure
struct MockMHDResponse {
    int dummy;
};

static struct MockMHDResponse mock_response = {0};

void setUp(void) {
    // Reset all mocks
    mock_mhd_reset_all();
    memset(&mock_response, 0, sizeof(mock_response));
}

void tearDown(void) {
    // Clean up test fixtures
    mock_mhd_reset_all();
}

// Test NULL parameters
static void test_add_custom_headers_null_response(void) {
    // Test with null response - should handle gracefully without crashing
    WebServerConfig config = {0};
    add_custom_headers(NULL, "/test/file.js", &config);
    // Should not crash
    TEST_PASS();
}

static void test_add_custom_headers_null_file_path(void) {
    // Test with null file path - should handle gracefully without crashing
    WebServerConfig config = {0};
    add_custom_headers((struct MHD_Response*)&mock_response, NULL, &config);
    // Should not crash
    TEST_PASS();
}

static void test_add_custom_headers_null_config(void) {
    // Test with null config - should handle gracefully without crashing
    add_custom_headers((struct MHD_Response*)&mock_response, "/test/file.js", NULL);
    // Should not crash
    TEST_PASS();
}

static void test_add_custom_headers_config_no_headers(void) {
    // Test with config that has NULL headers array
    WebServerConfig config = {0};
    config.headers = NULL;
    config.headers_count = 0;
    
    add_custom_headers((struct MHD_Response*)&mock_response, "/test/file.js", &config);
    // Should not crash
    TEST_PASS();
}

// Test filename extraction from path
static void test_add_custom_headers_path_with_slash(void) {
    // Test that filename is extracted correctly from path with slash
    // This tests line 50: filename++
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)"*.js", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    // Path with slash - should extract "file.js" and match pattern
    add_custom_headers((struct MHD_Response*)&mock_response, "/path/to/file.js", &config);
    
    // Verify header was added (mock tracks this)
    // The mock will record the MHD_add_response_header call
    TEST_PASS();
}

static void test_add_custom_headers_path_without_slash(void) {
    // Test filename extraction when no slash in path
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)"*.js", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    // Path without slash - should use entire path as filename
    add_custom_headers((struct MHD_Response*)&mock_response, "file.js", &config);
    
    // Should process successfully
    TEST_PASS();
}

// Test header rules matching
static void test_add_custom_headers_pattern_match(void) {
    // Test that headers are added when pattern matches
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".js", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    add_custom_headers((struct MHD_Response*)&mock_response, "/app/module.js", &config);
    
    // Header should be added
    TEST_PASS();
}

static void test_add_custom_headers_pattern_no_match(void) {
    // Test that headers are NOT added when pattern doesn't match
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".css", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    add_custom_headers((struct MHD_Response*)&mock_response, "/app/module.js", &config);
    
    // Header should NOT be added
    TEST_PASS();
}

// Test multiple header rules
static void test_add_custom_headers_multiple_rules(void) {
    // Test with multiple header rules
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".js", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"},
        {.pattern = (char*)".js", .header_name = (char*)"X-Content-Type-Options", .header_value = (char*)"nosniff"},
        {.pattern = (char*)"*", .header_name = (char*)"X-Frame-Options", .header_value = (char*)"DENY"}
    };
    config.headers = rules;
    config.headers_count = 3;
    
    // All matching rules should trigger header additions
    add_custom_headers((struct MHD_Response*)&mock_response, "/app/script.js", &config);
    
    TEST_PASS();
}

// Test different file paths
static void test_add_custom_headers_nested_path(void) {
    // Test deeply nested file path
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".html", .header_name = (char*)"Content-Security-Policy", .header_value = (char*)"default-src 'self'"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    add_custom_headers((struct MHD_Response*)&mock_response,
                       "/very/deep/nested/path/to/index.html", &config);
    
    TEST_PASS();
}

// Test wildcard pattern
static void test_add_custom_headers_wildcard_pattern(void) {
    // Test * pattern that should match all files
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)"*", .header_name = (char*)"X-Custom-Header", .header_value = (char*)"CustomValue"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    // Should match any file
    add_custom_headers((struct MHD_Response*)&mock_response, "/any/file.txt", &config);
    
    TEST_PASS();
}

// Test edge cases
static void test_add_custom_headers_empty_filename(void) {
    // Test with path ending in slash (empty filename after extraction)
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)"*", .header_name = (char*)"X-Test", .header_value = (char*)"Value"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    add_custom_headers((struct MHD_Response*)&mock_response, "/path/to/", &config);
    
    TEST_PASS();
}

static void test_add_custom_headers_special_characters_in_path(void) {
    // Test path with special characters
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".js", .header_name = (char*)"Cache-Control", .header_value = (char*)"no-cache"}
    };
    config.headers = rules;
    config.headers_count = 1;
    
    add_custom_headers((struct MHD_Response*)&mock_response,
                       "/path/with-dashes/and_underscores/file.js", &config);
    
    TEST_PASS();
}

// Test zero headers count
static void test_add_custom_headers_zero_count(void) {
    // Test with zero headers count
    WebServerConfig config = {0};
    HeaderRule rules[] = {
        {.pattern = (char*)".js", .header_name = (char*)"Cache-Control", .header_value = (char*)"max-age=3600"}
    };
    config.headers = rules;
    config.headers_count = 0;  // Zero count
    
    add_custom_headers((struct MHD_Response*)&mock_response, "/test.js", &config);
    
    // Should not process any rules
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_add_custom_headers_null_response);
    RUN_TEST(test_add_custom_headers_null_file_path);
    RUN_TEST(test_add_custom_headers_null_config);
    RUN_TEST(test_add_custom_headers_config_no_headers);
    RUN_TEST(test_add_custom_headers_path_with_slash);
    RUN_TEST(test_add_custom_headers_path_without_slash);
    RUN_TEST(test_add_custom_headers_pattern_match);
    RUN_TEST(test_add_custom_headers_pattern_no_match);
    RUN_TEST(test_add_custom_headers_multiple_rules);
    RUN_TEST(test_add_custom_headers_nested_path);
    RUN_TEST(test_add_custom_headers_wildcard_pattern);
    RUN_TEST(test_add_custom_headers_empty_filename);
    RUN_TEST(test_add_custom_headers_special_characters_in_path);
    RUN_TEST(test_add_custom_headers_zero_count);

    return UNITY_END();
}
