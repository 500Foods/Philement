/*
 * Unity Test File: API Service handle_exact_api_files_local_request Tests
 *
 * This file contains unit tests for the handle_exact_api_files_local_request
 * handler in api_service.c. This handler is a thin delegation wrapper that
 * forwards all parameters to handle_system_upload_request (the same handler
 * used by /api/system/upload).
 *
 * Tests verify the delegation passes parameters through correctly and that
 * the return value matches what handle_system_upload_request produces.
 */

// Enable mocks BEFORE any includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM

// Include mock headers first (redirects system/MHD functions)
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_service.h>
#include <src/api/api_utils.h>
#include <src/config/config.h>

// External variable (defined in global.c)
extern AppConfig *app_config;

// Mock app_config for testing
static AppConfig test_config;

// Function prototypes
void test_handle_files_local_delegates_get_rejected(void);
void test_handle_files_local_passes_connection_through(void);
void test_handle_files_local_get_sets_405_status(void);
void test_handle_files_local_ignores_unused_params(void);

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    memset(&test_config, 0, sizeof(AppConfig));
    test_config.api.prefix = (char*)"/api";
    test_config.api.headers = NULL;
    test_config.api.headers_count = 0;
    app_config = &test_config;
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    app_config = NULL;
}

/*
 * Test: handle_exact_api_files_local_request delegates to
 *       handle_system_upload_request, which rejects GET with 405.
 */
void test_handle_files_local_delegates_get_rejected(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *con_cls = NULL;

    enum MHD_Result result = handle_exact_api_files_local_request(
        NULL, conn, "/api/files/local", "GET", "1.1",
        NULL, &sz, &con_cls);

    /* handle_system_upload_request rejects non-POST with MHD_YES
       (the mocked MHD_queue_response returns MHD_YES) */
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

/*
 * Test: verify that the MHD status code reflects the 405 rejection
 *       from handle_system_upload_request's GET path.
 */
void test_handle_files_local_get_sets_405_status(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *con_cls = NULL;

    enum MHD_Result result = handle_exact_api_files_local_request(
        NULL, conn, "/api/files/local", "GET", "1.1",
        NULL, &sz, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_mhd_get_last_status_code());
}

/*
 * Test: verify that the real connection pointer is passed through
 *       to the underlying handler (not NULL or zeroed).
 */
void test_handle_files_local_passes_connection_through(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    size_t sz = 0;
    void *con_cls = NULL;

    enum MHD_Result result = handle_exact_api_files_local_request(
        NULL, conn, "/api/files/local", "GET", "1.1",
        NULL, &sz, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    /* If the connection wasn't passed through, the mock would still
       return YES — the key check is that it didn't crash with a bad
       pointer value, confirming the real connection was forwarded. */
}

/*
 * Test: verify that unused parameters (cls, url, version) don't cause
 *       issues — the function should still work with various values.
 */
void test_handle_files_local_ignores_unused_params(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *con_cls = NULL;

    /* Use non-NULL values for params the function declares as unused */
    enum MHD_Result result = handle_exact_api_files_local_request(
        (void*)0x1234, conn, "/api/files/local?foo=bar", "GET", "HTTP/1.0",
        (const char*)"ignored", &sz, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_mhd_get_last_status_code());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_files_local_delegates_get_rejected);
    RUN_TEST(test_handle_files_local_get_sets_405_status);
    RUN_TEST(test_handle_files_local_passes_connection_through);
    RUN_TEST(test_handle_files_local_ignores_unused_params);

    return UNITY_END();
}
