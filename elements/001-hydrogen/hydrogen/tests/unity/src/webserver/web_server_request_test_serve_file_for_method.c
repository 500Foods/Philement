/*
 * Unity Test File: Web Server Request - Serve File For Method Test
 * This file contains unit tests for serve_file_for_method() function
 *
 * This function extends serve_file() to support HEAD requests, which should
 * return headers but no body content.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>
#include <src/webserver/web_server_core.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// External global needed for the function
extern WebServerConfig *server_web_config;

// Test fixture variables
static char temp_dir[] = "/tmp/webserver_test_XXXXXX";
static char test_file_path[256];

void setUp(void) {
    // Reset all mocks
    mock_mhd_reset_all();
    
    // Create temp directory
    if (mkdtemp(temp_dir) == NULL) {
        strcpy(temp_dir, "/tmp");
    }
    
    // Initialize test file path
    snprintf(test_file_path, sizeof(test_file_path), "%s/test.html", temp_dir);
    
    // Create a test file with known content
    FILE *fp = fopen(test_file_path, "w");
    if (fp) {
        fprintf(fp, "<html><body>Test</body></html>");
        fclose(fp);
    }
    
    // Initialize mock web config
    static WebServerConfig mock_config = {0};
    static char web_root[256];
    static char upload_path[256];
    static char upload_dir[256];
    snprintf(web_root, sizeof(web_root), "%s", temp_dir);
    snprintf(upload_path, sizeof(upload_path), "/upload");
    snprintf(upload_dir, sizeof(upload_dir), "/tmp/upload");
    mock_config.web_root = web_root;
    mock_config.upload_path = upload_path;
    mock_config.upload_dir = upload_dir;
    server_web_config = &mock_config;
}

void tearDown(void) {
    // Clean up test file
    unlink(test_file_path);
    
    // Reset mocks
    mock_mhd_reset_all();
}

// Test NULL connection parameter
static void test_serve_file_for_method_null_connection(void) {
    // The function doesn't validate connection parameter - it passes it to MHD
    // With mocks enabled and file existing, this may succeed or fail depending on mock state
    // We just verify it doesn't crash
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    (void)serve_file_for_method(NULL, test_file_path, "GET");
    TEST_PASS(); // Function didn't crash with NULL connection
}

// Test NULL file path parameter
static void test_serve_file_for_method_null_file_path(void) {
    // Test null file path parameter - should return MHD_NO
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    enum MHD_Result result = serve_file_for_method(mock_conn, NULL, "GET");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test with nonexistent file
static void test_serve_file_for_method_nonexistent_file(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Test with a file that doesn't exist
    enum MHD_Result result = serve_file_for_method(mock_conn, "/nonexistent/path/file.html", "GET");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test successful GET request
static void test_serve_file_for_method_get_success(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test GET method with existing file
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "GET");
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test successful HEAD request
static void test_serve_file_for_method_head_success(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test HEAD method with existing file
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "HEAD");
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test GET with different file extensions
static void test_serve_file_for_method_extensions(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    char ext_file_path[256];
    
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test various file extensions
    const char *extensions[] = {".html", ".css", ".js", ".json", ".png", ".jpg", ".wasm", NULL};
    
    for (int i = 0; extensions[i] != NULL; i++) {
        snprintf(ext_file_path, sizeof(ext_file_path), "%s/test%s", temp_dir, extensions[i]);
        
        // Create the test file
        FILE *fp = fopen(ext_file_path, "w");
        if (fp) {
            fprintf(fp, "test content");
            fclose(fp);
        }
        
        // Test serving the file
        enum MHD_Result result = serve_file_for_method(mock_conn, ext_file_path, "GET");
        TEST_ASSERT_EQUAL(MHD_YES, result);
        
        // Clean up
        unlink(ext_file_path);
    }
}

// Test when MHD_create_response_from_fd fails
static void test_serve_file_for_method_response_create_fails(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Force MHD_create_response_from_fd to fail
    mock_mhd_set_create_response_should_fail(true);
    
    // Test with existing file but response creation fails
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "GET");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test when MHD_queue_response fails
static void test_serve_file_for_method_queue_response_fails(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow response creation but force queue to fail
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_NO);
    
    // Test with existing file but queue response fails
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "GET");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test NULL method parameter (should behave like GET)
static void test_serve_file_for_method_null_method(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // NULL method should be treated as GET
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test case sensitivity of method parameter
static void test_serve_file_for_method_case_sensitivity(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test that "head" (lowercase) is treated as GET, not HEAD
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "head");
    // Should succeed but treat as GET (not HEAD)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test backward compatibility with serve_file
static void test_serve_file_backward_compatibility(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // serve_file should call serve_file_for_method with "GET"
    enum MHD_Result result = serve_file(mock_conn, test_file_path);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test empty method string
static void test_serve_file_for_method_empty_method(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Empty method should be treated as GET
    enum MHD_Result result = serve_file_for_method(mock_conn, test_file_path, "");
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test file without extension
static void test_serve_file_for_method_no_extension(void) {
    struct MHD_Connection *mock_conn = (struct MHD_Connection*)0x12345678;
    char no_ext_path[256];
    snprintf(no_ext_path, sizeof(no_ext_path), "%s/noextension", temp_dir);
    
    // Create file without extension
    FILE *fp = fopen(no_ext_path, "w");
    if (fp) {
        fprintf(fp, "test content");
        fclose(fp);
    }
    
    // Allow MHD functions to succeed
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);
    
    // Test serving file without extension
    enum MHD_Result result = serve_file_for_method(mock_conn, no_ext_path, "GET");
    TEST_ASSERT_EQUAL(MHD_YES, result);
    
    // Clean up
    unlink(no_ext_path);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_serve_file_for_method_null_connection);
    RUN_TEST(test_serve_file_for_method_null_file_path);
    RUN_TEST(test_serve_file_for_method_nonexistent_file);
    RUN_TEST(test_serve_file_for_method_get_success);
    RUN_TEST(test_serve_file_for_method_head_success);
    RUN_TEST(test_serve_file_for_method_extensions);
    RUN_TEST(test_serve_file_for_method_response_create_fails);
    RUN_TEST(test_serve_file_for_method_queue_response_fails);
    RUN_TEST(test_serve_file_for_method_null_method);
    RUN_TEST(test_serve_file_for_method_case_sensitivity);
    RUN_TEST(test_serve_file_backward_compatibility);
    RUN_TEST(test_serve_file_for_method_empty_method);
    RUN_TEST(test_serve_file_for_method_no_extension);

    return UNITY_END();
}
