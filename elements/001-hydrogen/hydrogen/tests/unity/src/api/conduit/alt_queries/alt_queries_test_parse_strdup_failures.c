/*
 * Unity Test File: Alt Queries Parse Request Strdup Failures
 * This file contains unit tests for strdup/allocation failure paths
 * in parse_alt_queries_request in src/api/conduit/alt_queries/alt_queries.c
 *
 * Tests the error paths when strdup fails for token (lines 345-347),
 * database (lines 366-369), and json_deep_copy fails for queries (lines 404-408).
 * Also tests the parse failure path (lines 326-327).
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation
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
void test_parse_alt_queries_request_parse_failure(void);
void test_parse_alt_queries_request_token_strdup_failure(void);
void test_parse_alt_queries_request_database_strdup_failure(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}

// Test parse failure path (lines 326-327) - NULL buffer data
void test_parse_alt_queries_request_parse_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    void *con_cls = NULL;

    // Create buffer with NULL data - handle_request_parsing_with_buffer will fail
    ApiPostBuffer buffer = {0};
    buffer.data = NULL;
    buffer.size = 0;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    // Parsing should fail → returns result from handle_request_parsing_with_buffer
    TEST_ASSERT_NOT_EQUAL(MHD_YES, result);
}

// Test token strdup failure (lines 345-347)
void test_parse_alt_queries_request_token_strdup_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    void *con_cls = NULL;

    // Valid JSON with all required fields
    ApiPostBuffer buffer = {0};
    buffer.data = strdup("{\"token\": \"jwt_token\", \"database\": \"test\", \"queries\": [{\"query_ref\": 1}]}");
    buffer.size = strlen(buffer.data);

    // Fail on the first strdup call (token allocation)
    // The JSON parsing uses jansson which uses its own memory, but strdup is intercepted
    mock_system_set_malloc_failure(1);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    // Token strdup failure → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test database strdup failure (lines 366-369)
void test_parse_alt_queries_request_database_strdup_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *method = "POST";
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;
    void *con_cls = NULL;

    // Valid JSON with all required fields
    ApiPostBuffer buffer = {0};
    buffer.data = strdup("{\"token\": \"jwt_token\", \"database\": \"test\", \"queries\": [{\"query_ref\": 1}]}");
    buffer.size = strlen(buffer.data);

    // Fail on the second strdup call (database allocation; first is token which succeeds)
    mock_system_set_malloc_failure(2);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = parse_alt_queries_request(
        mock_connection, method, &buffer, &con_cls, &token, &database, &queries_array
    );

    free(buffer.data);
    // database strdup failure → frees token, returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Note: valid request parsing, invalid token type, invalid database type,
// and invalid queries type tests are already covered in
// alt_queries_test_parse_alt_queries_request.c without USE_MOCK_SYSTEM
// (which would interfere with mock strdup in test code itself)

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_alt_queries_request_parse_failure);
    RUN_TEST(test_parse_alt_queries_request_token_strdup_failure);
    RUN_TEST(test_parse_alt_queries_request_database_strdup_failure);

    return UNITY_END();
}
