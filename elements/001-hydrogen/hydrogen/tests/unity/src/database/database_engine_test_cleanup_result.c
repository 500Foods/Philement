/*
 * Unity Test File: database_engine_cleanup_result
 * This file contains unit tests for database_engine_cleanup_result functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
void database_engine_cleanup_result(QueryResult* result);

// Function prototypes for test functions
void test_database_engine_cleanup_result_null(void);
void test_database_engine_cleanup_result_empty(void);
void test_database_engine_cleanup_result_with_data(void);
void test_database_engine_cleanup_result_with_columns(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_cleanup_result_null(void) {
    // Test cleanup with NULL result - should not crash
    database_engine_cleanup_result(NULL);
    // Function should handle gracefully
}

void test_database_engine_cleanup_result_empty(void) {
    // Test cleanup with empty result structure
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);

    database_engine_cleanup_result(result);
    // Memory should be freed, no crash
}

void test_database_engine_cleanup_result_with_data(void) {
    // Test cleanup with result containing data
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);

    result->data_json = strdup("{\"test\": \"data\"}");
    result->success = true;
    result->row_count = 1;
    result->column_count = 1;

    database_engine_cleanup_result(result);
    // Memory should be freed, no crash
}

void test_database_engine_cleanup_result_with_columns(void) {
    // Test cleanup with result containing column names
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);

    result->column_count = 2;
    result->column_names = calloc(2, sizeof(char*));
    TEST_ASSERT_NOT_NULL(result->column_names);

    result->column_names[0] = strdup("id");
    result->column_names[1] = strdup("name");
    result->data_json = strdup("[]");

    database_engine_cleanup_result(result);
    // Memory should be freed, no crash
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_cleanup_result_null);
    RUN_TEST(test_database_engine_cleanup_result_empty);
    RUN_TEST(test_database_engine_cleanup_result_with_data);
    RUN_TEST(test_database_engine_cleanup_result_with_columns);

    return UNITY_END();
}