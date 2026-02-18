/*
 * Unity Test File: Alt Queries Execute Single Alt Query
 * This file contains unit tests for the execute_single_alt_query function
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * Tests single query execution logic for alternative authenticated queries.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for execute_single_alt_query
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source headers
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/alt_queries/alt_queries.h>

// Function prototypes for test functions
void test_execute_single_alt_query_null_query_obj(void);
void test_execute_single_alt_query_missing_query_ref(void);
void test_execute_single_alt_query_invalid_query_ref_type(void);
void test_execute_single_alt_query_with_params(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// Test execute_single_alt_query with NULL query object
void test_execute_single_alt_query_null_query_obj(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *database = "testdb";
    int query_ref = 0;
    PendingQueryResult *pending = NULL;
    QueryCacheEntry *cache_entry = NULL;
    DatabaseQueue *selected_queue = NULL;

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = execute_single_alt_query(
        mock_connection, NULL, database, &query_ref, &pending, &cache_entry, &selected_queue
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test execute_single_alt_query with missing query_ref field
void test_execute_single_alt_query_missing_query_ref(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *database = "testdb";
    int query_ref = 0;
    PendingQueryResult *pending = NULL;
    QueryCacheEntry *cache_entry = NULL;
    DatabaseQueue *selected_queue = NULL;

    // Create query object without query_ref
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "params", json_object());

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = execute_single_alt_query(
        mock_connection, query_obj, database, &query_ref, &pending, &cache_entry, &selected_queue
    );

    json_decref(query_obj);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test execute_single_alt_query with invalid query_ref type (not integer)
void test_execute_single_alt_query_invalid_query_ref_type(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *database = "testdb";
    int query_ref = 0;
    PendingQueryResult *pending = NULL;
    QueryCacheEntry *cache_entry = NULL;
    DatabaseQueue *selected_queue = NULL;

    // Create query object with string query_ref instead of integer
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_string("not_a_number"));

    // Mock MHD to return YES for error response
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = execute_single_alt_query(
        mock_connection, query_obj, database, &query_ref, &pending, &cache_entry, &selected_queue
    );

    json_decref(query_obj);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test execute_single_alt_query with params field
void test_execute_single_alt_query_with_params(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    const char *database = "testdb";
    int query_ref = 0;
    PendingQueryResult *pending = NULL;
    QueryCacheEntry *cache_entry = NULL;
    DatabaseQueue *selected_queue = NULL;

    // Create query object with params
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));
    json_t *params = json_object();
    json_t *int_params = json_object();
    json_object_set_new(int_params, "id", json_integer(123));
    json_object_set_new(params, "INTEGER", int_params);
    json_object_set_new(query_obj, "params", params);

    // Mock MHD to return YES for error response (database lookup will fail)
    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = execute_single_alt_query(
        mock_connection, query_obj, database, &query_ref, &pending, &cache_entry, &selected_queue
    );

    json_decref(query_obj);
    // Should fail at database lookup since database is not configured
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_alt_query_null_query_obj);
    RUN_TEST(test_execute_single_alt_query_missing_query_ref);
    RUN_TEST(test_execute_single_alt_query_invalid_query_ref_type);
    RUN_TEST(test_execute_single_alt_query_with_params);

    return UNITY_END();
}
