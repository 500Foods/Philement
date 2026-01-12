/*
 * Unity Test File: free_query_result helper function
 * This file tests the newly extracted helper function for cleanup
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <string.h>

// Forward declarations for functions being tested
void free_query_result(QueryResult* result);

// Forward declarations for test functions
void test_free_query_result_with_null(void);
void test_free_query_result_with_empty_struct(void);
void test_free_query_result_with_error_message(void);
void test_free_query_result_with_data_json(void);
void test_free_query_result_with_both_fields(void);
void test_free_query_result_multiple_calls(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

/**
 * Test: free_query_result_with_null
 * Purpose: Verify function handles NULL pointer gracefully
 */
void test_free_query_result_with_null(void) {
    // Should not crash when called with NULL
    free_query_result(NULL);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

/**
 * Test: free_query_result_with_empty_struct
 * Purpose: Verify function handles empty structure (all fields NULL/0)
 */
void test_free_query_result_with_empty_struct(void) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);
    
    // All fields should be NULL/0 from calloc
    result->success = false;
    result->row_count = 0;
    result->execution_time_ms = 0;
    result->error_message = NULL;
    result->data_json = NULL;
    
    // Should free the structure without issues
    free_query_result(result);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

/**
 * Test: free_query_result_with_error_message
 * Purpose: Verify function frees error_message properly
 */
void test_free_query_result_with_error_message(void) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);
    
    result->success = false;
    result->error_message = strdup("Test error message");
    result->data_json = NULL;
    
    // Should free both error_message and structure
    free_query_result(result);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

/**
 * Test: free_query_result_with_data_json
 * Purpose: Verify function frees data_json properly
 */
void test_free_query_result_with_data_json(void) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);
    
    result->success = true;
    result->error_message = NULL;
    result->data_json = strdup("{\"test\": \"data\"}");
    
    // Should free both data_json and structure
    free_query_result(result);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

/**
 * Test: free_query_result_with_both_fields
 * Purpose: Verify function frees both error_message and data_json
 */
void test_free_query_result_with_both_fields(void) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);
    
    result->success = false;
    result->error_message = strdup("Error occurred");
    result->data_json = strdup("{\"partial\": \"data\"}");
    result->row_count = 0;
    result->execution_time_ms = 150;
    
    // Should free all allocated fields and structure
    free_query_result(result);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

/**
 * Test: free_query_result_multiple_calls
 * Purpose: Verify calling function multiple times with NULL is safe
 */
void test_free_query_result_multiple_calls(void) {
    // Multiple calls with NULL should be safe
    free_query_result(NULL);
    free_query_result(NULL);
    free_query_result(NULL);
    TEST_ASSERT_TRUE(true); // If we get here, no crash occurred
}

int main(void) {
    UNITY_BEGIN();

    // Test NULL handling
    RUN_TEST(test_free_query_result_with_null);
    
    // Test various structure states
    RUN_TEST(test_free_query_result_with_empty_struct);
    RUN_TEST(test_free_query_result_with_error_message);
    RUN_TEST(test_free_query_result_with_data_json);
    RUN_TEST(test_free_query_result_with_both_fields);
    
    // Test edge cases
    RUN_TEST(test_free_query_result_multiple_calls);

    return UNITY_END();
}
