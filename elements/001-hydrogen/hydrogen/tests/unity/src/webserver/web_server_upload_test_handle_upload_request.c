/*
 * Unity Test File: Web Server Upload - Handle Upload Request Test
 * This file contains unit tests for handle_upload_request() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_upload.h>
#include <src/webserver/web_server_core.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

// Forward declarations for test functions
void test_handle_upload_request_response_already_sent(void);
void test_handle_upload_request_upload_failed(void);
void test_handle_upload_request_no_file_uploaded(void);
void test_handle_upload_request_file_upload_completed(void);
void test_handle_upload_request_queue_response_failure(void);

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    system("rm -f /tmp/test_upload_*.gcode 2>/dev/null || true");
}

// Test: response already sent - should return MHD_YES without doing anything
void test_handle_upload_request_response_already_sent(void) {
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->response_sent = true;

    void *con_cls = con_info;
    size_t upload_data_size = 0;

    enum MHD_Result result = handle_upload_request(NULL, "", &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(con_info->response_sent);

    free(con_info);
}

// Test: upload failed due to size limit - should send error response
void test_handle_upload_request_upload_failed(void) {
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->upload_failed = true;
    con_info->error_code = MHD_HTTP_CONTENT_TOO_LARGE;
    con_info->response_sent = false;

    void *con_cls = con_info;
    size_t upload_data_size = 0;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_upload_request(NULL, "", &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(con_info->response_sent);

    free(con_info);
}

// Test: no file uploaded (fp is NULL) - should send error response
void test_handle_upload_request_no_file_uploaded(void) {
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->fp = NULL;
    con_info->upload_failed = false;
    con_info->response_sent = false;

    void *con_cls = con_info;
    size_t upload_data_size = 0;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_upload_request(NULL, "", &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(con_info->response_sent);

    free(con_info);
}

// Test: file upload completed - should process file and send response
void test_handle_upload_request_file_upload_completed(void) {
    // Create a temp G-code file (suffix length must match ".gcode")
    char temp_path[] = "/tmp/test_upload_XXXXXX.gcode";
    int fd = mkstemps(temp_path, 6);
    TEST_ASSERT(fd != -1);

    const char *gcode = "G21 ; metric values\nG90 ; absolute positioning\nG28 ; home all axes\n";
    write(fd, gcode, strlen(gcode));
    close(fd);

    // Open the file for reading (as the upload handler would have it open)
    FILE *fp = fopen(temp_path, "rb");
    TEST_ASSERT_NOT_NULL(fp);

    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->fp = fp;
    con_info->upload_failed = false;
    con_info->response_sent = false;
    con_info->original_filename = strdup("test_print.gcode");
    con_info->new_filename = strdup(temp_path);
    con_info->total_size = strlen(gcode);

    void *con_cls = con_info;
    size_t upload_data_size = 0;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_upload_request(NULL, "", &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(con_info->response_sent);

    // Clean up
    free(con_info->original_filename);
    free(con_info->new_filename);
    free(con_info);
    unlink(temp_path);
}

// Test: queue response failure - should return MHD_NO
void test_handle_upload_request_queue_response_failure(void) {
    struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
    TEST_ASSERT_NOT_NULL(con_info);
    con_info->fp = NULL;
    con_info->upload_failed = false;
    con_info->response_sent = false;

    void *con_cls = con_info;
    size_t upload_data_size = 0;

    mock_mhd_set_queue_response_result(MHD_NO);

    enum MHD_Result result = handle_upload_request(NULL, "", &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_TRUE(con_info->response_sent);

    free(con_info);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_upload_request_response_already_sent);
    RUN_TEST(test_handle_upload_request_upload_failed);
    RUN_TEST(test_handle_upload_request_no_file_uploaded);
    RUN_TEST(test_handle_upload_request_file_upload_completed);
    RUN_TEST(test_handle_upload_request_queue_response_failure);

    return UNITY_END();
}
