/*
 * Mock API utilities functions for unit testing
 */

#include "mock_api_utils.h"

// Static state for mocks
static ApiBufferResult mock_buffer_result = API_BUFFER_COMPLETE;
static enum MHD_Result mock_send_error_result = MHD_YES;

// Mock implementations
ApiBufferResult mock_api_buffer_post_data(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out
) {
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    (void)buffer_out;

    return mock_buffer_result;
}

void mock_api_free_post_buffer(void **con_cls) {
    (void)con_cls;
    // Do nothing in mock
}

enum MHD_Result mock_api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
) {
    (void)connection;
    (void)con_cls;
    (void)error_message;
    (void)http_status;

    return mock_send_error_result;
}


// Mock control functions
void mock_api_utils_reset_all(void) {
    mock_buffer_result = API_BUFFER_COMPLETE;
    mock_send_error_result = MHD_YES;
}

void mock_api_utils_set_buffer_result(ApiBufferResult result) {
    mock_buffer_result = result;
}

void mock_api_utils_set_send_error_result(enum MHD_Result result) {
    mock_send_error_result = result;
}