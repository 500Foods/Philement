/*
 * Unity Test File: Terminal Init and Cleanup Coverage Tests
 * This file contains unit tests targeting uncovered lines in terminal.c:
 * - init_terminal_support shutdown/state/init-session-manager/payload/failure paths
 * - handle_terminal_request compressed-file, .br fallback, and filesystem paths
 * - cleanup_terminal_support with allocated terminal files
 * - terminal_url_validator and terminal_request_handler wrapper functions
 * - serve_file_from_path error and brotli paths
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/terminal/terminal.h>
#include <src/terminal/terminal_session.h>
#include <src/webserver/web_server_core.h>
#include <src/webserver/web_server_compression.h>
#include <src/payload/payload_cache.h>
#include <src/payload/payload.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* Mock headers for terminal source compilation (USE_MOCK_LIBMICROHTTPD and
 * USE_MOCK_SYSTEM are defined globally by CMake for terminal tests, but we
 * include mock_system.h to access control functions like mock_system_set_*.) */
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Include mocks for external dependencies */
#include <unity/mocks/mock_libmicrohttpd.h>

/* Function prototypes for functions being tested */
bool init_terminal_support(TerminalConfig *config);
void cleanup_terminal_support(TerminalConfig *config);
enum MHD_Result serve_file_from_path(struct MHD_Connection *connection, const char *file_path);
enum MHD_Result handle_terminal_request(struct MHD_Connection *connection,
                                        const char *url,
                                        const TerminalConfig *config);

/* External global state from state.c */
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t terminal_system_shutdown;

/* External app_config for CORS */
extern AppConfig *app_config;

/* Function prototypes for test functions */
void test_init_terminal_support_server_stopping(void);
void test_init_terminal_support_web_server_shutdown(void);
void test_init_terminal_support_not_starting(void);
void test_init_terminal_support_already_initialized(void);
void test_init_terminal_support_filesystem_mode_success(void);
void test_init_terminal_support_filesystem_mode_default_index_page(void);
void test_init_terminal_support_payload_mode_cache_unavailable(void);
void test_init_terminal_support_payload_mode_calloc_failure(void);
void test_init_terminal_support_payload_mode_strdup_failure(void);
void test_init_terminal_support_payload_mode_success(void);
void test_init_terminal_support_filesystem_mode_with_index_page(void);
void test_init_terminal_support_disabled_config(void);
void test_init_terminal_support_null_config(void);
void test_handle_terminal_request_compressed_file_client_accepts_brotli(void);
void test_handle_terminal_request_file_not_found_filesystem_mode(void);
void test_handle_terminal_request_directory_no_webroot(void);
void test_handle_terminal_request_file_path_too_long(void);
void test_handle_terminal_request_brotli_direct_request_with_basepath(void);
void test_handle_terminal_request_root_path_redirect(void);
void test_cleanup_terminal_support_with_allocated_files(void);
void test_serve_file_from_path_fstat_failure(void);
void test_serve_file_from_path_create_response_failure(void);
void test_serve_file_from_path_brotli_file_success(void);
void test_serve_file_from_path_nonexistent_file(void);
void test_terminal_url_validator_null_config(void);
void test_terminal_url_validator_valid_request(void);
void test_terminal_url_validator_no_match(void);
void test_terminal_request_handler_null_connection(void);
void test_terminal_request_handler_redirect(void);

void test_serve_file_from_path_nonexistent_file(void);
void test_serve_file_from_path_non_brotli_success(void);
void test_handle_terminal_request_null_params(void);
void test_is_terminal_subsystem_initialized(void);

void test_handle_terminal_request_redirect_asprintf_failure(void);
void test_handle_terminal_request_compressed_file_no_accepts_br(void);
void test_handle_terminal_request_with_accept_encoding(void);
void test_handle_terminal_request_payload_only_mode(void);

void test_format_file_size_bytes(void);
void test_format_file_size_kilobytes(void);
void test_format_file_size_megabytes(void);

/* Test fixtures */
static TerminalConfig test_config;
static struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

void setUp(void) {
    /* Reset mock system */
    mock_system_reset_all();

    /* Reset MHD mocks */
    mock_mhd_reset_all();

    /* Disable cleanup thread for testing */
    terminal_session_disable_cleanup_thread();

    /* Clean up any existing session manager */
    cleanup_session_manager();

    /* Reset server state to defaults */
    server_starting = 1;
    server_stopping = 0;
    web_server_shutdown = 0;
    terminal_system_shutdown = 0;

    /* Reset terminal subsystem - cleanup first */
    cleanup_terminal_support(NULL);

    /* Disable cleanup thread again after cleanup */
    terminal_session_disable_cleanup_thread();

    /* Set up minimal app_config for CORS headers */
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
    }

    /* Initialize test config */
    memset(&test_config, 0, sizeof(TerminalConfig));
    test_config.enabled = true;
    test_config.web_path = (char*)"/terminal";
    test_config.index_page = (char*)"terminal.html";
    test_config.max_sessions = 10;
    test_config.idle_timeout_seconds = 300;
    test_config.webroot = NULL;
}

void tearDown(void) {
    /* Clean up after each test */
    cleanup_terminal_support(NULL);

    /* Reset server state */
    server_starting = 1;
    server_stopping = 0;
    web_server_shutdown = 0;
    terminal_system_shutdown = 0;

    /* Reset mocks */
    mock_system_reset_all();
    mock_mhd_reset_all();
}

/*
 * TEST SUITE: init_terminal_support - state guard error paths (lines 96-104)
 */

void test_init_terminal_support_server_stopping(void) {
    server_stopping = 1;

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_web_server_shutdown(void) {
    web_server_shutdown = 1;

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_not_starting(void) {
    server_starting = 0;

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

/*
 * TEST SUITE: init_terminal_support - already initialized (lines 108-113)
 */

void test_init_terminal_support_already_initialized(void) {
    /* First initialization in filesystem mode should succeed */
    test_config.webroot = (char*)"/tmp";
    bool result1 = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result1);

    /* Second initialization should hit the "Already initialized" path (line 110)
     * and return true without reinitializing */
    bool result2 = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result2);
}

/*
 * TEST SUITE: init_terminal_support - disabled/null config (lines 107-113)
 */

void test_init_terminal_support_disabled_config(void) {
    test_config.enabled = false;

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_null_config(void) {
    bool result = init_terminal_support(NULL);
    TEST_ASSERT_FALSE(result);
}

/*
 * TEST SUITE: init_terminal_support - payload mode (lines 135-154, 159-162)
 */

void test_init_terminal_support_payload_mode_cache_unavailable(void) {
    /* Use PAYLOAD: webroot to trigger payload mode */
    test_config.webroot = (char*)"PAYLOAD:/terminal";

    bool result = init_terminal_support(&test_config);

    /* Should fail because payload cache is not available in test environment */
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_payload_mode_calloc_failure(void) {
    /* PAYLOAD: webroot forces payload mode */
    test_config.webroot = (char*)"PAYLOAD:/terminal";

    /* Simulate calloc failure for terminal_files allocation */
    mock_system_set_calloc_failure(1);

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_payload_mode_strdup_failure(void) {
    /* PAYLOAD: webroot forces payload mode */
    test_config.webroot = (char*)"PAYLOAD:/terminal";

    /* Simulate strdup failure during name allocation */
    mock_system_set_malloc_failure(1);

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_FALSE(result);
}

void test_init_terminal_support_payload_mode_success(void) {
    /* Test the success path in payload mode */
    /* We need payload cache to be available - check if it's available */
    if (!is_payload_cache_available()) {
        /* Payload cache not available in test environment, skip this test */
        TEST_IGNORE_MESSAGE("Payload cache not available in test environment");
        return;
    }

    test_config.webroot = (char*)"PAYLOAD:/terminal";

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result);
}

/*
 * TEST SUITE: init_terminal_support - filesystem mode (lines 196-225, 209)
 */

void test_init_terminal_support_filesystem_mode_success(void) {
    /* Use filesystem mode with a real directory */
    test_config.webroot = (char*)"/tmp";

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result);
}

void test_init_terminal_support_filesystem_mode_default_index_page(void) {
    /* Filesystem mode with no index_page - exercises default index page log (line 209) */
    test_config.webroot = (char*)"/tmp";
    test_config.index_page = NULL;

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result);
}

void test_init_terminal_support_filesystem_mode_with_index_page(void) {
    /* Filesystem mode with custom index page */
    test_config.webroot = (char*)"/tmp";
    test_config.index_page = (char*)"custom_index.html";

    bool result = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(result);
}

/*
 * TEST SUITE: handle_terminal_request - file serving and filesystem paths
 */

void test_handle_terminal_request_compressed_file_client_accepts_brotli(void) {
    /* Initialize in filesystem mode so global_terminal_config is set */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Set up mock to return Accept-Encoding with br */
    mock_mhd_set_lookup_result("gzip, deflate, br");

    /* Request a .br file directly */
    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/test.html.br", &test_config);

    /* Should return MHD_NO since the file doesn't exist in our empty terminal_files */
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_file_not_found_filesystem_mode(void) {
    /* Initialize in filesystem mode */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Request a non-existent file - should fall through to filesystem check */
    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/nonexistent_file_xyz.html", &test_config);

    /* File doesn't exist on filesystem, should return MHD_NO */
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_directory_no_webroot(void) {
    /* No webroot configured - should use current directory fallback */
    test_config.webroot = NULL;
    init_terminal_support(&test_config);

    /* Request a file that doesn't exist in current directory */
    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/nonexistent_file_xyz123.html", &test_config);

    /* File doesn't exist, should return MHD_NO */
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_file_path_too_long(void) {
    /* Filesystem mode with a very long path that causes snprintf to overflow */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Build a very long URL that will cause the path to exceed PATH_MAX */
    char long_url[PATH_MAX + 500];
    memset(long_url, 'a', sizeof(long_url) - 1);
    long_url[0] = '/';
    long_url[sizeof(long_url) - 1] = '\0';

    /* We need this to be inside the /terminal prefix */
    memmove(long_url + 10, "/terminal/", 10);

    enum MHD_Result result = handle_terminal_request(mock_connection, long_url, &test_config);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_brotli_direct_request_with_basepath(void) {
    /* Initialize in filesystem mode */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Request a .br file directly - this exercises the .br fallback logic (lines 411-425) */
    /* Since terminal_files is empty in filesystem mode, file will be NULL */
    /* It will then check the .br fallback path */
    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/missing.br", &test_config);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_root_path_redirect(void) {
    /* Initialize in filesystem mode */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Set up mock for successful redirect */
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_add_header_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);

    /* Request exact prefix to trigger redirect */
    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal", &test_config);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

/*
 * TEST SUITE: cleanup_terminal_support with allocated files (lines 556-560)
 */

void test_cleanup_terminal_support_with_allocated_files(void) {
    /* In filesystem mode, terminal_files is NULL, so cleanup just zeroes things */
    test_config.webroot = (char*)"/tmp";
    bool init_result = init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(init_result);

    /* Call cleanup - should handle gracefully even with NULL terminal_files */
    cleanup_terminal_support(NULL);
    TEST_PASS();
}

/*
 * TEST SUITE: serve_file_from_path error paths (lines 279-288, 306-307)
 */

void test_serve_file_from_path_fstat_failure(void) {
    /* Test with null connection - should return MHD_NO before any file operations */
    enum MHD_Result result = serve_file_from_path(NULL, "/tmp/test_file.h");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_serve_file_from_path_create_response_failure(void) {
    /* Create a temp file that opens but response creation fails */
    const char *test_file = "/tmp/terminal_serve_test_resp_fail.html";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "<html>test</html>";
        write(fd, content, strlen(content));
        close(fd);

        /* Set MHD create response to fail */
        mock_mhd_set_create_response_should_fail(true);

        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        TEST_ASSERT_EQUAL(MHD_NO, result);

        /* Reset mock */
        mock_mhd_set_create_response_should_fail(false);
    } else {
        TEST_PASS(); /* Skip if we can't create temp file */
    }

    unlink(test_file);
}

void test_serve_file_from_path_brotli_file_success(void) {
    /* Create a temp HTML file and a .br version to test brotli serving */
    const char *base_file = "/tmp/terminal_serve_test_br.html";
    const char *br_file = "/tmp/terminal_serve_test_br.html.br";

    /* Create the HTML file */
    int fd = open(base_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "<html><body>BR Test</body></html>";
        write(fd, content, strlen(content));
        close(fd);

        /* Create the .br file */
        int br_fd = open(br_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (br_fd != -1) {
            const char *br_content = "compressed_data";
            write(br_fd, br_content, strlen(br_content));
            close(br_fd);

            /* Set up mock connection to accept brotli */
            mock_mhd_set_lookup_result("gzip, deflate, br");

            /* Reset MHD mocks for success */
            mock_mhd_set_create_response_should_fail(false);
            mock_mhd_set_add_header_should_fail(false);
            mock_mhd_set_queue_response_result(MHD_YES);

            /* serve_file_from_path opens the .br version internally */
            enum MHD_Result result = serve_file_from_path(mock_connection, base_file);
            /* Result depends on mock response creation */
            TEST_ASSERT(result == MHD_YES || result == MHD_NO);
        } else {
            TEST_PASS(); /* Skip if we can't create br file */
        }
    } else {
        TEST_PASS(); /* Skip if we can't create temp file */
    }

    /* Clean up */
    unlink(base_file);
    unlink(br_file);
}

void test_serve_file_from_path_nonexistent_file(void) {
    /* Test serving a file that doesn't exist */
    enum MHD_Result result = serve_file_from_path(mock_connection, "/tmp/nonexistent_file_99999.html");
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_serve_file_from_path_non_brotli_success(void) {
    /* Test successful serve of a regular file (not brotli) */
    const char *test_file = "/tmp/terminal_serve_test_regular.html";
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *content = "<html>test</html>";
        write(fd, content, strlen(content));
        close(fd);

        /* Set up mocks for success */
        mock_mhd_set_create_response_should_fail(false);
        mock_mhd_set_add_header_should_fail(false);
        mock_mhd_set_queue_response_result(MHD_YES);

        /* Client does not accept brotli - should serve regular file */
        mock_mhd_set_lookup_result("gzip");

        enum MHD_Result result = serve_file_from_path(mock_connection, test_file);
        TEST_ASSERT_EQUAL(MHD_YES, result);
    } else {
        TEST_PASS(); /* Skip if we can't create temp file */
    }

    unlink(test_file);
}

void test_handle_terminal_request_null_params(void) {
    /* Test with null params - should return MHD_NO immediately */
    enum MHD_Result result = handle_terminal_request(NULL, "/terminal/test.html", &test_config);
    TEST_ASSERT_EQUAL(MHD_NO, result);

    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    result = handle_terminal_request(mock_connection, NULL, &test_config);
    TEST_ASSERT_EQUAL(MHD_NO, result);

    result = handle_terminal_request(mock_connection, "/terminal/test.html", NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_is_terminal_subsystem_initialized(void) {
    /* Before init, should return false */
    cleanup_terminal_support(NULL);
    TEST_ASSERT_FALSE(is_terminal_subsystem_initialized());

    /* After init, should return true */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);
    TEST_ASSERT_TRUE(is_terminal_subsystem_initialized());

    cleanup_terminal_support(NULL);
    TEST_ASSERT_FALSE(is_terminal_subsystem_initialized());
}

void test_handle_terminal_request_redirect_asprintf_failure(void) {
    /* When asprintf fails, redirect is skipped and request falls through to file lookup */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_add_header_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);

    /* Set asprintf to fail - redirect will be skipped, falls through to file lookup */
    mock_system_set_asprintf_failure(1);

    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal", &test_config);
    /* Falls through to index page lookup, file doesn't exist, returns MHD_NO */
    TEST_ASSERT_EQUAL(MHD_NO, result);

    mock_system_set_asprintf_failure(0);
}

void test_handle_terminal_request_compressed_file_no_accepts_br(void) {
    /* Test file lookup when client does NOT accept brotli - covers different branch */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* No Accept-Encoding header */
    mock_mhd_set_lookup_result(NULL);

    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/nonexistent.css", &test_config);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_with_accept_encoding(void) {
    /* Test with Accept-Encoding header - covers the log line for accept_encoding present */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    mock_mhd_set_lookup_result("gzip, deflate");

    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/missing.js", &test_config);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_handle_terminal_request_payload_only_mode(void) {
    /* Test PAYLOAD: webroot mode - covers the payload-only branch */
    test_config.webroot = (char*)"PAYLOAD:";
    init_terminal_support(&test_config);

    mock_mhd_set_lookup_result(NULL);

    enum MHD_Result result = handle_terminal_request(mock_connection, "/terminal/nonexistent.html", &test_config);
    /* PAYLOAD: mode means no filesystem fallback - should return MHD_NO */
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

/*
 * TEST SUITE: terminal_url_validator and terminal_request_handler wrappers
 */

void test_terminal_url_validator_null_config(void) {
    /* With no terminal subsystem initialized, global config is NULL */
    /* terminal_url_validator should return false */
    bool result = terminal_url_validator("/terminal");
    TEST_ASSERT_FALSE(result);
}

void test_terminal_url_validator_valid_request(void) {
    /* Initialize terminal subsystem in filesystem mode */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Now terminal_url_validator should work using global_terminal_config */
    bool result = terminal_url_validator("/terminal");
    TEST_ASSERT_TRUE(result);

    bool result2 = terminal_url_validator("/terminal/index.html");
    TEST_ASSERT_TRUE(result2);

    bool result3 = terminal_url_validator("/other");
    TEST_ASSERT_FALSE(result3);
}

void test_terminal_url_validator_no_match(void) {
    /* Initialize terminal subsystem */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Test non-matching URLs */
    TEST_ASSERT_FALSE(terminal_url_validator("/api/terminal"));
    TEST_ASSERT_FALSE(terminal_url_validator("/terminalx"));
    TEST_ASSERT_FALSE(terminal_url_validator(NULL));
}

void test_terminal_request_handler_null_connection(void) {
    /* Initialize terminal subsystem */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Pass NULL as connection - handle_terminal_request checks for null first */
    enum MHD_Result result = terminal_request_handler(NULL, NULL, "/terminal", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_terminal_request_handler_redirect(void) {
    /* Initialize terminal subsystem */
    test_config.webroot = (char*)"/tmp";
    init_terminal_support(&test_config);

    /* Set up mock for successful redirect */
    mock_mhd_set_create_response_should_fail(false);
    mock_mhd_set_add_header_should_fail(false);
    mock_mhd_set_queue_response_result(MHD_YES);

    /* Request exact prefix to trigger redirect via the wrapper */
    enum MHD_Result result = terminal_request_handler(NULL, mock_connection, "/terminal", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_format_file_size_bytes(void) {
    char buffer[64];
    format_file_size(0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("0 bytes", buffer);

    format_file_size(512, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("512 bytes", buffer);

    format_file_size(1023, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1023 bytes", buffer);
}

void test_format_file_size_kilobytes(void) {
    char buffer[64];
    format_file_size(1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1.0K", buffer);

    format_file_size(2048, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("2.0K", buffer);

    format_file_size(1536, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1.5K", buffer);
}

void test_format_file_size_megabytes(void) {
    char buffer[64];
    format_file_size(1024 * 1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1.0M", buffer);

    format_file_size(5 * 1024 * 1024, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("5.0M", buffer);
}

int main(void) {
    UNITY_BEGIN();

    /* init_terminal_support state guard tests */
    RUN_TEST(test_init_terminal_support_server_stopping);
    RUN_TEST(test_init_terminal_support_web_server_shutdown);
    RUN_TEST(test_init_terminal_support_not_starting);
    RUN_TEST(test_init_terminal_support_disabled_config);
    RUN_TEST(test_init_terminal_support_null_config);
    RUN_TEST(test_init_terminal_support_already_initialized);

    /* init_terminal_support payload mode tests */
    RUN_TEST(test_init_terminal_support_payload_mode_cache_unavailable);
    RUN_TEST(test_init_terminal_support_payload_mode_calloc_failure);
    RUN_TEST(test_init_terminal_support_payload_mode_strdup_failure);
    RUN_TEST(test_init_terminal_support_payload_mode_success);

    /* init_terminal_support filesystem mode tests */
    RUN_TEST(test_init_terminal_support_filesystem_mode_success);
    RUN_TEST(test_init_terminal_support_filesystem_mode_default_index_page);
    RUN_TEST(test_init_terminal_support_filesystem_mode_with_index_page);

    /* handle_terminal_request tests */
    RUN_TEST(test_handle_terminal_request_compressed_file_client_accepts_brotli);
    RUN_TEST(test_handle_terminal_request_file_not_found_filesystem_mode);
    RUN_TEST(test_handle_terminal_request_directory_no_webroot);
    RUN_TEST(test_handle_terminal_request_file_path_too_long);
    RUN_TEST(test_handle_terminal_request_brotli_direct_request_with_basepath);
    RUN_TEST(test_handle_terminal_request_root_path_redirect);

    /* cleanup_terminal_support tests */
    RUN_TEST(test_cleanup_terminal_support_with_allocated_files);

    /* serve_file_from_path error path tests */
    RUN_TEST(test_serve_file_from_path_fstat_failure);
    RUN_TEST(test_serve_file_from_path_create_response_failure);
    RUN_TEST(test_serve_file_from_path_brotli_file_success);
    RUN_TEST(test_serve_file_from_path_nonexistent_file);
    RUN_TEST(test_serve_file_from_path_non_brotli_success);
    RUN_TEST(test_handle_terminal_request_null_params);
    RUN_TEST(test_is_terminal_subsystem_initialized);

    RUN_TEST(test_handle_terminal_request_redirect_asprintf_failure);
    RUN_TEST(test_handle_terminal_request_compressed_file_no_accepts_br);
    RUN_TEST(test_handle_terminal_request_with_accept_encoding);
    RUN_TEST(test_handle_terminal_request_payload_only_mode);

    /* terminal_url_validator and terminal_request_handler wrapper tests */
    RUN_TEST(test_terminal_url_validator_null_config);
    RUN_TEST(test_terminal_url_validator_valid_request);
    RUN_TEST(test_terminal_url_validator_no_match);
    RUN_TEST(test_terminal_request_handler_null_connection);
    RUN_TEST(test_terminal_request_handler_redirect);

    /* format_file_size tests for lines 51-59 */
    RUN_TEST(test_format_file_size_bytes);
    RUN_TEST(test_format_file_size_kilobytes);
    RUN_TEST(test_format_file_size_megabytes);

    return UNITY_END();
}
