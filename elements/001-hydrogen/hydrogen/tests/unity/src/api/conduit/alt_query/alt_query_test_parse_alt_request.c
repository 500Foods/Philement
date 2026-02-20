/*
 * Unity Test File: Alt Query Parse Alt Request
 * This file contains unit tests for the parse_alt_request function
 * in src/api/conduit/alt_query/alt_query.c
 *
 * Tests request parsing for alternative authenticated query.
 *
 * IMPORTANT: Buffer must use http_method = 'P' for POST mode so that
 * handle_request_parsing_with_buffer parses JSON from buffer->data.
 * If http_method = 0 (zeroed), it uses GET mode and looks at MHD params
 * (all NULL from mock), which causes premature failure at "missing token".
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for parse_alt_request
 * 2026-02-19: Fixed buffer.http_method = 'P' for POST mode
 *             Added tests for all error paths and valid cases
 *
 * TEST_VERSION: 1.1.0
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
#include <src/api/conduit/alt_query/alt_query.h>

// Forward declaration of the function we're testing
enum MHD_Result parse_alt_request(
    struct MHD_Connection *connection,
    const char *method,
    ApiPostBuffer *buffer,
    void **con_cls,
    char **token,
    char **database,
    int *query_ref,
    json_t **params_json);

// Function prototypes for test functions
void test_parse_alt_request_null_method(void);
void test_parse_alt_request_null_token_ptr(void);
void test_parse_alt_request_null_database_ptr(void);
void test_parse_alt_request_null_query_ref_ptr(void);
void test_parse_alt_request_null_params_ptr(void);
void test_parse_alt_request_parse_failure_empty_buffer(void);
void test_parse_alt_request_missing_token_field(void);
void test_parse_alt_request_missing_database_field(void);
void test_parse_alt_request_missing_query_ref_field(void);
void test_parse_alt_request_invalid_token_type(void);
void test_parse_alt_request_invalid_database_type(void);
void test_parse_alt_request_invalid_query_ref_type(void);
void test_parse_alt_request_valid_no_params(void);
void test_parse_alt_request_valid_with_params(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

// Test parse_alt_request with NULL method - returns MHD_NO immediately (null check)
void test_parse_alt_request_null_method(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, NULL, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with NULL token pointer
void test_parse_alt_request_null_token_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, NULL, &database, &query_ref, &params_json
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with NULL database pointer
void test_parse_alt_request_null_database_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, NULL, &query_ref, &params_json
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with NULL query_ref pointer
void test_parse_alt_request_null_query_ref_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, NULL, &params_json
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with NULL params pointer
void test_parse_alt_request_null_params_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, NULL
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with POST mode buffer but NULL data (empty buffer)
// handle_request_parsing_with_buffer returns MHD_NO: "Missing request body"
// Covers lines 151-152
void test_parse_alt_request_parse_failure_empty_buffer(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode: checks data is not NULL/empty
    buffer.data = NULL;         // NULL data → parse_request_data returns NULL → MHD_NO
    buffer.size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    // handle_request_parsing_with_buffer: POST mode with NULL data → "Missing request body" → MHD_NO
    // Lines 151-152 in parse_alt_request are covered
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with missing token field (POST mode)
// Covers the token-missing error path (lines 157-172)
void test_parse_alt_request_missing_token_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"database\": \"test\", \"query_ref\": 123}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with missing database field (POST mode, token present)
// Covers the database-missing error path (lines 185-200)
void test_parse_alt_request_missing_database_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode - token must be present!
    buffer.data = strdup("{\"token\": \"jwt\", \"query_ref\": 123}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    // Token strdup succeeds, then database missing → parse_alt_request frees *token internally
    // Lines 185-200 covered (missing database path)
    // Note: token is freed by parse_alt_request in error path - do NOT free again here
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with missing query_ref field (POST mode, token+database present)
// Covers the query_ref-missing error path (lines 214-230)
void test_parse_alt_request_missing_query_ref_field(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode - token+database must be present!
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": \"test\"}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    // Token strdup + database strdup succeed, query_ref missing → parse_alt_request frees
    // *token and *database internally → Lines 214-230 covered (missing query_ref path)
    // Note: token and database are freed by parse_alt_request internally - do NOT free again
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with invalid token type (not string) - POST mode
// Covers the token-invalid-type error path
void test_parse_alt_request_invalid_token_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"token\": 123, \"database\": \"test\", \"query_ref\": 456}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with invalid database type (not string) - POST mode
// Token is present (string), database is a number
void test_parse_alt_request_invalid_database_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": 123, \"query_ref\": 456}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    // parse_alt_request frees *token internally when database type is invalid
    // Note: do NOT free token here to avoid double-free
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with invalid query_ref type (not integer) - POST mode
// Token + database are present (strings), query_ref is a string
void test_parse_alt_request_invalid_query_ref_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"token\": \"jwt\", \"database\": \"test\", \"query_ref\": \"not_a_number\"}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parse_alt_request with all valid required fields, no params (POST mode)
// token + database + query_ref present, no params field → params_json = NULL
// Covers the else branch (line 240: *params_json = NULL)
void test_parse_alt_request_valid_no_params(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = (json_t*)0x1;  // Pre-set to non-NULL to verify it's cleared
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"token\": \"jwt_token\", \"database\": \"testdb\", \"query_ref\": 42}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);

    // Should succeed: all required fields present, no params
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL_STRING("jwt_token", token);
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_EQUAL(42, query_ref);
    TEST_ASSERT_NULL(params_json);  // Line 240: no params field → params_json = NULL

    free(token);
    free(database);
}

// Test parse_alt_request with all valid fields including params (POST mode)
// Covers the if branch at lines 237-239 (*params_json = json_deep_copy)
void test_parse_alt_request_valid_with_params(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';  // POST mode
    buffer.data = strdup("{\"token\": \"jwt_token\", \"database\": \"testdb\", \"query_ref\": 99, \"params\": {\"key\": \"value\"}}");
    buffer.size = strlen(buffer.data);
    void *con_cls = NULL;

    enum MHD_Result result = parse_alt_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &query_ref, &params_json
    );

    free(buffer.data);

    // Should succeed: all fields present including params
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL_STRING("jwt_token", token);
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_EQUAL(99, query_ref);
    TEST_ASSERT_NOT_NULL(params_json);  // params_json should be set

    free(token);
    free(database);
    json_decref(params_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_alt_request_null_method);
    RUN_TEST(test_parse_alt_request_null_token_ptr);
    RUN_TEST(test_parse_alt_request_null_database_ptr);
    RUN_TEST(test_parse_alt_request_null_query_ref_ptr);
    RUN_TEST(test_parse_alt_request_null_params_ptr);
    RUN_TEST(test_parse_alt_request_parse_failure_empty_buffer);
    RUN_TEST(test_parse_alt_request_missing_token_field);
    RUN_TEST(test_parse_alt_request_missing_database_field);
    RUN_TEST(test_parse_alt_request_missing_query_ref_field);
    RUN_TEST(test_parse_alt_request_invalid_token_type);
    RUN_TEST(test_parse_alt_request_invalid_database_type);
    RUN_TEST(test_parse_alt_request_invalid_query_ref_type);
    RUN_TEST(test_parse_alt_request_valid_no_params);
    RUN_TEST(test_parse_alt_request_valid_with_params);

    return UNITY_END();
}
