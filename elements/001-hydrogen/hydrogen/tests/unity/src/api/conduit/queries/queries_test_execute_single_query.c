/*
 * Unity Test File: Conduit Queries Execute Single Query
 * This file contains unit tests for the execute_single_query function
 * in src/api/conduit/queries/queries.c
 *
 * CHANGELOG:
 * 2026-01-15: Initial creation of unit tests for execute_single_query
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing
#define USE_MOCK_LAUNCH
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/queries/queries.h>

// Function prototypes for test functions
void test_execute_single_query_null_database(void);
void test_execute_single_query_null_query_obj(void);
void test_execute_single_query_missing_query_ref(void);
void test_execute_single_query_invalid_query_ref_type(void);
void test_execute_single_query_database_not_found(void);
void test_execute_single_query_query_not_found(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_launch_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_launch_reset_all();
    mock_system_reset_all();
}

// Test execute_single_query with NULL database
void test_execute_single_query_null_database(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(123));

    json_t *result = execute_single_query(NULL, query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with NULL query_obj
void test_execute_single_query_null_query_obj(void) {
    json_t *result = execute_single_query("test_db", NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object", json_string_value(json_object_get(result, "error")));

    json_decref(result);
}

// Test execute_single_query with missing query_ref
void test_execute_single_query_missing_query_ref(void) {
    json_t *query_obj = json_object();
    // No query_ref set

    json_t *result = execute_single_query("test_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Missing required field: query_ref", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with invalid query_ref type
void test_execute_single_query_invalid_query_ref_type(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_string("not_a_number"));

    json_t *result = execute_single_query("test_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Missing required field: query_ref", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with database not found
void test_execute_single_query_database_not_found(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(123));

    // Note: This test would require mocking the database lookup functions
    // For now, we test the basic parameter validation
    // In a full implementation, we'd mock lookup_database_and_query to return false

    json_t *result = execute_single_query("nonexistent_db", query_obj);

    // The function will attempt to look up the database and fail
    // Since we can't easily mock all the database functions, this test
    // serves as a placeholder for future enhancement

    TEST_ASSERT_NOT_NULL(result);
    // The exact error depends on the database lookup implementation

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with query not found
void test_execute_single_query_query_not_found(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(99999)); // Non-existent query

    json_t *result = execute_single_query("test_db", query_obj);

    // Similar to above, this would require mocking
    TEST_ASSERT_NOT_NULL(result);

    json_decref(query_obj);
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_query_null_database);
    RUN_TEST(test_execute_single_query_null_query_obj);
    RUN_TEST(test_execute_single_query_missing_query_ref);
    RUN_TEST(test_execute_single_query_invalid_query_ref_type);
    RUN_TEST(test_execute_single_query_database_not_found);
    RUN_TEST(test_execute_single_query_query_not_found);

    return UNITY_END();
}