/*
 * Unity Test File: Web Server Request - Request Completed Test
 * This file contains unit tests for request_completed() function
 * Updated version that follows better testing patterns from websocket examples
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>
#include <src/webserver/web_server_core.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>

// Mock external variables
__attribute__((weak)) ServiceThreads webserver_threads = {0};

// Mock functions
__attribute__((weak))
enum MHD_Result MHD_destroy_post_processor(struct MHD_PostProcessor *pp) {
    (void)pp; // Mock - do nothing
    return MHD_YES;
}

__attribute__((weak))
int fclose(FILE *stream) {
    (void)stream; // Mock - do nothing
    return 0;
}

__attribute__((weak))
void free(void *ptr) {
    (void)ptr; // Mock - do nothing
}

__attribute__((weak))
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    (void)threads; (void)thread_id; // Mock - do nothing
}

__attribute__((weak))
pthread_t pthread_self(void) {
    return (pthread_t)12345; // Mock thread ID
}

void setUp(void) {
    // Reset all mocks to default state
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Reset global state
    memset(&webserver_threads, 0, sizeof(ServiceThreads));
}

void tearDown(void) {
    // Clean up test fixtures, if any
    mock_mhd_reset_all();
    mock_system_reset_all();
}

static void test_request_completed_null_parameters(void) {
    // Test with null parameters - should handle gracefully without crashing

    // Test with NULL cls parameter
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;
    void *con_cls = NULL;

    // This should not crash - the function ignores cls and connection parameters
    request_completed(NULL, mock_connection, &con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should still be NULL
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_null_con_cls(void) {
    // Test with null con_cls pointer - should handle gracefully without crashing

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;
    void *null_con_cls = NULL;

    // This should not crash - the function checks if *con_cls is NULL before processing
    request_completed(NULL, mock_connection, &null_con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should still be NULL
    TEST_ASSERT_NULL(null_con_cls);
}

static void test_request_completed_null_con_info_in_con_cls(void) {
    // Test with null connection info in con_cls - should handle gracefully

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;
    struct ConnectionInfo *null_con_info = NULL;
    void *con_cls = &null_con_info;

    // This should not crash - the function checks if *con_cls is NULL
    request_completed(NULL, mock_connection, (void **)con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should still point to NULL
    TEST_ASSERT_NULL(*(void **)con_cls);
}

static void test_request_completed_with_valid_con_info(void) {
    // Test with valid connection info - should clean up resources properly

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Create a ConnectionInfo structure with some resources
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);

    // Set up some resources to be cleaned up
    con_info->postprocessor = (struct MHD_PostProcessor *)0xDEADBEEF; // Mock postprocessor
    con_info->fp = (FILE *)0xCAFEBABE; // Mock file pointer
    con_info->original_filename = strdup("test_original.txt");
    con_info->new_filename = strdup("test_new.txt");

    void *con_cls = con_info;

    // Call the function - should clean up resources
    request_completed(NULL, mock_connection, (void **)&con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should be set to NULL after cleanup
    TEST_ASSERT_NULL(con_cls);

    // Note: We can't verify that free was called on the strings since our mock doesn't track that,
    // but the function should have attempted to free them
}

static void test_request_completed_cleanup_postprocessor(void) {
    // Test cleanup of postprocessor - should call MHD_destroy_post_processor

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Create ConnectionInfo with postprocessor
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->postprocessor = (struct MHD_PostProcessor *)0xDEADBEEF;

    void *con_cls = con_info;

    // Call the function - should destroy postprocessor
    request_completed(NULL, mock_connection, (void **)&con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should be set to NULL
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_cleanup_file_pointer(void) {
    // Test cleanup of file pointer - should call fclose

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Create ConnectionInfo with file pointer
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->fp = (FILE *)0xCAFEBABE;

    void *con_cls = con_info;

    // Call the function - should close file pointer
    request_completed(NULL, mock_connection, (void **)&con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should be set to NULL
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_cleanup_filenames(void) {
    // Test cleanup of filename strings - should free strings

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Create ConnectionInfo with filenames
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->original_filename = strdup("original.txt");
    con_info->new_filename = strdup("new.txt");

    void *con_cls = con_info;

    // Call the function - should free filename strings
    request_completed(NULL, mock_connection, (void **)&con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should be set to NULL
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_thread_cleanup(void) {
    // Test thread removal from tracking - should call remove_service_thread

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Create ConnectionInfo
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);

    void *con_cls = con_info;

    // Call the function - should remove thread from tracking
    request_completed(NULL, mock_connection, (void **)&con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);

    // con_cls should be set to NULL
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_multiple_calls_safe(void) {
    // Test that multiple calls are safe - should handle NULL con_cls gracefully

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;
    void *con_cls = NULL;

    // First call - should handle NULL gracefully
    request_completed(NULL, mock_connection, &con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);
    TEST_ASSERT_NULL(con_cls);

    // Second call - should still handle NULL gracefully
    request_completed(NULL, mock_connection, &con_cls, MHD_REQUEST_TERMINATED_COMPLETED_OK);
    TEST_ASSERT_NULL(con_cls);
}

static void test_request_completed_termination_codes(void) {
    // Test different termination codes - should handle all codes

    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x12345678;

    // Test with different termination codes
    enum MHD_RequestTerminationCode codes[] = {
        MHD_REQUEST_TERMINATED_COMPLETED_OK,
        MHD_REQUEST_TERMINATED_WITH_ERROR,
        MHD_REQUEST_TERMINATED_TIMEOUT_REACHED,
        MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN
    };

    for (size_t i = 0; i < sizeof(codes)/sizeof(codes[0]); i++) {
        // Create ConnectionInfo for each test
        struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
        TEST_ASSERT_NOT_NULL(con_info);
        con_info->original_filename = strdup("test.txt");

        void *con_cls = con_info;

        // Call with different termination codes
        request_completed(NULL, mock_connection, (void **)&con_cls, codes[i]);

        // con_cls should be set to NULL regardless of termination code
        TEST_ASSERT_NULL(con_cls);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_request_completed_null_parameters);
    RUN_TEST(test_request_completed_null_con_cls);
    RUN_TEST(test_request_completed_null_con_info_in_con_cls);
    RUN_TEST(test_request_completed_with_valid_con_info);
    RUN_TEST(test_request_completed_cleanup_postprocessor);
    RUN_TEST(test_request_completed_cleanup_file_pointer);
    RUN_TEST(test_request_completed_cleanup_filenames);
    RUN_TEST(test_request_completed_thread_cleanup);
    RUN_TEST(test_request_completed_multiple_calls_safe);
    RUN_TEST(test_request_completed_termination_codes);

    return UNITY_END();
}
