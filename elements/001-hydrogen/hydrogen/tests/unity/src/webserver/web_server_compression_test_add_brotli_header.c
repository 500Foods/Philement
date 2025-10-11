/*
 * Unity Test File: Web Server Compression - Add Brotli Header Test
 * This file contains unit tests for add_brotli_header() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_compression.h>

// Mock MHD response structure
struct MockMHDResponse {
    char content_encoding[256];
    char vary_header[256];
    int header_count;
};

// Global mock state
static struct MockMHDResponse mock_response = {0};

// Mock MHD function
// cppcheck-suppress[constParameterPointer] Mock function must match real library signature
enum MHD_Result MHD_add_response_header(struct MHD_Response *response,
                                      const char *header, const char *content) {
    if (!response || !header || !content) {
        return 0; // MHD_NO
    }

    if (strcmp(header, "Content-Encoding") == 0) {
        strncpy(mock_response.content_encoding, content, sizeof(mock_response.content_encoding) - 1);
    } else if (strcmp(header, "Vary") == 0) {
        strncpy(mock_response.vary_header, content, sizeof(mock_response.vary_header) - 1);
    }

    mock_response.header_count++;
    return 1; // MHD_YES
}

void setUp(void) {
    // Reset mock state
    memset(&mock_response, 0, sizeof(mock_response));
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_add_brotli_header_null_response(void) {
    // Test null response parameter
    add_brotli_header(NULL);
    // Should not crash, no headers should be added
    TEST_ASSERT_EQUAL(0, mock_response.header_count);
}

static void test_add_brotli_header_valid_response(void) {
    // Test with valid response
    add_brotli_header((struct MHD_Response*)&mock_response);

    // Should have added two headers
    TEST_ASSERT_EQUAL(2, mock_response.header_count);
    TEST_ASSERT_EQUAL_STRING("br", mock_response.content_encoding);
    TEST_ASSERT_EQUAL_STRING("Accept-Encoding", mock_response.vary_header);
}

static void test_add_brotli_header_multiple_calls(void) {
    // Test multiple calls to ensure headers are added each time
    add_brotli_header((struct MHD_Response*)&mock_response);
    add_brotli_header((struct MHD_Response*)&mock_response);

    // Should have added four headers total
    TEST_ASSERT_EQUAL(4, mock_response.header_count);
    // Headers should be overwritten (last call wins)
    TEST_ASSERT_EQUAL_STRING("br", mock_response.content_encoding);
    TEST_ASSERT_EQUAL_STRING("Accept-Encoding", mock_response.vary_header);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_add_brotli_header_null_response);
    RUN_TEST(test_add_brotli_header_valid_response);
    RUN_TEST(test_add_brotli_header_multiple_calls);

    return UNITY_END();
}