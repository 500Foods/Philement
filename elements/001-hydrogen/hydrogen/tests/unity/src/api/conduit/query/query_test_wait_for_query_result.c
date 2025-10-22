/*
 * Unity Test File: wait_for_query_result
 * This file contains unit tests for wait_for_query_result function in query.c
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/database_pending.h"

// Include source header
#include "../../../../../src/api/conduit/query/query.h"

// Mock functions for pending_result_wait and pending_result_get

#define pending_result_wait mock_pending_result_wait
#define pending_result_get mock_pending_result_get

// Dummy types for compilation

typedef struct PendingQueryResult PendingQueryResult;

// Mock implementations
static int mock_wait_result = 0;
static const QueryResult* mock_get_result = NULL;

static int mock_pending_result_wait(PendingQueryResult* pending) {
    (void)pending;
    return mock_wait_result;
}

static const QueryResult* mock_pending_result_get(PendingQueryResult* pending) {
    (void)pending;
    return mock_get_result;
}

void setUp(void) {
    // Reset mocks
    mock_wait_result = 0;
    mock_get_result = NULL;
    (void)mock_pending_result_wait((PendingQueryResult*)NULL);
    (void)mock_pending_result_get((PendingQueryResult*)NULL);
}

void tearDown(void) {
    // Clean up if needed
}

// Test with NULL pending
static void test_wait_for_query_result_null_pending(void) {
    PendingQueryResult* pending = NULL;

    const QueryResult* result = wait_for_query_result(pending);

    TEST_ASSERT_NULL(result);
}

// Test with wait_result != 0 (failure)
static void test_wait_for_query_result_wait_failure(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    mock_wait_result = -1; // Simulate failure

    const QueryResult* result = wait_for_query_result(dummy_pending);

    TEST_ASSERT_NULL(result);
}

// Test with wait_result == 0 and valid get result
static void test_wait_for_query_result_success(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    mock_wait_result = 0; // Success

    mock_get_result = (const QueryResult*)0x1;

    const QueryResult* result = wait_for_query_result(dummy_pending);

    TEST_ASSERT_NOT_NULL(result);
}

// Test with wait_result == 0 but get returns NULL
static void test_wait_for_query_result_get_null(void) {
    PendingQueryResult* dummy_pending = (PendingQueryResult*)0x1;
    mock_wait_result = 0;
    mock_get_result = NULL;

    const QueryResult* result = wait_for_query_result(dummy_pending);

    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_wait_for_query_result_null_pending);
    RUN_TEST(test_wait_for_query_result_wait_failure);
    RUN_TEST(test_wait_for_query_result_success);
    RUN_TEST(test_wait_for_query_result_get_null);

    return UNITY_END();
}