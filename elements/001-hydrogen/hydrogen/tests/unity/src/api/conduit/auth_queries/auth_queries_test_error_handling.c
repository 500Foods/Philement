/*
 * Unity Test File: Auth Queries Error Handling
 * This file contains unit tests for error handling paths in auth_queries.c
 *
 * Tests error scenarios for the authenticated queries endpoint including:
 * - Buffer handling paths (CONTINUE, METHOD_ERROR, BUFFER_ERROR)
 * - validate_jwt_and_extract_database error paths
 * - execute_single_auth_query error paths
 * - cleanup_auth_queries_resources comprehensive tests
 *
 * KEY INSIGHT: auth_queries.c always calls the REAL api_buffer_post_data
 * because the macro redefinition only applies to the test file itself, not
 * to the compiled auth_queries.c object.
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation of error handling tests
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
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/auth_queries/auth_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_auth_queries_post_null_data_returns_yes(void);
void test_auth_queries_put_method_returns_yes(void);
void test_auth_queries_null_method_returns_yes(void);
void test_auth_queries_get_method_returns_no(void);
void test_auth_queries_post_with_data_returns_yes(void);
void test_auth_queries_buffer_error_via_malloc_failure(void);
void test_auth_queries_validate_jwt_null_connection(void);
void test_auth_queries_validate_jwt_null_database_ptr(void);
void test_auth_queries_validate_jwt_no_auth_header(void);
void test_auth_queries_validate_jwt_invalid_format(void);
void test_auth_queries_execute_null_database(void);
void test_auth_queries_execute_null_query_obj(void);
void test_auth_queries_execute_missing_query_ref(void);
void test_auth_queries_execute_invalid_query_ref_type(void);
void test_auth_queries_execute_nonexistent_database(void);
void test_auth_queries_cleanup_all_null(void);
void test_auth_queries_cleanup_valid_params(void);
void test_auth_queries_cleanup_partial_null(void);

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

// === Buffer handling tests ===

// Test: POST with NULL data - API_BUFFER_CONTINUE path
void test_auth_queries_post_null_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // POST+NULL first call → allocates buffer → CONTINUE → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) {
        free(con_cls);
    }
}

// Test: PUT method - API_BUFFER_METHOD_ERROR path
void test_auth_queries_put_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    const char *method = "PUT";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // PUT → METHOD_ERROR → sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: NULL method - API_BUFFER_METHOD_ERROR path
void test_auth_queries_null_method_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, NULL, NULL, &upload_data_size, &con_cls
    );

    // NULL method → METHOD_ERROR → sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: GET method - BUFFER_COMPLETE → method validation failure
void test_auth_queries_get_method_returns_no(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    const char *method = "GET";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // GET → COMPLETE → method validation failure → MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test: POST with data → CONTINUE (first call, data being buffered)
void test_auth_queries_post_with_data_returns_yes(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    const char *method = "POST";
    const char *upload_data = "{\"queries\":[{\"query_ref\":1}]}";
    size_t upload_data_size = strlen(upload_data);
    void *con_cls = NULL;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, method, upload_data, &upload_data_size, &con_cls
    );

    // POST+data first call → CONTINUE
    TEST_ASSERT_EQUAL(MHD_YES, result);

    if (con_cls) {
        free(con_cls);
    }
}

// Test: API_BUFFER_ERROR via malloc failure
void test_auth_queries_buffer_error_via_malloc_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *url = "/api/conduit/auth_queries";
    const char *method = "POST";
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    // Fail the second malloc (buffer->data allocation)
    mock_system_set_malloc_failure(2);
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = handle_conduit_auth_queries_request(
        mock_connection, url, method, NULL, &upload_data_size, &con_cls
    );

    // API_BUFFER_ERROR → sends error → MHD_YES
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// === validate_jwt_and_extract_database tests ===

// Test with NULL connection parameter
void test_auth_queries_validate_jwt_null_connection(void) {
    char *database = NULL;

    enum MHD_Result result = validate_jwt_and_extract_database(NULL, &database);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

// Test with NULL database output pointer
void test_auth_queries_validate_jwt_null_database_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test with no Authorization header
void test_auth_queries_validate_jwt_no_auth_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;

    // Mock MHD_lookup_connection_value returns NULL (default mock behavior)
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);

    // Missing auth header → sends error → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

// Test with invalid Authorization header format (no "Bearer " prefix)
void test_auth_queries_validate_jwt_invalid_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;

    // Set mock to return a non-Bearer auth header
    mock_mhd_set_lookup_result("InvalidToken12345");
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);

    // Invalid format → sends error → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(database);
}

// === execute_single_auth_query tests ===

// Test with NULL database
void test_auth_queries_execute_null_database(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));

    json_t *result = execute_single_auth_query(NULL, query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object",
        json_string_value(json_object_get(result, "error")));

    json_decref(result);
    json_decref(query_obj);
}

// Test with NULL query object
void test_auth_queries_execute_null_query_obj(void) {
    json_t *result = execute_single_auth_query("testdb", NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object",
        json_string_value(json_object_get(result, "error")));

    json_decref(result);
}

// Test with missing query_ref
void test_auth_queries_execute_missing_query_ref(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "some_field", json_integer(123));

    json_t *result = execute_single_auth_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Missing required field: query_ref",
        json_string_value(json_object_get(result, "error")));

    json_decref(result);
    json_decref(query_obj);
}

// Test with invalid query_ref type (string instead of integer)
void test_auth_queries_execute_invalid_query_ref_type(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_string("not_a_number"));

    json_t *result = execute_single_auth_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));

    json_decref(result);
    json_decref(query_obj);
}

// Test with nonexistent database
void test_auth_queries_execute_nonexistent_database(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));

    json_t *result = execute_single_auth_query("nonexistent_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    // Will fail at database lookup
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));

    json_decref(result);
    json_decref(query_obj);
}

// === cleanup_auth_queries_resources tests ===

// Test with all NULL parameters
void test_auth_queries_cleanup_all_null(void) {
    cleanup_auth_queries_resources(NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    TEST_PASS_MESSAGE("cleanup_auth_queries_resources handled all NULL parameters gracefully");
}

// Test with valid parameters
void test_auth_queries_cleanup_valid_params(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "test", json_true());

    char *database = strdup("testdb");
    json_t *queries_array = json_array();
    json_t *deduplicated_queries = json_array();
    size_t *mapping_array = calloc(1, sizeof(size_t));
    bool *is_duplicate = calloc(1, sizeof(bool));

    json_t **unique_results = calloc(2, sizeof(json_t*));
    unique_results[0] = json_object();
    json_object_set_new(unique_results[0], "success", json_true());
    unique_results[1] = json_object();
    json_object_set_new(unique_results[1], "success", json_false());

    cleanup_auth_queries_resources(
        request_json, database, queries_array, deduplicated_queries,
        mapping_array, is_duplicate, unique_results, 2
    );

    TEST_PASS_MESSAGE("cleanup_auth_queries_resources handled valid parameters correctly");
}

// Test with some NULL and some valid parameters
void test_auth_queries_cleanup_partial_null(void) {
    json_t *request_json = json_object();
    char *database = strdup("testdb");

    cleanup_auth_queries_resources(
        request_json, database, NULL, NULL, NULL, NULL, NULL, 0
    );

    TEST_PASS_MESSAGE("cleanup_auth_queries_resources handled partial NULL parameters gracefully");
}

int main(void) {
    UNITY_BEGIN();

    // Buffer handling tests
    RUN_TEST(test_auth_queries_post_null_data_returns_yes);
    RUN_TEST(test_auth_queries_put_method_returns_yes);
    RUN_TEST(test_auth_queries_null_method_returns_yes);
    RUN_TEST(test_auth_queries_get_method_returns_no);
    RUN_TEST(test_auth_queries_post_with_data_returns_yes);
    RUN_TEST(test_auth_queries_buffer_error_via_malloc_failure);

    // validate_jwt_and_extract_database tests
    RUN_TEST(test_auth_queries_validate_jwt_null_connection);
    RUN_TEST(test_auth_queries_validate_jwt_null_database_ptr);
    RUN_TEST(test_auth_queries_validate_jwt_no_auth_header);
    RUN_TEST(test_auth_queries_validate_jwt_invalid_format);

    // execute_single_auth_query tests
    RUN_TEST(test_auth_queries_execute_null_database);
    RUN_TEST(test_auth_queries_execute_null_query_obj);
    RUN_TEST(test_auth_queries_execute_missing_query_ref);
    RUN_TEST(test_auth_queries_execute_invalid_query_ref_type);
    RUN_TEST(test_auth_queries_execute_nonexistent_database);

    // cleanup tests
    RUN_TEST(test_auth_queries_cleanup_all_null);
    RUN_TEST(test_auth_queries_cleanup_valid_params);
    RUN_TEST(test_auth_queries_cleanup_partial_null);

    return UNITY_END();
}
