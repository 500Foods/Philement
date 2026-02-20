/*
 * Mock API utilities functions for unit testing
 */

#include "mock_api_utils.h"
#include <stdlib.h>
#include <string.h>

// Static state for mocks
static ApiBufferResult mock_buffer_result = API_BUFFER_COMPLETE;
static enum MHD_Result mock_send_error_result = MHD_YES;
static const char *mock_buffer_data = NULL;

// Static buffer for mock use - allocated once and reused
static ApiPostBuffer mock_buffer_storage = {0};

// Mock implementations
ApiBufferResult mock_api_buffer_post_data(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out
) {
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    if (mock_buffer_result == API_BUFFER_COMPLETE && buffer_out) {
        // Set up a minimal valid buffer for testing
        mock_buffer_storage.magic = 0xB0FFAB1E;  // API_POST_BUFFER_MAGIC
        if (mock_buffer_data) {
            mock_buffer_storage.data = (char *)mock_buffer_data;
            mock_buffer_storage.size = strlen(mock_buffer_data);
        } else {
            mock_buffer_storage.data = NULL;
            mock_buffer_storage.size = 0;
        }
        mock_buffer_storage.capacity = mock_buffer_storage.size;
        mock_buffer_storage.http_method = (method && method[0] == 'G') ? 'G' : 'P';
        *buffer_out = &mock_buffer_storage;
    }

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
    mock_buffer_data = NULL;
    mock_buffer_storage.data = NULL;
    mock_buffer_storage.size = 0;
    mock_buffer_storage.capacity = 0;
    mock_buffer_storage.http_method = 'P';
}

void mock_api_utils_set_buffer_result(ApiBufferResult result) {
    mock_buffer_result = result;
}

void mock_api_utils_set_send_error_result(enum MHD_Result result) {
    mock_send_error_result = result;
}

void mock_api_utils_set_buffer_data(const char *data) {
    mock_buffer_data = data;
}