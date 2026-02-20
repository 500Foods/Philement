/*
 * Unity Test File: Alt Query Error Handling
 * This file contains unit tests for error handling paths in alt_query.c
 *
 * Tests error scenarios for alternative authenticated query endpoint.
 * 
 * KEY INSIGHT: alt_query.c always calls the REAL api_buffer_post_data (not mock)
 * because the macro redefinition in USE_MOCK_API_UTILS only applies to the test
 * file itself, not to the compiled alt_query.c object. Therefore:
 * - POST+NULL data → real api_buffer_post_data → API_BUFFER_CONTINUE → MHD_YES
 * - PUT method → real api_buffer_post_data → API_BUFFER_METHOD_ERROR → MHD_YES
 * - GET method → real api_buffer_post_data → API_BUFFER_COMPLETE → method val. failure → MHD_NO
 *
 * Tests cover early exit paths (CONTINUE, METHOD_ERROR) and via GET: COMPLETE path.
 * Deep parse/validate paths are tested in alt_query_test_parse_alt_request.c
 * and alt_query_test_validate_jwt_for_auth.c files.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for error handling
 * 2026-02-19: Fixed segfault - source bug with buf_result vs buffer_result check
 * 2026-02-19: Fixed assertions to match actual behavior (real api_buffer_post_data)
 *
 * TEST_VERSION: 1.3.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external HTTP and system dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include source headers
#include <src/api/conduit/alt_query/alt_query.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_alt_query_post_null_data_returns_yes(void);
void test_alt_query_put_method_returns_yes(void);
void test_alt_query_null_method_returns_yes(void);
void test_alt_query_get_method_returns_no(void);
void test_alt_query_post_with_data_returns_yes(void);
void test_alt_query_buffer_error_via_malloc_failure(void);
void test_alt_query_handle_alt_query_buffer_result_continue(void);
void test_alt_query_handle_alt_query_buffer_result_error(void);
void test_alt_query_handle_alt_query_buffer_result_method_error(void);
void test_alt_query_handle_alt_query_buffer_result_complete(void);
void test_alt_query_handle_alt_query_buffer_result_default(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    app_config->databases.connection_count = 1;
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = 5;
}

void tearDown(void) {
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            free(app_config->databases.connections[i].connection_name);
        }
        free(app_config);
        app_config = NULL;
    }

    mock_mhd_reset_all();
    mock_system_reset_all();
}

// Test: POST with NULL data - real api_buffer_post_data returns CONTINUE on first call
// Handler returns MHD_YES (CONTINUE path: has more data to receive)
void test_alt_query_post_null_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: POST+NULL first call → allocates buffer → API_BUFFER_CONTINUE
    // handle_alt_query_buffer_result(CONTINUE) → MHD_YES; buf_result!=COMPLETE → returns MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup the allocated buffer
    if (con_cls) {
        free(((char*)con_cls)); // api_buffer_post_data allocates data within buffer
    }
}

// Test: PUT method - real api_buffer_post_data returns METHOD_ERROR
// Handler calls api_send_error_and_cleanup which returns MHD_YES (via mocked MHD_queue_response)
void test_alt_query_put_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "PUT";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: PUT → METHOD_ERROR; handle_alt_query_buffer_result sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: NULL method - real api_buffer_post_data returns METHOD_ERROR (null is not GET/POST/OPTIONS)
void test_alt_query_null_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, NULL, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: NULL method → METHOD_ERROR; handler sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: GET method - real api_buffer_post_data returns COMPLETE
// Then handle_method_validation("GET") → not POST → returns MHD_NO
void test_alt_query_get_method_returns_no(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "GET";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: GET → COMPLETE immediately
    // handle_method_validation("GET") → not POST → returns MHD_NO → handler returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test: POST with data upload - first call returns CONTINUE
// All subsequent buffer interactions return MHD_YES
void test_alt_query_post_with_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    const char *upload_data = "{\"token\":\"t\",\"database\":\"d\",\"query_ref\":1}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: POST+data first call → allocates buffer, copies data → CONTINUE
    // buf_result==CONTINUE → returns MHD_YES (buffer still accumulating)
    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (con_cls) {
        free(con_cls);
    }
}

// Test: Trigger API_BUFFER_ERROR via malloc failure for data buffer
// When malloc fails for buffer->data, api_buffer_post_data returns API_BUFFER_ERROR
// handle_alt_query_buffer_result(ERROR) → api_send_error_and_cleanup → MHD_YES
void test_alt_query_buffer_error_via_malloc_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Fail the SECOND malloc call: first calloc for buffer struct succeeds,
    // second malloc for buffer->data fails → returns API_BUFFER_ERROR
    mock_system_set_malloc_failure(2);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // API_BUFFER_ERROR → handle_alt_query_buffer_result → api_send_error_and_cleanup → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Direct tests for handle_alt_query_buffer_result function
// These test the function directly to achieve coverage of all switch cases

// Test CONTINUE case of handle_alt_query_buffer_result
void test_alt_query_handle_alt_query_buffer_result_continue(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_alt_query_buffer_result(
        mock_connection, API_BUFFER_CONTINUE, &con_cls
    );

    // CONTINUE → returns MHD_YES directly
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test ERROR case of handle_alt_query_buffer_result
void test_alt_query_handle_alt_query_buffer_result_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_alt_query_buffer_result(
        mock_connection, API_BUFFER_ERROR, &con_cls
    );

    // ERROR → api_send_error_and_cleanup → api_send_json_response → MHD_YES (from mock)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test METHOD_ERROR case of handle_alt_query_buffer_result
void test_alt_query_handle_alt_query_buffer_result_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_alt_query_buffer_result(
        mock_connection, API_BUFFER_METHOD_ERROR, &con_cls
    );

    // METHOD_ERROR → api_send_error_and_cleanup → api_send_json_response → MHD_YES (from mock)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test COMPLETE case of handle_alt_query_buffer_result
void test_alt_query_handle_alt_query_buffer_result_complete(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    enum MHD_Result result = handle_alt_query_buffer_result(
        mock_connection, API_BUFFER_COMPLETE, &con_cls
    );

    // COMPLETE → returns MHD_YES directly (continue processing)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test default case of handle_alt_query_buffer_result (unknown buffer result value)
void test_alt_query_handle_alt_query_buffer_result_default(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    // Use an invalid/unknown enum value to hit the default case
    enum MHD_Result result = handle_alt_query_buffer_result(
        mock_connection, (ApiBufferResult)99, &con_cls
    );

    // default → api_send_error_and_cleanup → MHD_YES (from mock)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alt_query_post_null_data_returns_yes);
    RUN_TEST(test_alt_query_put_method_returns_yes);
    RUN_TEST(test_alt_query_null_method_returns_yes);
    RUN_TEST(test_alt_query_get_method_returns_no);
    RUN_TEST(test_alt_query_post_with_data_returns_yes);
    RUN_TEST(test_alt_query_buffer_error_via_malloc_failure);
    RUN_TEST(test_alt_query_handle_alt_query_buffer_result_continue);
    RUN_TEST(test_alt_query_handle_alt_query_buffer_result_error);
    RUN_TEST(test_alt_query_handle_alt_query_buffer_result_method_error);
    RUN_TEST(test_alt_query_handle_alt_query_buffer_result_complete);
    RUN_TEST(test_alt_query_handle_alt_query_buffer_result_default);

    return UNITY_END();
}
