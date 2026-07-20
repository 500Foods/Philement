/*
 * Unity Test File: auth_stream SSE helpers
 * Covers auth_stream_send_sse_response_headers, auth_stream_send_sse_error_response,
 * and auth_stream_chat_response in src/api/wschat/auth_stream/auth_stream.c
 *
 * CHANGELOG:
 * 2026-07-19: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_stream/auth_stream.h>

// Mock includes (USE_MOCK_LIBMICROHTTPD is global — do not redefine)
#include <unity/mocks/mock_libmicrohttpd.h>

// Test function prototypes
void test_auth_stream_add_sse_headers_null(void);
void test_auth_stream_send_sse_response_headers_success(void);
void test_auth_stream_send_sse_response_headers_create_fails(void);
void test_auth_stream_send_sse_error_response_basic(void);
void test_auth_stream_chat_response_stub(void);
void test_auth_stream_chat_response_create_fails(void);

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_auth_stream_add_sse_headers_null(void) {
    // Must not crash on NULL response
    auth_stream_add_sse_headers(NULL);
    TEST_PASS();
}

void test_auth_stream_send_sse_response_headers_success(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;

    enum MHD_Result ret = auth_stream_send_sse_response_headers(conn);

    TEST_ASSERT_EQUAL(MHD_YES, ret);
}

void test_auth_stream_send_sse_response_headers_create_fails(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;

    mock_mhd_set_create_response_should_fail(true);

    enum MHD_Result ret = auth_stream_send_sse_response_headers(conn);

    TEST_ASSERT_EQUAL(MHD_NO, ret);
}

void test_auth_stream_send_sse_error_response_basic(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;

    enum MHD_Result ret = auth_stream_send_sse_error_response(
        conn, "Database not found", MHD_HTTP_BAD_REQUEST);

    TEST_ASSERT_EQUAL(MHD_YES, ret);
}

void test_auth_stream_chat_response_stub(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;

    enum MHD_Result ret = auth_stream_chat_response(conn, NULL, "{}", "test_db");

    TEST_ASSERT_EQUAL(MHD_YES, ret);
}

void test_auth_stream_chat_response_create_fails(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;

    mock_mhd_set_create_response_should_fail(true);

    enum MHD_Result ret = auth_stream_chat_response(conn, NULL, "{}", "test_db");

    TEST_ASSERT_EQUAL(MHD_NO, ret);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_stream_add_sse_headers_null);
    RUN_TEST(test_auth_stream_send_sse_response_headers_success);
    RUN_TEST(test_auth_stream_send_sse_response_headers_create_fails);
    RUN_TEST(test_auth_stream_send_sse_error_response_basic);
    RUN_TEST(test_auth_stream_chat_response_stub);
    RUN_TEST(test_auth_stream_chat_response_create_fails);

    return UNITY_END();
}
