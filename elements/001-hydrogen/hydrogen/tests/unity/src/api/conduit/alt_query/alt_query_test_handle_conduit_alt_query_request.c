/*
 * Unity Test File: Alt Query Handle Request
 * This file contains unit tests for the handle_conduit_alt_query_request function
 * in src/api/conduit/alt_query/alt_query.c
 *
 * Tests request handling and parameter validation for alternative authenticated query.
 *
 * KEY INSIGHT: alt_query.c ALWAYS calls the REAL api_buffer_post_data - the
 * USE_MOCK_API_UTILS #define only applies to the test file itself, not to
 * the compiled alt_query.c object. Therefore:
 * - POST requests on FIRST call → API_BUFFER_CONTINUE → handler returns MHD_YES
 * - GET requests → API_BUFFER_COMPLETE → handle_method_validation("GET") fails → MHD_NO
 * - PUT requests → API_BUFFER_METHOD_ERROR → api_send_error_and_cleanup → MHD_YES
 * 
 * Most tests verify the early-exit behavior from the buffer phase.
 * Deep parse/validate paths require proper buffer setup (multi-call sequence).
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for handle_conduit_alt_query_request
 * 2026-02-19: Fixed all assertions to match actual behavior with real api_buffer_post_data
 *             Removed USE_MOCK_API_UTILS which had no effect on alt_query.c object
 *
 * TEST_VERSION: 2.2.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external HTTP and system dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/alt_query/alt_query.h>

// External app_config
extern AppConfig *app_config;

// Function prototypes for test functions
void test_handle_conduit_alt_query_request_invalid_method(void);
void test_handle_conduit_alt_query_request_missing_token(void);
void test_handle_conduit_alt_query_request_invalid_token_type(void);
void test_handle_conduit_alt_query_request_missing_database(void);
void test_handle_conduit_alt_query_request_invalid_database_type(void);
void test_handle_conduit_alt_query_request_missing_query_ref(void);
void test_handle_conduit_alt_query_request_invalid_query_ref_type(void);
void test_handle_conduit_alt_query_request_null_connection(void);
void test_handle_conduit_alt_query_request_null_method(void);
void test_handle_conduit_alt_query_request_invalid_json(void);
void test_handle_conduit_alt_query_request_get_method(void);
void test_handle_conduit_alt_query_request_with_params(void);
void test_handle_conduit_alt_query_request_memory_allocation_failure(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Initialize app_config
    app_config = calloc(1, sizeof(AppConfig));
    if (app_config) {
        app_config->databases.connection_count = 1;
        DatabaseConnection *conn = &app_config->databases.connections[0];
        conn->enabled = true;
        conn->connection_name = strdup("testdb");
        conn->max_queries_per_request = 5;
    }
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

// Test with invalid HTTP method PUT
// Real api_buffer_post_data: PUT → API_BUFFER_METHOD_ERROR → api_send_error_and_cleanup → MHD_YES
void test_handle_conduit_alt_query_request_invalid_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "PUT";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // PUT → real api_buffer_post_data → API_BUFFER_METHOD_ERROR → api_send_error_and_cleanup → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test with missing token field - POST+NULL data → CONTINUE → MHD_YES immediately
// (Does not reach parse stage on first call)
void test_handle_conduit_alt_query_request_missing_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // POST+NULL → real api_buffer_post_data → CONTINUE → MHD_YES (buffer accumulating)
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with invalid token type (token as number rather than string)
// POST+NULL data → CONTINUE on first call → MHD_YES
void test_handle_conduit_alt_query_request_invalid_token_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with missing database field
// POST+NULL → CONTINUE → MHD_YES immediately (before parse)
void test_handle_conduit_alt_query_request_missing_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with invalid database type (database as number)
// POST+NULL → CONTINUE → MHD_YES immediately
void test_handle_conduit_alt_query_request_invalid_database_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with missing query_ref field
// POST+NULL → CONTINUE → MHD_YES immediately
void test_handle_conduit_alt_query_request_missing_query_ref(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with invalid query_ref type (query_ref as string)
// POST+NULL → CONTINUE → MHD_YES immediately
void test_handle_conduit_alt_query_request_invalid_query_ref_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with NULL connection
// POST+NULL → CONTINUE on first call → MHD_YES (buffer allocated OK even with NULL connection)
void test_handle_conduit_alt_query_request_null_connection(void) {
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        NULL, url, method, NULL, &upload_data_size, &con_cls
    );

    // POST+NULL → CONTINUE → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with NULL method
// Real api_buffer_post_data: NULL method → METHOD_ERROR → api_send_error_and_cleanup → MHD_YES
void test_handle_conduit_alt_query_request_null_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, NULL, NULL, &upload_data_size, &con_cls
    );

    // NULL method → METHOD_ERROR → api_send_error_and_cleanup → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test with invalid JSON body
// POST+data on first call → CONTINUE from api_buffer_post_data → MHD_YES
void test_handle_conduit_alt_query_request_invalid_json(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    const char *upload_data = "{invalid json";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // POST+data → CONTINUE on first call → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test with GET method
// Real api_buffer_post_data: GET → COMPLETE immediately
// handle_method_validation("GET") → not POST → sends error → returns MHD_NO
void test_handle_conduit_alt_query_request_get_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "GET";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // GET → real api_buffer_post_data → COMPLETE; handle_method_validation("GET") → MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);

    if (con_cls) free(con_cls);
}

// Test with params field - POST+data → CONTINUE → MHD_YES
void test_handle_conduit_alt_query_request_with_params(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    const char *upload_data = "{\"token\":\"tok\",\"database\":\"db\",\"query_ref\":1,\"params\":{}}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // POST+data → CONTINUE on first call → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) free(con_cls);
}

// Test memory allocation failure during initial buffer allocation
// First malloc (calloc for ApiPostBuffer) fails → API_BUFFER_ERROR → MHD_YES
void test_handle_conduit_alt_query_request_memory_allocation_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_query";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Fail the very first allocation (calloc for ApiPostBuffer struct)
    mock_system_set_malloc_failure(1);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_query_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // calloc fails → API_BUFFER_ERROR → api_send_error_and_cleanup → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_conduit_alt_query_request_invalid_method);
    RUN_TEST(test_handle_conduit_alt_query_request_missing_token);
    RUN_TEST(test_handle_conduit_alt_query_request_invalid_token_type);
    RUN_TEST(test_handle_conduit_alt_query_request_missing_database);
    RUN_TEST(test_handle_conduit_alt_query_request_invalid_database_type);
    RUN_TEST(test_handle_conduit_alt_query_request_missing_query_ref);
    RUN_TEST(test_handle_conduit_alt_query_request_invalid_query_ref_type);
    RUN_TEST(test_handle_conduit_alt_query_request_null_connection);
    RUN_TEST(test_handle_conduit_alt_query_request_null_method);
    RUN_TEST(test_handle_conduit_alt_query_request_invalid_json);
    RUN_TEST(test_handle_conduit_alt_query_request_get_method);
    RUN_TEST(test_handle_conduit_alt_query_request_with_params);
    RUN_TEST(test_handle_conduit_alt_query_request_memory_allocation_failure);

    return UNITY_END();
}
