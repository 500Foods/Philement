/*
 * Mock API utilities functions for unit testing
 *
 * This file provides mock implementations of api_utils functions
 * to enable unit testing of code that depends on api_utils without requiring
 * the actual api_utils library during testing.
 */

#ifndef MOCK_API_UTILS_H
#define MOCK_API_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <microhttpd.h>
#include <jansson.h>

// Include api_utils.h for type definitions only when mocking is enabled
#ifdef USE_MOCK_API_UTILS
#include <src/api/api_utils.h>
#else
// Define types locally when not mocking
typedef struct {
    uint32_t magic;
    char *data;
    size_t size;
    size_t capacity;
    char http_method;
} ApiPostBuffer;

typedef enum {
    API_BUFFER_CONTINUE,
    API_BUFFER_COMPLETE,
    API_BUFFER_ERROR,
    API_BUFFER_METHOD_ERROR
} ApiBufferResult;
#endif

// Mock function declarations
#ifdef USE_MOCK_API_UTILS
#define api_buffer_post_data mock_api_buffer_post_data
#define api_free_post_buffer mock_api_free_post_buffer
#define api_send_error_and_cleanup mock_api_send_error_and_cleanup
#endif

// Mock implementations
ApiBufferResult mock_api_buffer_post_data(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out
);

void mock_api_free_post_buffer(void **con_cls);

enum MHD_Result mock_api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
);


// Mock control functions
void mock_api_utils_reset_all(void);
void mock_api_utils_set_buffer_result(ApiBufferResult result);
void mock_api_utils_set_send_error_result(enum MHD_Result result);

#endif /* MOCK_API_UTILS_H */