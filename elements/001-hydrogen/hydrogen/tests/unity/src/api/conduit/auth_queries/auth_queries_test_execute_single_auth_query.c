/*
 * Unity Test File: Auth Queries Execute Single Query
 * This file contains unit tests for the execute_single_auth_query function
 * in src/api/conduit/auth_queries/auth_queries.c
 *
 * Tests single query execution logic for authenticated queries.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for execute_single_auth_query
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
#include <src/api/conduit/auth_queries/auth_queries.h>

// Function prototypes for test functions
void test_execute_single_auth_query_null_database(void);
void test_execute_single_auth_query_null_query_obj(void);
void test_execute_single_auth_query_missing_query_ref(void);
void test_execute_single_auth_query_invalid_query_ref_type(void);
void test_execute_single_auth_query_with_params(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test execute_single_auth_query with NULL database
void test_execute_single_auth_query_null_database(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));

    json_t *result = execute_single_auth_query(NULL, query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    
    json_t *error = json_object_get(result, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));

    json_decref(result);
    json_decref(query_obj);
}

// Test execute_single_auth_query with NULL query object
void test_execute_single_auth_query_null_query_obj(void) {
    json_t *result = execute_single_auth_query("testdb", NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    
    json_t *error = json_object_get(result, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));

    json_decref(result);
}

// Test execute_single_auth_query with missing query_ref
void test_execute_single_auth_query_missing_query_ref(void) {
    json_t *query_obj = json_object();
    // Don't add query_ref field
    json_object_set_new(query_obj, "some_field", json_integer(123));

    json_t *result = execute_single_auth_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    
    json_t *error = json_object_get(result, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));

    json_decref(result);
    json_decref(query_obj);
}

// Test execute_single_auth_query with invalid query_ref type
void test_execute_single_auth_query_invalid_query_ref_type(void) {
    json_t *query_obj = json_object();
    // query_ref should be integer, not string
    json_object_set_new(query_obj, "query_ref", json_string("not_a_number"));

    json_t *result = execute_single_auth_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    
    json_t *error = json_object_get(result, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));

    json_decref(result);
    json_decref(query_obj);
}

// Test execute_single_auth_query with params field
void test_execute_single_auth_query_with_params(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));
    
    json_t *params = json_object();
    json_t *int_params = json_object();
    json_object_set_new(int_params, "id", json_integer(123));
    json_object_set_new(params, "INTEGER", int_params);
    json_object_set_new(query_obj, "params", params);

    json_t *result = execute_single_auth_query("testdb", query_obj);

    // Query will fail because database is not configured, but function should handle it gracefully
    TEST_ASSERT_NOT_NULL(result);
    
    json_decref(result);
    json_decref(query_obj);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_auth_query_null_database);
    RUN_TEST(test_execute_single_auth_query_null_query_obj);
    RUN_TEST(test_execute_single_auth_query_missing_query_ref);
    RUN_TEST(test_execute_single_auth_query_invalid_query_ref_type);
    RUN_TEST(test_execute_single_auth_query_with_params);

    return UNITY_END();
}
