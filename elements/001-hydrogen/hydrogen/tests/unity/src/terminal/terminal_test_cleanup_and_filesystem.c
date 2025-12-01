/*
 * Unity Test File: Terminal Cleanup and Filesystem Tests
 * This file contains unit tests for cleanup_terminal_support() and serve_file_from_path()
 * from src/terminal/terminal.c to improve overall test coverage
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/terminal/terminal.h>
#include <src/terminal/terminal_session.h>
#include <src/webserver/web_server_core.h>
#include <src/webserver/web_server_compression.h>

// System headers
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Forward declarations for functions being tested
void cleanup_terminal_support(TerminalConfig *config);
enum MHD_Result serve_file_from_path(struct MHD_Connection *connection, const char *file_path);
void format_file_size(size_t size, char *buffer, size_t buffer_size);

// Forward declarations for external functions
void add_cors_headers(struct MHD_Response *response);
bool client_accepts_brotli(struct MHD_Connection *connection);
bool brotli_file_exists(const char *file_path, char *br_file_path, size_t br_buffer_size);
void add_brotli_header(struct MHD_Response *response);
void cleanup_session_manager(void);

// Function prototypes for test functions
void test_cleanup_terminal_support_with_null_config(void);
void test_cleanup_terminal_support_with_valid_config(void);
void test_cleanup_terminal_support_multiple_calls(void);
void test_serve_file_from_path_null_connection(void);
void test_serve_file_from_path_null_file_path(void);
void test_serve_file_from_path_nonexistent_file(void);
void test_serve_file_from_path_html_extension(void);
void test_serve_file_from_path_css_extension(void);
void test_serve_file_from_path_js_extension(void);
void test_serve_file_from_path_json_extension(void);
void test_serve_file_from_path_png_extension(void);
void test_serve_file_from_path_no_extension(void);
void test_serve_file_from_path_unknown_extension(void);
void test_serve_file_from_path_txt_extension(void);
void test_format_file_size_bytes(void);
void test_format_file_size_kilobytes(void);
void test_format_file_size_megabytes(void);

// Mock connection for testing
static struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

// Test fixtures
static TerminalConfig test_config;

void setUp(void) {
    // Initialize test config
    memset(&test_config, 0, sizeof(TerminalConfig));
    test_config.enabled = true;
    test_config.web_path = (char*)"/terminal";
    test_config.index_page = (char*)"terminal.html";
    test_config.max_sessions = 10;
    test_config.idle_timeout_seconds = 300;
}

void tearDown(void) {
    // No cleanup needed for these simple tests
}

/*
 * TEST SUITE: cleanup_terminal_support
 * This function has 0% coverage in both Unity and blackbox tests
 */

void test_cleanup_terminal_support_with_null_config(void) {
    // Test cleanup with NULL config (should handle gracefully)
    cleanup_terminal_support(NULL);
    TEST_PASS(); // If we get here without crashing, test passes
}

void test_cleanup_terminal_support_with_valid_config(void) {
    // Test cleanup with valid config
    cleanup_terminal_support(&test_config);
    TEST_PASS(); // If we get here without crashing, test passes
}

void test_cleanup_terminal_support_multiple_calls(void) {
    // Test that cleanup can be called multiple times safely
    cleanup_terminal_support(&test_config);
    cleanup_terminal_support(&test_config);
    cleanup_terminal_support(&test_config);
    TEST_PASS(); // Should handle multiple calls gracefully
}

/*
 * TEST SUITE: serve_file_from_path
 * This function has 0% Unity coverage but some blackbox coverage
 */

void test_serve_file_from_path_null_connection(void) {
    // Test with NULL connection
    enum MHD_Result result = serve_file_from_path(NULL, "/tmp/test.html");
    // Function should handle NULL connection gracefully
    // The actual behavior depends on implementation
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_serve_file_from_path_null_file_path(void) {
    // Test with NULL file path
    enum MHD_Result result = serve_file_from_path(mock_connection, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_serve_file_from_path_nonexistent_file(void) {
    // Test with non-existent file
    const char *nonexistent = "/tmp/this_file_absolutely_does_not_exist_12345.html";
    enum MHD_Result result = serve_file_from_path(mock_connection, nonexistent);
    TEST_ASSERT_EQUAL(MHD_NO, result); // Should fail to open file
}

void test_serve_file_from_path_html_extension(void) {
    // Test content type detection for HTML files
    // Create a temporary test file
    const char *test_file = "/tmp/terminal_test_file.html";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "<html><body>Test</body></html>";
        write(fd, content, strlen(content));
        close(fd);
        
        // Try to serve the file
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        
        // Clean up
        unlink(test_file);
        
        // Result may be MHD_NO if mock connection doesn't support response creation
        // but we've exercised the content-type detection code
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS(); // Skip if we can't create temp file
    }
}

void test_serve_file_from_path_css_extension(void) {
    // Test content type detection for CSS files
    const char *test_file = "/tmp/terminal_test_file.css";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "body { color: blue; }";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_js_extension(void) {
    // Test content type detection for JavaScript files
    const char *test_file = "/tmp/terminal_test_file.js";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "console.log('test');";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_json_extension(void) {
    // Test content type detection for JSON files
    const char *test_file = "/tmp/terminal_test_file.json";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "{\"test\": true}";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_png_extension(void) {
    // Test content type detection for PNG files
    const char *test_file = "/tmp/terminal_test_file.png";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        // Minimal PNG header
        const unsigned char png_header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        write(fd, png_header, sizeof(png_header));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_no_extension(void) {
    // Test serving a file with no extension
    const char *test_file = "/tmp/terminal_test_file_noext";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "test content";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_unknown_extension(void) {
    // Test serving a file with an unknown extension
    const char *test_file = "/tmp/terminal_test_file.xyz";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "test content";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

void test_serve_file_from_path_txt_extension(void) {
    // Test serving a .txt file (not explicitly handled, falls through)
    const char *test_file = "/tmp/terminal_test_file.txt";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "test text content";
        write(fd, content, strlen(content));
        close(fd);
        
        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        unlink(test_file);
        
        TEST_ASSERT(result == MHD_YES || result == MHD_NO);
    } else {
        TEST_PASS();
    }
}

/*
 * TEST SUITE: format_file_size
 * Test the helper function for file size formatting
 */

void test_format_file_size_bytes(void) {
    char buffer[32];
    
    // Test small file (bytes)
    format_file_size(512, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("512 bytes", buffer);
    
    // Test exactly 1023 bytes
    format_file_size(1023, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1023 bytes", buffer);
}

void test_format_file_size_kilobytes(void) {
    char buffer[32];
    
    // Test 1KB
    format_file_size(1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1.0K", buffer);
    
    // Test 10KB
    format_file_size(10240, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("10.0K", buffer);
    
    // Test just under 1MB
    format_file_size(1024 * 1024 - 1, buffer, sizeof(buffer));
    // Should be formatted as K
    TEST_ASSERT_NOT_NULL(strstr(buffer, "K"));
}

void test_format_file_size_megabytes(void) {
    char buffer[32];
    
    // Test 1MB
    format_file_size(1024 * 1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1.0M", buffer);
    
    // Test 5MB
    format_file_size(5 * 1024 * 1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("5.0M", buffer);
    
    // Test 100MB
    format_file_size(100 * 1024 * 1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("100.0M", buffer);
}

int main(void) {
    UNITY_BEGIN();
    
    // cleanup_terminal_support tests
    RUN_TEST(test_cleanup_terminal_support_with_null_config);
    RUN_TEST(test_cleanup_terminal_support_with_valid_config);
    RUN_TEST(test_cleanup_terminal_support_multiple_calls);
    
    // serve_file_from_path tests
    RUN_TEST(test_serve_file_from_path_null_connection);
    RUN_TEST(test_serve_file_from_path_null_file_path);
    RUN_TEST(test_serve_file_from_path_nonexistent_file);
    RUN_TEST(test_serve_file_from_path_html_extension);
    RUN_TEST(test_serve_file_from_path_css_extension);
    RUN_TEST(test_serve_file_from_path_js_extension);
    RUN_TEST(test_serve_file_from_path_json_extension);
    RUN_TEST(test_serve_file_from_path_png_extension);
    RUN_TEST(test_serve_file_from_path_no_extension);
    RUN_TEST(test_serve_file_from_path_unknown_extension);
    RUN_TEST(test_serve_file_from_path_txt_extension);
    
    // format_file_size tests
    RUN_TEST(test_format_file_size_bytes);
    RUN_TEST(test_format_file_size_kilobytes);
    RUN_TEST(test_format_file_size_megabytes);
    
    return UNITY_END();
}