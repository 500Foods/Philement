/*
 * Unity Test File: handle_version_request Function Tests
 * This file contains unit tests for the handle_version_request() function
 * from src/api/system/version/version.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/system/version/version.h>

// Forward declaration for the function being tested
enum MHD_Result handle_version_request(struct MHD_Connection *connection);

// Mock structures for testing
struct MockMHDConnection {
    int dummy; // Minimal mock
};

// Function prototypes for test functions
void test_handle_version_request_normal_operation(void);
void test_handle_version_request_json_object_failure(void);
void test_handle_version_request_api_send_json_failure(void);

// Mock function prototypes
struct MHD_Response *MHD_create_response_from_buffer(size_t size, void *buffer, enum MHD_ResponseMemoryMode mode);
enum MHD_Result MHD_queue_response(struct MHD_Connection *connection, unsigned int status_code, struct MHD_Response *response);
enum MHD_Result MHD_add_response_header(struct MHD_Response *response, const char *header, const char *content);
void MHD_destroy_response(struct MHD_Response *response);

// Mock structures
struct MockMHDResponse {
    size_t size;
    void *data;
    int status_code;
};

// Global state for mocks
static struct MockMHDResponse *mock_response = NULL;
static int mock_mhd_queue_response_result = 1; // MHD_YES

// Mock function implementations
__attribute__((weak))
struct MHD_Response *MHD_create_response_from_buffer(size_t size, void *buffer, enum MHD_ResponseMemoryMode mode) {
    (void)mode;
    if (!mock_response) {
        mock_response = (struct MockMHDResponse *)malloc(sizeof(struct MockMHDResponse));
        if (!mock_response) return NULL;
        memset(mock_response, 0, sizeof(struct MockMHDResponse));
    }
    mock_response->size = size;
    mock_response->data = buffer;
    return (struct MHD_Response *)mock_response;
}

__attribute__((weak))
enum MHD_Result MHD_queue_response(struct MHD_Connection *connection, unsigned int status_code, struct MHD_Response *response) {
    (void)connection; (void)response;
    if (mock_response) {
        mock_response->status_code = (int)status_code;
    }
    return mock_mhd_queue_response_result;
}

__attribute__((weak))
enum MHD_Result MHD_add_response_header(struct MHD_Response *response, const char *header, const char *content) {
    (void)response; (void)header; (void)content;
    return 1; // MHD_YES
}

__attribute__((weak))
void MHD_destroy_response(struct MHD_Response *response) {
    (void)response;
    // Don't free mock_response as we reuse it
}

// Test fixtures
void setUp(void) {
    // Reset mock state
    mock_mhd_queue_response_result = 1; // MHD_YES

    // Clean up previous mock response
    if (mock_response) {
        free(mock_response);
        mock_response = NULL;
    }
}

void tearDown(void) {
    // Clean up mock objects
    if (mock_response) {
        free(mock_response);
        mock_response = NULL;
    }
}

void test_handle_version_request_normal_operation(void) {
    // Test normal operation with valid connection
    struct MockMHDConnection mock_conn = {0};

    enum MHD_Result result = handle_version_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(1, result); // Should return MHD_YES (1)
}


void test_handle_version_request_api_send_json_failure(void) {
    // Test error handling when MHD_queue_response() fails
    struct MockMHDConnection mock_conn = {0};
    mock_mhd_queue_response_result = 0; // MHD_NO

    enum MHD_Result result = handle_version_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(0, result); // Should return MHD_NO (0)
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_version_request_normal_operation);
    RUN_TEST(test_handle_version_request_api_send_json_failure);

    return UNITY_END();
}