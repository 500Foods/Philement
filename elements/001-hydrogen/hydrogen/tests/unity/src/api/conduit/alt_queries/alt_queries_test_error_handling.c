/*
 * Unity Test File: Alt Queries Error Handling
 * This file contains unit tests for error handling paths in alt_queries.c
 *
 * Tests error scenarios for alternative authenticated queries endpoint.
 * 
 * KEY INSIGHT: alt_queries.c always calls the REAL api_buffer_post_data (not mock)
 * because the macro redefinition in USE_MOCK_API_UTILS only applies to the test
 * file itself, not to the compiled alt_queries.c object. Therefore:
 * - POST+NULL data → real api_buffer_post_data → API_BUFFER_CONTINUE → MHD_YES
 * - PUT method → real api_buffer_post_data → API_BUFFER_METHOD_ERROR → MHD_YES
 * - GET method → real api_buffer_post_data → API_BUFFER_COMPLETE → method val. failure → MHD_NO
 *
 * Tests cover early exit paths (CONTINUE, METHOD_ERROR) and via GET: COMPLETE path.
 * Deep parse/validate paths are tested in alt_queries_test_parse_alt_queries_request.c
 * and alt_queries_test_validate_jwt_for_auth_alt.c files.
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation of unit tests for error handling
 *
 * TEST_VERSION: 1.0.0
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
#include <src/api/conduit/alt_queries/alt_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_alt_queries_post_null_data_returns_yes(void);
void test_alt_queries_put_method_returns_yes(void);
void test_alt_queries_null_method_returns_yes(void);
void test_alt_queries_get_method_returns_no(void);
void test_alt_queries_post_with_data_returns_yes(void);
void test_alt_queries_buffer_error_via_malloc_failure(void);

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
void test_alt_queries_post_null_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: POST+NULL first call → allocates buffer → API_BUFFER_CONTINUE
    // handle_alt_queries_buffer_result(CONTINUE) → MHD_YES; buf_result!=COMPLETE → returns MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup the allocated buffer
    if (con_cls) {
        free(((char*)con_cls)); // api_buffer_post_data allocates data within buffer
    }
}

// Test: PUT method - real api_buffer_post_data returns METHOD_ERROR
// Handler calls api_send_error_and_cleanup which returns MHD_YES (via mocked MHD_queue_response)
void test_alt_queries_put_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "PUT";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: PUT → METHOD_ERROR; handle_alt_query_buffer_result sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: NULL method - real api_buffer_post_data returns METHOD_ERROR (null is not GET/POST/OPTIONS)
void test_alt_queries_null_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, NULL, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: NULL method → METHOD_ERROR; handler sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: GET method - real api_buffer_post_data returns COMPLETE
// Then handle_method_validation("GET") → not POST → returns MHD_NO
void test_alt_queries_get_method_returns_no(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "GET";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // REAL api_buffer_post_data: GET → COMPLETE immediately
    // handle_method_validation("GET") → not POST → returns MHD_NO → handler returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test: POST with data upload - first call returns CONTINUE
// All subsequent buffer interactions return MHD_YES
void test_alt_queries_post_with_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\":\"t\",\"database\":\"d\",\"queries\":[{\"query_ref\":1}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
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
// handle_alt_queries_buffer_result(ERROR) → api_send_error_and_cleanup → MHD_YES
void test_alt_queries_buffer_error_via_malloc_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Fail the SECOND malloc call: first calloc for buffer struct succeeds,
    // second malloc for buffer->data fails → returns API_BUFFER_ERROR
    mock_system_set_malloc_failure(2);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // API_BUFFER_ERROR → handle_alt_queries_buffer_result → api_send_error_and_cleanup → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alt_queries_post_null_data_returns_yes);
    RUN_TEST(test_alt_queries_put_method_returns_yes);
    RUN_TEST(test_alt_queries_null_method_returns_yes);
    RUN_TEST(test_alt_queries_get_method_returns_no);
    RUN_TEST(test_alt_queries_post_with_data_returns_yes);
    RUN_TEST(test_alt_queries_buffer_error_via_malloc_failure);

    return UNITY_END();
}
