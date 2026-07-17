/*
 * Unity Test File: handle_cap_buffer_result
 * Tests the handle_cap_buffer_result function in
 * src/api/conduit/cap/cap_query.c. Exercises every switch case:
 * CONTINUE, ERROR, METHOD_ERROR, COMPLETE, and default.
 *
 * CHANGELOG:
 * 2026-07-16: Initial creation covering all buffer result branches.
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

// api_send_error_and_cleanup is already mocked globally (USE_MOCK_LIBMICROHTTPD)
#include <src/api/conduit/cap/cap_query.h>

// Function prototypes for test functions
void test_handle_cap_buffer_result_continue(void);
void test_handle_cap_buffer_result_error(void);
void test_handle_cap_buffer_result_method_error(void);
void test_handle_cap_buffer_result_complete(void);
void test_handle_cap_buffer_result_default(void);
void test_handle_cap_buffer_result_null_connection(void);
void test_handle_cap_buffer_result_null_con_cls(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_handle_cap_buffer_result_continue(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, API_BUFFER_CONTINUE, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_error(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, API_BUFFER_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_method_error(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, API_BUFFER_METHOD_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_complete(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, API_BUFFER_COMPLETE, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_default(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    void *con_cls = NULL;

    // Cast an invalid value to hit the default case
    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, (ApiBufferResult)999, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_null_connection(void) {
    void *con_cls = NULL;

    enum MHD_Result result = handle_cap_buffer_result(
        NULL, API_BUFFER_ERROR, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_cap_buffer_result_null_con_cls(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;

    enum MHD_Result result = handle_cap_buffer_result(
        mock_connection, API_BUFFER_ERROR, NULL
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_cap_buffer_result_continue);
    RUN_TEST(test_handle_cap_buffer_result_error);
    RUN_TEST(test_handle_cap_buffer_result_method_error);
    RUN_TEST(test_handle_cap_buffer_result_complete);
    RUN_TEST(test_handle_cap_buffer_result_default);
    RUN_TEST(test_handle_cap_buffer_result_null_connection);
    RUN_TEST(test_handle_cap_buffer_result_null_con_cls);

    return UNITY_END();
}
