/*
 * Unity Test File: Database Pending Results
 * This file contains unit tests for synchronous query execution
 */

// Enable mocks BEFORE any other includes to override system functions
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database_pending.h>
#include <src/database/database.h>

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
void test_pending_result_manager_create_malloc_failure(void);
void test_pending_result_manager_create_calloc_failure(void);
void test_pending_result_manager_create_null_parameter(void);
void test_pending_result_register_malloc_failure(void);
void test_pending_result_register_strdup_failure(void);
void test_pending_result_register_null_manager(void);
void test_pending_result_register_null_query_id(void);
void test_pending_result_register_realloc_failure(void);
void test_pending_result_wait_timeout(void);
void test_pending_result_wait_multiple(void);
void test_pending_result_wait_multiple_null_parameters(void);
void test_pending_result_wait_multiple_timeout(void);
void test_pending_result_cleanup_expired_with_result(void);

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
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_EQUAL(0, manager->count);
    TEST_ASSERT_TRUE(manager->capacity > 0);

    pending_result_manager_destroy(manager, NULL);
}

// Test registering a pending result
void test_pending_result_register(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "test_query_123", 30, NULL);
    TEST_ASSERT_NOT_NULL(pending);
    TEST_ASSERT_EQUAL_STRING("test_query_123", pending->query_id);
    TEST_ASSERT_EQUAL(30, pending->timeout_seconds);
    TEST_ASSERT_FALSE(pending->completed);
    TEST_ASSERT_FALSE(pending->timed_out);
    // Note: Cannot test pthread types directly as they are opaque
    // The fact that registration succeeded indicates they were initialized

    TEST_ASSERT_EQUAL(1, manager->count);

    pending_result_manager_destroy(manager, NULL);
}

// Test registering multiple pending results
void test_pending_result_register_multiple(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending1 = pending_result_register(manager, "query1", 10, NULL);
    PendingQueryResult* pending2 = pending_result_register(manager, "query2", 20, NULL);
    PendingQueryResult* pending3 = pending_result_register(manager, "query3", 30, NULL);

    TEST_ASSERT_NOT_NULL(pending1);
    TEST_ASSERT_NOT_NULL(pending2);
    TEST_ASSERT_NOT_NULL(pending3);

    TEST_ASSERT_EQUAL(3, manager->count);
    TEST_ASSERT_EQUAL_STRING("query1", pending1->query_id);
    TEST_ASSERT_EQUAL_STRING("query2", pending2->query_id);
    TEST_ASSERT_EQUAL_STRING("query3", pending3->query_id);

    pending_result_manager_destroy(manager, NULL);
}

// Test signaling result ready
void test_pending_result_signal_ready(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result
    PendingQueryResult* pending = pending_result_register(manager, "test_signal", 30, NULL);
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
    bool signaled = pending_result_signal_ready(manager, "test_signal", mock_result, NULL);
    TEST_ASSERT_TRUE(signaled);

    // Check that the result was stored
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));
    TEST_ASSERT_FALSE(pending_result_is_timed_out(pending));

    QueryResult* retrieved = pending_result_get(pending);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(2, retrieved->row_count);
    TEST_ASSERT_EQUAL(2, retrieved->column_count);
    // Note: data_json is NULL in the mock result, so we don't test it

    pending_result_manager_destroy(manager, NULL);
}

// Test signaling result ready for non-existent query
void test_pending_result_signal_ready_not_found(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
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
    bool signaled = pending_result_signal_ready(manager, "non_existent", mock_result, NULL);
    TEST_ASSERT_FALSE(signaled);

    // Clean up the result since it wasn't claimed
    free(mock_result);

    pending_result_manager_destroy(manager, NULL);
}

// Test cleanup of expired results
void test_pending_result_cleanup_expired(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result with short timeout
    PendingQueryResult* pending = pending_result_register(manager, "expired_test", 1, NULL);
    TEST_ASSERT_NOT_NULL(pending);

    // Manually set submitted time to past
    pending->submitted_at = time(NULL) - 5; // 5 seconds ago

    // Run cleanup
    size_t cleaned = pending_result_cleanup_expired(manager, NULL);
    TEST_ASSERT_EQUAL(1, cleaned);
    TEST_ASSERT_EQUAL(0, manager->count);

    pending_result_manager_destroy(manager, NULL);
}

// Test that non-expired results are not cleaned up
void test_pending_result_cleanup_not_expired(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    // Register a pending result with long timeout
    PendingQueryResult* pending = pending_result_register(manager, "not_expired", 300, NULL);
    TEST_ASSERT_NOT_NULL(pending);

    // Run cleanup
    size_t cleaned = pending_result_cleanup_expired(manager, NULL);
    TEST_ASSERT_EQUAL(0, cleaned);
    TEST_ASSERT_EQUAL(1, manager->count);

    pending_result_manager_destroy(manager, NULL);
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
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    size_t initial_capacity = manager->capacity;

    // Fill to capacity
    for (size_t i = 0; i < initial_capacity; i++) {
        char query_id[32];
        snprintf(query_id, sizeof(query_id), "query_%zu", i);
        PendingQueryResult* pending = pending_result_register(manager, query_id, 30, NULL);
        TEST_ASSERT_NOT_NULL(pending);
    }

    TEST_ASSERT_EQUAL(initial_capacity, manager->count);

    // Add one more to trigger expansion
    PendingQueryResult* extra_pending = pending_result_register(manager, "extra_query", 30, NULL);
    TEST_ASSERT_NOT_NULL(extra_pending);

    TEST_ASSERT_TRUE(manager->capacity > initial_capacity);
    TEST_ASSERT_EQUAL(initial_capacity + 1, manager->count);

    pending_result_manager_destroy(manager, NULL);
}

// Test NULL parameter handling
void test_pending_result_null_parameters(void) {
    // Test manager creation with NULL
    pending_result_manager_destroy(NULL, NULL); // Should not crash

    // Test registration with NULL parameters
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending1 = pending_result_register(NULL, "test", 30, NULL);
    TEST_ASSERT_NULL(pending1);

    PendingQueryResult* pending2 = pending_result_register(manager, NULL, 30, NULL);
    TEST_ASSERT_NULL(pending2);

    pending_result_manager_destroy(manager, NULL);

    // Test signaling with NULL parameters
    bool signaled1 = pending_result_signal_ready(NULL, "test", NULL, NULL);
    TEST_ASSERT_FALSE(signaled1);

    bool signaled2 = pending_result_signal_ready(manager, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(signaled2);
}

// Test result state checks
void test_pending_result_state_checks(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "state_test", 30, NULL);
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

    bool signaled = pending_result_signal_ready(manager, "state_test", mock_result, NULL);
    TEST_ASSERT_TRUE(signaled);

    // Now should be completed
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));
    TEST_ASSERT_FALSE(pending_result_is_timed_out(pending));

    pending_result_manager_destroy(manager, NULL);
}

// Test manager creation with malloc failure
void test_pending_result_manager_create_malloc_failure(void) {
    mock_system_set_malloc_failure(1);
    
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NULL(manager);
    
    mock_system_reset_all();
}

// Test manager creation with calloc failure
void test_pending_result_manager_create_calloc_failure(void) {
    mock_system_set_calloc_failure(1);
    
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NULL(manager);
    
    mock_system_reset_all();
}

// Test manager creation with NULL parameter (can't easily mock pthread functions)
void test_pending_result_manager_create_null_parameter(void) {
    // Test that the function handles NULL parameters gracefully
    // Since we can't easily mock pthread functions, we test parameter validation
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager); // Should succeed with NULL dqm_label

    pending_result_manager_destroy(manager, NULL);
}

// Test pending result registration with malloc failure
void test_pending_result_register_malloc_failure(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    mock_system_set_malloc_failure(1);
    PendingQueryResult* pending = pending_result_register(manager, "test_query", 30, NULL);
    TEST_ASSERT_NULL(pending);
    
    pending_result_manager_destroy(manager, NULL);
    mock_system_reset_all();
}

// Test pending result registration with strdup failure
void test_pending_result_register_strdup_failure(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    mock_system_set_malloc_failure(2); // Fail on second malloc (strdup)
    PendingQueryResult* pending = pending_result_register(manager, "test_query", 30, NULL);
    TEST_ASSERT_NULL(pending);
    
    pending_result_manager_destroy(manager, NULL);
    mock_system_reset_all();
}

// Test pending result registration with NULL manager
void test_pending_result_register_null_manager(void) {
    // Test that registration fails gracefully with NULL manager
    PendingQueryResult* pending = pending_result_register(NULL, "test_query", 30, NULL);
    TEST_ASSERT_NULL(pending);
}

// Test pending result registration with NULL query_id
void test_pending_result_register_null_query_id(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    // Test that registration fails gracefully with NULL query_id
    PendingQueryResult* pending = pending_result_register(manager, NULL, 30, NULL);
    TEST_ASSERT_NULL(pending);

    pending_result_manager_destroy(manager, NULL);
}

// Test pending result registration with realloc failure
void test_pending_result_register_realloc_failure(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    // Fill manager to capacity
    size_t initial_capacity = manager->capacity;
    for (size_t i = 0; i < initial_capacity; i++) {
        char query_id[32];
        snprintf(query_id, sizeof(query_id), "query_%zu", i);
        PendingQueryResult* pending = pending_result_register(manager, query_id, 30, NULL);
        TEST_ASSERT_NOT_NULL(pending);
    }
    
    // Now cause realloc to fail
    mock_system_set_realloc_failure(1);
    PendingQueryResult* pending = pending_result_register(manager, "extra_query", 30, NULL);
    TEST_ASSERT_NULL(pending);
    
    pending_result_manager_destroy(manager, NULL);
    mock_system_reset_all();
}

// Test pending result wait with timeout
void test_pending_result_wait_timeout(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "test_query", 0, NULL); // 0 timeout for instant timeout
    TEST_ASSERT_NOT_NULL(pending);

    int rc = pending_result_wait(pending, NULL);
    TEST_ASSERT_EQUAL(-1, rc); // Should timeout immediately

    pending_result_manager_destroy(manager, NULL);
}

// Test pending result wait multiple
void test_pending_result_wait_multiple(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    PendingQueryResult* pending1 = pending_result_register(manager, "query1", 0, NULL);
    PendingQueryResult* pending2 = pending_result_register(manager, "query2", 0, NULL);
    PendingQueryResult* pending3 = pending_result_register(manager, "query3", 0, NULL);
    TEST_ASSERT_NOT_NULL(pending1);
    TEST_ASSERT_NOT_NULL(pending2);
    TEST_ASSERT_NOT_NULL(pending3);
    
    PendingQueryResult* pendings[] = {pending1, pending2, pending3};
    int rc = pending_result_wait_multiple(pendings, 3, 0, NULL); // 0 timeout
    TEST_ASSERT_EQUAL(-1, rc);
    
    pending_result_manager_destroy(manager, NULL);
}

// Test pending result wait multiple with null parameters
void test_pending_result_wait_multiple_null_parameters(void) {
    int rc = pending_result_wait_multiple(NULL, 0, 0, NULL); // 0 timeout
    TEST_ASSERT_EQUAL(-1, rc);
}

// Test pending result wait multiple with timeout
void test_pending_result_wait_multiple_timeout(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    PendingQueryResult* pending = pending_result_register(manager, "query1", 0, NULL); // 0 timeout
    TEST_ASSERT_NOT_NULL(pending);
    
    PendingQueryResult* pendings[] = {pending};
    int rc = pending_result_wait_multiple(pendings, 1, 0, NULL); // 0 timeout
    TEST_ASSERT_EQUAL(-1, rc);
    
    pending_result_manager_destroy(manager, NULL);
}

// Test pending result cleanup expired with result
void test_pending_result_cleanup_expired_with_result(void) {
    PendingResultManager* manager = pending_result_manager_create(NULL);
    TEST_ASSERT_NOT_NULL(manager);
    
    PendingQueryResult* pending = pending_result_register(manager, "expired_test", 1, NULL);
    TEST_ASSERT_NOT_NULL(pending);
    
    QueryResult* mock_result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(mock_result);
    mock_result->success = true;
    mock_result->data_json = strdup("{\"test\": \"data\"}");
    pending->result = mock_result;
    
    pending->submitted_at = time(NULL) - 5;
    
    size_t cleaned = pending_result_cleanup_expired(manager, NULL);
    TEST_ASSERT_EQUAL(1, cleaned);
    TEST_ASSERT_EQUAL(0, manager->count);
    
    pending_result_manager_destroy(manager, NULL);
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
    RUN_TEST(test_pending_result_manager_create_malloc_failure);
    RUN_TEST(test_pending_result_manager_create_calloc_failure);
    RUN_TEST(test_pending_result_manager_create_null_parameter);
    RUN_TEST(test_pending_result_register_malloc_failure);
    RUN_TEST(test_pending_result_register_strdup_failure);
    RUN_TEST(test_pending_result_register_null_manager);
    RUN_TEST(test_pending_result_register_null_query_id);
    RUN_TEST(test_pending_result_register_realloc_failure);
    RUN_TEST(test_pending_result_wait_timeout);
    RUN_TEST(test_pending_result_wait_multiple);
    RUN_TEST(test_pending_result_wait_multiple_null_parameters);
    RUN_TEST(test_pending_result_wait_multiple_timeout);
    RUN_TEST(test_pending_result_cleanup_expired_with_result);

    return UNITY_END();
}