/*
 * Unity Test File: build_response_json
 * This file contains unit tests for build_response_json function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include "unity.h"

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Mock for wait_for_query_result
const QueryResult* mock_wait_for_query_result(PendingQueryResult* pending);

// Override the function
#define wait_for_query_result mock_wait_for_query_result

// Mock state
static const QueryResult* mock_result = NULL;

// Function prototypes
void test_build_response_json_error_case(void);
void test_build_response_json_null_pending(void);
void test_build_response_json_success_case(void);

void setUp(void) {
    mock_system_reset_all();
    mock_result = NULL;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_result = NULL;
}

// Mock implementation
const QueryResult* mock_wait_for_query_result(PendingQueryResult* pending) {
    (void)pending;
    return mock_result;
}

// Test error response (since we can't easily mock successful query results)
void test_build_response_json_error_case(void) {
    // Create mock data
    QueryCacheEntry cache_entry = {
        .description = (char*)"Test query",
        .timeout_seconds = 30
    };

    DatabaseQueue selected_queue = {
        .queue_type = (char*)"read"
    };

    PendingQueryResult pending = {0};

    // Since we can't mock wait_for_query_result easily, the function will go to error path
    // This still provides coverage for the error handling
    json_t* response = build_response_json(123, "test_db", &cache_entry, &selected_queue, &pending, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check that it's an error response (since no real query is running)
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check query_ref
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));

    // Check database
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    json_decref(response);
}


// Test with NULL pending result
void test_build_response_json_null_pending(void) {
    QueryCacheEntry cache_entry = {
        .description = (char*)"Test query"
    };

    DatabaseQueue selected_queue = {
        .queue_type = (char*)"read"
    };

    json_t* response = build_response_json(789, "test_db", &cache_entry, &selected_queue, NULL, NULL);

    // Should handle NULL pending gracefully (though this might not be realistic)
    TEST_ASSERT_NOT_NULL(response);
    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_response_json_error_case);
    RUN_TEST(test_build_response_json_null_pending);

    return UNITY_END();
}