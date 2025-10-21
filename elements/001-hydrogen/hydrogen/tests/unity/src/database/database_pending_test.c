/*
 * Unity Test File: Database Pending Results
 * This file contains unit tests for synchronous query execution
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database_pending.h"
#include "../../../../src/database/database.h"

// Function prototypes for Unity tests
void test_pending_result_manager_create_destroy(void);
void test_pending_result_register(void);
void test_pending_result_register_multiple(void);
void test_pending_result_signal_ready(void);
void test_pending_result_signal_ready_not_found(void);
void test_pending_result_cleanup_expired(void);
void test_pending_result_cleanup_not_expired(void);
void test_get_pending_result_manager(void);
void test_pending_result_manager_expansion(void);
void test_pending_result_null_parameters(void);
void test_pending_result_state_checks(void);

// Test setup and teardown
void setUp(void) {
    // Reset global pending result manager before each test
    // Note: We can't directly access g_pending_manager from test code
    // The get_pending_result_manager() function handles initialization
}

void tearDown(void) {
    // Clean up after each test
    // Note: We can't directly access g_pending_manager from test code
    // The manager will be cleaned up by the next test's setUp
}

// Test manager creation and destruction
void test_pending_result_manager_create_destroy(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_EQUAL(0, manager->count);
    TEST_ASSERT_TRUE(manager->capacity > 0);

    pending_result_manager_destroy(manager);
}

// Test registering a pending result
void test_pending_result_register(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "test_query_123", 30);
    TEST_ASSERT_NOT_NULL(pending);
    TEST_ASSERT_EQUAL_STRING("test_query_123", pending->query_id);
    TEST_ASSERT_EQUAL(30, pending->timeout_seconds);
    TEST_ASSERT_FALSE(pending->completed);
    TEST_ASSERT_FALSE(pending->timed_out);
    // Note: Cannot test pthread types directly as they are opaque
    // The fact that registration succeeded indicates they were initialized

    TEST_ASSERT_EQUAL(1, manager->count);

    pending_result_manager_destroy(manager);
}

// Test registering multiple pending results
void test_pending_result_register_multiple(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending1 = pending_result_register(manager, "query1", 10);
    PendingQueryResult* pending2 = pending_result_register(manager, "query2", 20);
    PendingQueryResult* pending3 = pending_result_register(manager, "query3", 30);

    TEST_ASSERT_NOT_NULL(pending1);
    TEST_ASSERT_NOT_NULL(pending2);
    TEST_ASSERT_NOT_NULL(pending3);

    TEST_ASSERT_EQUAL(3, manager->count);
    TEST_ASSERT_EQUAL_STRING("query1", pending1->query_id);
    TEST_ASSERT_EQUAL_STRING("query2", pending2->query_id);
    TEST_ASSERT_EQUAL_STRING("query3", pending3->query_id);

    pending_result_manager_destroy(manager);
}

// Test signaling result ready
void test_pending_result_signal_ready(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result
    PendingQueryResult* pending = pending_result_register(manager, "test_signal", 30);
    TEST_ASSERT_NOT_NULL(pending);

    // Create a mock result - use heap allocation to avoid double-free issues
    QueryResult* mock_result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(mock_result);
    mock_result->success = true;
    mock_result->data_json = NULL;
    mock_result->row_count = 2;
    mock_result->column_count = 2;
    mock_result->column_names = NULL;
    mock_result->error_message = NULL;
    mock_result->execution_time_ms = 100;
    mock_result->affected_rows = 0;

    // Signal that result is ready
    bool signaled = pending_result_signal_ready(manager, "test_signal", mock_result);
    TEST_ASSERT_TRUE(signaled);

    // Check that the result was stored
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));
    TEST_ASSERT_FALSE(pending_result_is_timed_out(pending));

    QueryResult* retrieved = pending_result_get(pending);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(2, retrieved->row_count);
    TEST_ASSERT_EQUAL(2, retrieved->column_count);
    // Note: data_json is NULL in the mock result, so we don't test it

    pending_result_manager_destroy(manager);
}

// Test signaling result ready for non-existent query
void test_pending_result_signal_ready_not_found(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    // Create a mock result - use heap allocation to avoid issues
    QueryResult* mock_result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(mock_result);
    mock_result->success = true;
    mock_result->data_json = NULL;
    mock_result->row_count = 1;
    mock_result->column_count = 1;
    mock_result->column_names = NULL;
    mock_result->error_message = NULL;
    mock_result->execution_time_ms = 50;
    mock_result->affected_rows = 0;

    // Try to signal a non-existent query
    bool signaled = pending_result_signal_ready(manager, "non_existent", mock_result);
    TEST_ASSERT_FALSE(signaled);

    // Clean up the result since it wasn't claimed
    free(mock_result);

    pending_result_manager_destroy(manager);
}

// Test cleanup of expired results
void test_pending_result_cleanup_expired(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result with short timeout
    PendingQueryResult* pending = pending_result_register(manager, "expired_test", 1);
    TEST_ASSERT_NOT_NULL(pending);

    // Manually set submitted time to past
    pending->submitted_at = time(NULL) - 5; // 5 seconds ago

    // Run cleanup
    size_t cleaned = pending_result_cleanup_expired(manager);
    TEST_ASSERT_EQUAL(1, cleaned);
    TEST_ASSERT_EQUAL(0, manager->count);

    pending_result_manager_destroy(manager);
}

// Test that non-expired results are not cleaned up
void test_pending_result_cleanup_not_expired(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result with long timeout
    PendingQueryResult* pending = pending_result_register(manager, "not_expired", 300);
    TEST_ASSERT_NOT_NULL(pending);

    // Run cleanup
    size_t cleaned = pending_result_cleanup_expired(manager);
    TEST_ASSERT_EQUAL(0, cleaned);
    TEST_ASSERT_EQUAL(1, manager->count);

    pending_result_manager_destroy(manager);
}

// Test global manager access
void test_get_pending_result_manager(void) {
    // First call should create manager
    PendingResultManager* manager1 = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager1);

    // Second call should return same manager
    PendingResultManager* manager2 = get_pending_result_manager();
    TEST_ASSERT_EQUAL(manager1, manager2);
}

// Test manager expansion when capacity exceeded
void test_pending_result_manager_expansion(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    size_t initial_capacity = manager->capacity;

    // Fill to capacity
    for (size_t i = 0; i < initial_capacity; i++) {
        char query_id[32];
        snprintf(query_id, sizeof(query_id), "query_%zu", i);
        PendingQueryResult* pending = pending_result_register(manager, query_id, 30);
        TEST_ASSERT_NOT_NULL(pending);
    }

    TEST_ASSERT_EQUAL(initial_capacity, manager->count);

    // Add one more to trigger expansion
    PendingQueryResult* extra_pending = pending_result_register(manager, "extra_query", 30);
    TEST_ASSERT_NOT_NULL(extra_pending);

    TEST_ASSERT_TRUE(manager->capacity > initial_capacity);
    TEST_ASSERT_EQUAL(initial_capacity + 1, manager->count);

    pending_result_manager_destroy(manager);
}

// Test NULL parameter handling
void test_pending_result_null_parameters(void) {
    // Test manager creation with NULL
    pending_result_manager_destroy(NULL); // Should not crash

    // Test registration with NULL parameters
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending1 = pending_result_register(NULL, "test", 30);
    TEST_ASSERT_NULL(pending1);

    PendingQueryResult* pending2 = pending_result_register(manager, NULL, 30);
    TEST_ASSERT_NULL(pending2);

    pending_result_manager_destroy(manager);

    // Test signaling with NULL parameters
    bool signaled1 = pending_result_signal_ready(NULL, "test", NULL);
    TEST_ASSERT_FALSE(signaled1);

    bool signaled2 = pending_result_signal_ready(manager, NULL, NULL);
    TEST_ASSERT_FALSE(signaled2);
}

// Test result state checks
void test_pending_result_state_checks(void) {
    PendingResultManager* manager = pending_result_manager_create();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "state_test", 30);
    TEST_ASSERT_NOT_NULL(pending);

    // Initially not completed or timed out
    TEST_ASSERT_FALSE(pending_result_is_completed(pending));
    TEST_ASSERT_FALSE(pending_result_is_timed_out(pending));

    // Signal completion - create a properly initialized result
    QueryResult* mock_result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(mock_result);
    mock_result->success = true;
    mock_result->data_json = strdup("{\"test\": \"data\"}");  // Add some test data
    mock_result->row_count = 2;
    mock_result->column_count = 2;
    mock_result->column_names = calloc(mock_result->column_count, sizeof(char*));
    TEST_ASSERT_NOT_NULL(mock_result->column_names);
    mock_result->column_names[0] = strdup("id");
    mock_result->column_names[1] = strdup("name");
    mock_result->error_message = NULL;
    mock_result->execution_time_ms = 75;
    mock_result->affected_rows = 0;

    bool signaled = pending_result_signal_ready(manager, "state_test", mock_result);
    TEST_ASSERT_TRUE(signaled);

    // Now should be completed
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));
    TEST_ASSERT_FALSE(pending_result_is_timed_out(pending));

    pending_result_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_pending_result_manager_create_destroy);
    RUN_TEST(test_pending_result_register);
    RUN_TEST(test_pending_result_register_multiple);
    RUN_TEST(test_pending_result_signal_ready);
    RUN_TEST(test_pending_result_signal_ready_not_found);
    RUN_TEST(test_pending_result_cleanup_expired);
    RUN_TEST(test_pending_result_cleanup_not_expired);
    RUN_TEST(test_get_pending_result_manager);
    RUN_TEST(test_pending_result_manager_expansion);
    RUN_TEST(test_pending_result_null_parameters);
    RUN_TEST(test_pending_result_state_checks);

    return UNITY_END();
}