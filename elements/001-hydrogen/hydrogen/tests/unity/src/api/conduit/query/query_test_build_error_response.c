/*
 * Unity Test File: build_error_response
 * This file contains unit tests for build_error_response function in query.c
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include <jansson.h>
#include "../../../../../src/database/database_pending.h"
#include "../../../../../src/database/database_cache.h"

// Include source header
#include "../../../../../src/api/conduit/query/query.h"

// Mock for pending_result_is_timed_out
static bool mock_pending_result_is_timed_out(const PendingQueryResult* pending);
#define pending_result_is_timed_out mock_pending_result_is_timed_out

// Dummy types for compilation

typedef struct PendingQueryResult PendingQueryResult;
typedef struct QueryResult QueryResult;
typedef struct QueryCacheEntry QueryCacheEntry;

// Mock implementation
static bool mock_is_timed_out = false;

static bool mock_pending_result_is_timed_out(const PendingQueryResult* pending) {
    (void)pending;
    return mock_is_timed_out;
}

void setUp(void) {
    // Reset mocks
    mock_is_timed_out = false;
    (void)mock_pending_result_is_timed_out(NULL);
}

void tearDown(void) {
    // Clean up
}

// Test timeout case
static void test_build_error_response_timeout(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = (QueryCacheEntry*)0x1; // Dummy
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* dummy_result = (const QueryResult*)0x1;
    mock_is_timed_out = true;

    json_t* response = build_error_response(query_ref, database, cache_entry, dummy_pending, dummy_result);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution timeout", json_string_value(error));

    json_decref(response);
}

// Test database error case
static void test_build_error_response_database_error(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = (QueryCacheEntry*)0x1;
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* result = (const QueryResult*)0x1; // Assume error_message set
    mock_is_timed_out = false;

    json_t* response = build_error_response(query_ref, database, cache_entry, dummy_pending, result);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Database error", json_string_value(error));

    json_decref(response);
}

// Test general failure case
static void test_build_error_response_general_failure(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = (QueryCacheEntry*)0x1;
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    const QueryResult* result = (const QueryResult*)0x1; // No error_message
    mock_is_timed_out = false;

    json_t* response = build_error_response(query_ref, database, cache_entry, dummy_pending, result);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution failed", json_string_value(error));

    json_decref(response);
}

// Test with NULL result
static void test_build_error_response_null_result(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = (QueryCacheEntry*)0x1;
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    QueryResult* result = NULL;
    mock_is_timed_out = false;

    json_t* response = build_error_response(query_ref, database, cache_entry, dummy_pending, result);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution failed", json_string_value(error));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_error_response_timeout);
    RUN_TEST(test_build_error_response_database_error);
    RUN_TEST(test_build_error_response_general_failure);
    RUN_TEST(test_build_error_response_null_result);

    return UNITY_END();
}