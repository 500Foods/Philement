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

// Always include hydrogen.h first for type definitions, then api_utils.h
// for ApiPostBuffer and ApiBufferResult, so that both mock_api_utils.c
// (compiled without USE_MOCK_API_UTILS) and test files (compiled with
// USE_MOCK_API_UTILS) use the same enum types.
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Mock function declarations
#ifdef USE_MOCK_API_UTILS
#define api_buffer_post_data mock_api_buffer_post_data
#define api_free_post_buffer mock_api_free_post_buffer
#define api_send_error_and_cleanup mock_api_send_error_and_cleanup
#define api_send_json_response mock_api_send_json_response
#define api_parse_json_body mock_api_parse_json_body
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

enum MHD_Result mock_api_send_json_response(
    struct MHD_Connection *connection,
    json_t *json_obj,
    unsigned int status_code
);

json_t *mock_api_parse_json_body(ApiPostBuffer *buffer);


// Mock control functions
void mock_api_utils_reset_all(void);
void mock_api_utils_set_buffer_result(ApiBufferResult result);
void mock_api_utils_set_send_error_result(enum MHD_Result result);
void mock_api_utils_set_buffer_data(const char *data);

// Capture mode for mock_api_send_json_response:
// When enabled, the mock stores the json_obj pointer and status_code
// instead of freeing the json_obj, so tests can inspect the response.
// The caller is responsible for freeing the captured json_obj via
// mock_api_utils_reset_all() or mock_api_utils_reset_capture().
void mock_api_utils_set_capture_mode(bool capture);
json_t *mock_api_utils_get_captured_response(void);
unsigned int mock_api_utils_get_captured_status(void);
int mock_api_utils_get_send_json_response_call_count(void);
void mock_api_utils_reset_capture(void);

#endif /* MOCK_API_UTILS_H */