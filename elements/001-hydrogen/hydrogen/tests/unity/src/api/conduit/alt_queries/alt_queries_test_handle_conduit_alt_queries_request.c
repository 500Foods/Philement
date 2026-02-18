/*
 * Unity Test File: Alt Queries Handle Request
 * This file contains unit tests for the handle_conduit_alt_queries_request function
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * Tests request handling and parameter validation for alternative authenticated queries.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for handle_conduit_alt_queries_request
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/alt_queries/alt_queries.h>

// Function prototypes for test functions
void test_handle_conduit_alt_queries_request_invalid_method(void);
void test_handle_conduit_alt_queries_request_missing_token(void);
void test_handle_conduit_alt_queries_request_invalid_token_type(void);
void test_handle_conduit_alt_queries_request_missing_database(void);
void test_handle_conduit_alt_queries_request_invalid_database_type(void);
void test_handle_conduit_alt_queries_request_missing_queries(void);
void test_handle_conduit_alt_queries_request_invalid_queries_type(void);
void test_handle_conduit_alt_queries_request_empty_queries_array(void);
void test_handle_conduit_alt_queries_request_null_connection(void);
void test_handle_conduit_alt_queries_request_null_method(void);
void test_handle_conduit_alt_queries_request_invalid_json(void);
void test_handle_conduit_alt_queries_request_get_method(void);
void test_handle_conduit_alt_queries_request_memory_allocation_failure_token(void);
void test_handle_conduit_alt_queries_request_memory_allocation_failure_database(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_system_reset_all();
}

// Test handle_conduit_alt_queries_request with invalid HTTP method
void test_handle_conduit_alt_queries_request_invalid_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "PUT";  // Invalid method
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with missing token field
void test_handle_conduit_alt_queries_request_missing_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";  // Missing token
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with invalid token type (not a string)
void test_handle_conduit_alt_queries_request_invalid_token_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": 123, \"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";  // token is number
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with missing database field
void test_handle_conduit_alt_queries_request_missing_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"queries\": [{\"query_ref\": 123}]}";  // Missing database
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with invalid database type
void test_handle_conduit_alt_queries_request_invalid_database_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": 123, \"queries\": [{\"query_ref\": 123}]}";  // database is number
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with missing queries field
void test_handle_conduit_alt_queries_request_missing_queries(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\"}";  // Missing queries
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with invalid queries type
void test_handle_conduit_alt_queries_request_invalid_queries_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": \"not_an_array\"}";  // queries is string
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with empty queries array
void test_handle_conduit_alt_queries_request_empty_queries_array(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": []}";  // Empty array
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with NULL connection
void test_handle_conduit_alt_queries_request_null_connection(void) {
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        NULL, url, method, upload_data, &upload_data_size, &con_cls
    );

    // Function handles NULL connection gracefully by returning YES after sending error
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with NULL method
void test_handle_conduit_alt_queries_request_null_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, NULL, upload_data, &upload_data_size, &con_cls
    );

    // Function handles NULL method gracefully by returning YES after sending error
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with invalid JSON
void test_handle_conduit_alt_queries_request_invalid_json(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{invalid json";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with GET method
void test_handle_conduit_alt_queries_request_get_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test handle_conduit_alt_queries_request with memory allocation failure for token
void test_handle_conduit_alt_queries_request_memory_allocation_failure_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock strdup to fail on first call (for token allocation)
    mock_system_set_malloc_failure(2);  // Allow json parsing to succeed first

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // Should handle allocation failure gracefully
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test handle_conduit_alt_queries_request with memory allocation failure for database
void test_handle_conduit_alt_queries_request_memory_allocation_failure_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/alt_queries";
    const char *method = "POST";
    const char *upload_data = "{\"token\": \"jwt_token\", \"database\": \"testdb\", \"queries\": [{\"query_ref\": 123}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    // Mock strdup to fail on second call (for database allocation, after token succeeds)
    mock_system_set_malloc_failure(3);  // Allow token allocation to succeed

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_alt_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // Should handle allocation failure gracefully
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_conduit_alt_queries_request_invalid_method);
    RUN_TEST(test_handle_conduit_alt_queries_request_missing_token);
    RUN_TEST(test_handle_conduit_alt_queries_request_invalid_token_type);
    RUN_TEST(test_handle_conduit_alt_queries_request_missing_database);
    RUN_TEST(test_handle_conduit_alt_queries_request_invalid_database_type);
    RUN_TEST(test_handle_conduit_alt_queries_request_missing_queries);
    RUN_TEST(test_handle_conduit_alt_queries_request_invalid_queries_type);
    RUN_TEST(test_handle_conduit_alt_queries_request_empty_queries_array);
    RUN_TEST(test_handle_conduit_alt_queries_request_null_connection);
    RUN_TEST(test_handle_conduit_alt_queries_request_null_method);
    RUN_TEST(test_handle_conduit_alt_queries_request_invalid_json);
    RUN_TEST(test_handle_conduit_alt_queries_request_get_method);
    RUN_TEST(test_handle_conduit_alt_queries_request_memory_allocation_failure_token);
    RUN_TEST(test_handle_conduit_alt_queries_request_memory_allocation_failure_database);

    return UNITY_END();
}
