/*
 * Unity Test File: Web Server Request - Query String Preservation
 *
 * Tests that directory redirect preserves the query string, and that
 * web_server_get_query_string() correctly reconstructs query parameters
 * from MHD GET argument values.
 */

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_logging.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>
#include <src/webserver/web_server_core.h>
#include <src/config/config_defaults.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

// Mock structures for testing
struct MockMHDConnection {
    int dummy;
};

// External declarations for global state variables
extern ServiceThreads webserver_threads;
extern AppConfig *app_config;

// Local test config
static AppConfig *test_app_config = NULL;

// Temporary web root for directory redirect tests
static char test_web_root[512];

void setUp(void) {
    // Reset all mocks to default state
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();

    // Initialize webserver_threads
    memset(&webserver_threads, 0, sizeof(ServiceThreads));
    strncpy(webserver_threads.subsystem, "WebServer", sizeof(webserver_threads.subsystem) - 1);

    // Allocate and initialize app_config with proper defaults
    if (!test_app_config) {
        test_app_config = (AppConfig *)calloc(1, sizeof(AppConfig));
        if (test_app_config) {
            bool success = initialize_config_defaults(test_app_config);
            if (success) {
                app_config = test_app_config;
            }
        }
    } else {
        // Reinitialize existing config
        memset(test_app_config, 0, sizeof(AppConfig));
        initialize_config_defaults(test_app_config);
        app_config = test_app_config;
    }
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();

    // Clean up temp directory path (memory freed by config cleanup in tearDown)
    test_web_root[0] = '\0';
}

// Test: web_server_get_query_string returns NULL when no query params
static void test_get_query_string_empty(void) {
    struct MockMHDConnection mock_conn = {0};

    mock_mhd_clear_values();

    char *qs = web_server_get_query_string((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NULL(qs);
}

// Test: web_server_get_query_string returns NULL for NULL connection
static void test_get_query_string_null_connection(void) {
    char *qs = web_server_get_query_string(NULL);
    TEST_ASSERT_NULL(qs);
}

// Test: web_server_get_query_string with a single query parameter
static void test_get_query_string_single_param(void) {
    struct MockMHDConnection mock_conn = {0};

    mock_mhd_clear_values();
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "code", "ABC123");

    char *qs = web_server_get_query_string((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(qs);
    TEST_ASSERT_EQUAL_STRING("code=ABC123", qs);
    free(qs);
}

// Test: web_server_get_query_string with multiple query parameters
static void test_get_query_string_multiple_params(void) {
    struct MockMHDConnection mock_conn = {0};

    mock_mhd_clear_values();
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "code", "ABC123");
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "state", "xyz");

    char *qs = web_server_get_query_string((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(qs);
    // Order depends on mock iteration order
    TEST_ASSERT_TRUE(strstr(qs, "code=ABC123") != NULL);
    TEST_ASSERT_TRUE(strstr(qs, "state=xyz") != NULL);
    TEST_ASSERT_TRUE(strstr(qs, "&") != NULL);
    free(qs);
}

// Test: web_server_get_query_string URL-encodes special characters in values
static void test_get_query_string_encodes_special_chars(void) {
    struct MockMHDConnection mock_conn = {0};

    mock_mhd_clear_values();
    // MHD gives us URL-decoded values; we re-encode them
    // api_url_encode converts space to '+' and '&' to %26
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "code", "hello world&test");

    char *qs = web_server_get_query_string((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(qs);
    TEST_ASSERT_TRUE(strstr(qs, "code=") != NULL);
    // Space -> '+', '&' -> '%26' (matching api_url_encode behavior)
    TEST_ASSERT_TRUE(strstr(qs, "code=hello+world%26test") != NULL);
    free(qs);
}

// Test: Directory redirect preserves query string for SPA share links
static void test_handle_request_directory_redirect_preserves_query(void) {
    struct MockMHDConnection mock_conn = {0};
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up a temp directory as web_root so stat() finds a directory
    strncpy(test_web_root, "/tmp/h2o_qsredir_XXXXXX", sizeof(test_web_root));
    if (mkdtemp(test_web_root) == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temp directory for web root");
        return;
    }

    // Set web_root in config
    free(app_config->webserver.web_root);
    app_config->webserver.web_root = strdup(test_web_root);

    // The URL path will be checked against web_root + url
    // We need a subdirectory that exists in the temp dir
    mkdir(test_web_root, 0755);

    // Create a subdirectory to redirect on
    char subdir_path[600];
    snprintf(subdir_path, sizeof(subdir_path), "%s/subdir", test_web_root);
    mkdir(subdir_path, 0755);

    // Mock a query parameter
    mock_mhd_clear_values();
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "code", "abc123");

    mock_mhd_set_queue_response_result(MHD_YES);

    // Request without trailing slash -> should redirect to /subdir/?code=abc123
    enum MHD_Result result = handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                           "/subdir", "GET", "HTTP/1.1", NULL,
                                           &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_MOVED_PERMANENTLY, mock_mhd_get_last_status_code());

    // Verify the Location header was set with the query string
    TEST_ASSERT_TRUE(mock_mhd_header_was_added("Location", "/subdir/?code=abc123"));
}

// Test: Directory redirect without query string still works
static void test_handle_request_directory_redirect_no_query(void) {
    struct MockMHDConnection mock_conn = {0};
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up temp directory
    strncpy(test_web_root, "/tmp/h2o_qsredir2_XXXXXX", sizeof(test_web_root));
    if (mkdtemp(test_web_root) == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temp directory for web root");
        return;
    }

    free(app_config->webserver.web_root);
    app_config->webserver.web_root = strdup(test_web_root);

    // Create a subdirectory
    char subdir_path[600];
    snprintf(subdir_path, sizeof(subdir_path), "%s/subdir", test_web_root);
    mkdir(subdir_path, 0755);

    // No query parameters
    mock_mhd_clear_values();

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                           "/subdir", "GET", "HTTP/1.1", NULL,
                                           &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_MOVED_PERMANENTLY, mock_mhd_get_last_status_code());

    // Verify the Location header is just the path with trailing slash
    TEST_ASSERT_TRUE(mock_mhd_header_was_added("Location", "/subdir/"));
}

// Test: Directory redirect with query parameter containing special chars
static void test_handle_request_directory_redirect_query_special_chars(void) {
    struct MockMHDConnection mock_conn = {0};
    void *con_cls = NULL;
    size_t upload_size = 0;

    strncpy(test_web_root, "/tmp/h2o_qsredir3_XXXXXX", sizeof(test_web_root));
    if (mkdtemp(test_web_root) == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temp directory for web root");
        return;
    }

    free(app_config->webserver.web_root);
    app_config->webserver.web_root = strdup(test_web_root);

    char subdir_path[600];
    snprintf(subdir_path, sizeof(subdir_path), "%s/lua", test_web_root);
    mkdir(subdir_path, 0755);

    // Query param with special characters that need encoding
    mock_mhd_clear_values();
    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "code", "hello world&test");

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                           "/lua", "GET", "HTTP/1.1", NULL,
                                           &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_MOVED_PERMANENTLY, mock_mhd_get_last_status_code());

    // The redirect URL should have the query string with encoded value
    // api_url_encode converts space to '+' and '&' to %26
    TEST_ASSERT_TRUE(mock_mhd_header_was_added("Location",
        "/lua/?code=hello+world%26test"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_query_string_empty);
    RUN_TEST(test_get_query_string_null_connection);
    RUN_TEST(test_get_query_string_single_param);
    RUN_TEST(test_get_query_string_multiple_params);
    RUN_TEST(test_get_query_string_encodes_special_chars);
    RUN_TEST(test_handle_request_directory_redirect_preserves_query);
    RUN_TEST(test_handle_request_directory_redirect_no_query);
    RUN_TEST(test_handle_request_directory_redirect_query_special_chars);

    // Clean up test config
    if (test_app_config) {
        free(test_app_config);
        test_app_config = NULL;
        app_config = NULL;
    }

    return UNITY_END();
}
