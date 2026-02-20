/*
 * Unity Test File: Alt Queries Parse Request
 * This file contains unit tests for the parse_alt_queries_request function
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for parse_alt_queries_request
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

// Include source headers
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/alt_queries/alt_queries.h>

// Function prototypes for test functions
void test_parse_alt_queries_request_null_method(void);
void test_parse_alt_queries_request_null_token_ptr(void);
void test_parse_alt_queries_request_null_database_ptr(void);
void test_parse_alt_queries_request_null_queries_ptr(void);
void test_parse_alt_queries_request_missing_token_field(void);
void test_parse_alt_queries_request_missing_database_field(void);
void test_parse_alt_queries_request_missing_queries_field(void);
void test_parse_alt_queries_request_empty_queries(void);
void test_parse_alt_queries_request_failed_to_copy_queries(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test parse_alt_queries_request with NULL method
void test_parse_alt_queries_request_null_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, NULL, &buffer, &con_cls, &token, &database, &queries_array
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with NULL token pointer
void test_parse_alt_queries_request_null_token_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, NULL, &database, &queries_array
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with NULL database pointer
void test_parse_alt_queries_request_null_database_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, NULL, &queries_array
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with NULL queries pointer
void test_parse_alt_queries_request_null_queries_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, NULL
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with missing token field
void test_parse_alt_queries_request_missing_token_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;
    buffer.data = strdup("{\"database\": \"test\", \"queries\": [{\"query_ref\": 1}]}");
    buffer.size = strlen(buffer.data);

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with missing database field
void test_parse_alt_queries_request_missing_database_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;
    buffer.data = strdup("{\"token\": \"jwt\", \"queries\": [{\"query_ref\": 1}]}");
    buffer.size = strlen(buffer.data);

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    // Token was allocated but we didn't reach the point of returning it
    free(token);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with missing queries field
void test_parse_alt_queries_request_missing_queries_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": \"test\"}");
    buffer.size = strlen(buffer.data);

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    free(token);
    free(database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_queries_request with empty queries array
void test_parse_alt_queries_request_empty_queries(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": \"test\", \"queries\": []}");
    buffer.size = strlen(buffer.data);

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    free(token);
    free(database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

void test_parse_alt_queries_request_failed_to_copy_queries(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    ApiPostBuffer buffer = {0};
    void *con_cls = NULL;
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": \"test\", \"queries\": [{\"query_ref\": 1}]}");
    buffer.size = strlen(buffer.data);

    // Mock memory allocation to fail when json_deep_copy tries to allocate
    mock_system_set_malloc_failure(10); // Fail on 10th allocation (adjust as needed)

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    free(token);
    free(database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_alt_queries_request_null_method);
    RUN_TEST(test_parse_alt_queries_request_null_token_ptr);
    RUN_TEST(test_parse_alt_queries_request_null_database_ptr);
    RUN_TEST(test_parse_alt_queries_request_null_queries_ptr);
    RUN_TEST(test_parse_alt_queries_request_missing_token_field);
    RUN_TEST(test_parse_alt_queries_request_missing_database_field);
    RUN_TEST(test_parse_alt_queries_request_missing_queries_field);
    RUN_TEST(test_parse_alt_queries_request_empty_queries);
    RUN_TEST(test_parse_alt_queries_request_failed_to_copy_queries);

    return UNITY_END();
}
