/*
 * Unity Test File: database_queue_create_lead_test
 * This file contains unit tests for database_queue_create_lead functions
 * focusing on error paths and edge cases to improve code coverage.
 */

#include <src/hydrogen.h>
#include "unity.h"

// Enable mocks for testing failure scenarios
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include source headers after mocks
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes - Core Infrastructure Functions
void test_database_queue_allocate_basic_null_database_name(void);
void test_database_queue_allocate_basic_null_connection_string(void);
void test_database_queue_allocate_basic_valid_parameters(void);
void test_database_queue_init_lead_sync_primitives_null_queue(void);
void test_database_queue_init_lead_sync_primitives_null_database_name(void);
void test_database_queue_init_lead_sync_primitives_valid_parameters(void);

// Tests for new synchronization primitive functions
void test_database_queue_init_basic_sync_primitives_null_queue(void);
void test_database_queue_init_basic_sync_primitives_valid_queue(void);
void test_database_queue_init_children_management_null_queue(void);
void test_database_queue_init_children_management_valid_queue(void);
void test_database_queue_init_connection_sync_null_queue(void);
void test_database_queue_init_connection_sync_valid_queue(void);
void test_database_queue_init_bootstrap_sync_null_queue(void);
void test_database_queue_init_bootstrap_sync_valid_queue(void);
void test_database_queue_init_initial_connection_sync_null_queue(void);
void test_database_queue_init_initial_connection_sync_valid_queue(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Reset all mocks to default state
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures
    mock_system_reset_all();
}



// Tests for individual helper functions to improve coverage

// Test database_queue_allocate_basic with NULL database_name
void test_database_queue_allocate_basic_null_database_name(void) {
    DatabaseQueue* result = database_queue_allocate_basic(NULL, "test_conn", NULL);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_allocate_basic with NULL connection_string
void test_database_queue_allocate_basic_null_connection_string(void) {
    DatabaseQueue* result = database_queue_allocate_basic("test_db", NULL, NULL);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_allocate_basic with valid parameters
void test_database_queue_allocate_basic_valid_parameters(void) {
    DatabaseQueue* result = database_queue_allocate_basic("test_db", "test_conn", "test_query");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_db", result->database_name);
    TEST_ASSERT_EQUAL_STRING("test_conn", result->connection_string);
    TEST_ASSERT_EQUAL_STRING("test_query", result->bootstrap_query);
    // Clean up
    free(result->database_name);
    free(result->connection_string);
    free(result->bootstrap_query);
    free(result);
}


// Tests for new synchronization primitive functions

// Test database_queue_init_basic_sync_primitives with NULL queue
void test_database_queue_init_basic_sync_primitives_null_queue(void) {
    bool result = database_queue_init_basic_sync_primitives(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_basic_sync_primitives with valid queue
void test_database_queue_init_basic_sync_primitives_valid_queue(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_init_basic_sync_primitives(queue);
    TEST_ASSERT_TRUE(result);

    // Clean up - need to destroy the primitives we just created
    sem_destroy(&queue->worker_semaphore);
    pthread_mutex_destroy(&queue->queue_access_lock);
    database_queue_destroy(queue);
}

// Test database_queue_init_children_management with NULL queue
void test_database_queue_init_children_management_null_queue(void) {
    bool result = database_queue_init_children_management(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_children_management with valid queue
void test_database_queue_init_children_management_valid_queue(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_init_children_management(queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(queue->child_queues);
    TEST_ASSERT_EQUAL(20, queue->max_child_queues);

    // Clean up
    free(queue->child_queues);
    pthread_mutex_destroy(&queue->children_lock);
    database_queue_destroy(queue);
}

// Test database_queue_init_connection_sync with NULL queue
void test_database_queue_init_connection_sync_null_queue(void) {
    bool result = database_queue_init_connection_sync(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_connection_sync with valid queue
void test_database_queue_init_connection_sync_valid_queue(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_init_connection_sync(queue);
    TEST_ASSERT_TRUE(result);

    // Clean up
    pthread_mutex_destroy(&queue->connection_lock);
    database_queue_destroy(queue);
}

// Test database_queue_init_bootstrap_sync with NULL queue
void test_database_queue_init_bootstrap_sync_null_queue(void) {
    bool result = database_queue_init_bootstrap_sync(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_bootstrap_sync with valid queue
void test_database_queue_init_bootstrap_sync_valid_queue(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_init_bootstrap_sync(queue);
    TEST_ASSERT_TRUE(result);

    // Clean up
    pthread_cond_destroy(&queue->bootstrap_cond);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    database_queue_destroy(queue);
}

// Test database_queue_init_initial_connection_sync with NULL queue
void test_database_queue_init_initial_connection_sync_null_queue(void) {
    bool result = database_queue_init_initial_connection_sync(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_initial_connection_sync with valid queue
void test_database_queue_init_initial_connection_sync_valid_queue(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_init_initial_connection_sync(queue);
    TEST_ASSERT_TRUE(result);

    // Clean up
    pthread_cond_destroy(&queue->initial_connection_cond);
    pthread_mutex_destroy(&queue->initial_connection_lock);
    database_queue_destroy(queue);
}


// Test database_queue_init_lead_sync_primitives with NULL queue
void test_database_queue_init_lead_sync_primitives_null_queue(void) {
    bool result = database_queue_init_lead_sync_primitives(NULL, "test_db");
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_lead_sync_primitives with NULL database_name
void test_database_queue_init_lead_sync_primitives_null_database_name(void) {
    DatabaseQueue queue;
    memset(&queue, 0, sizeof(DatabaseQueue));
    bool result = database_queue_init_lead_sync_primitives(&queue, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_lead_sync_primitives with valid parameters
void test_database_queue_init_lead_sync_primitives_valid_parameters(void) {
    DatabaseQueue* queue = database_queue_allocate_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Initialize properties first
    TEST_ASSERT_TRUE(database_queue_init_lead_properties(queue));

    bool result = database_queue_init_lead_sync_primitives(queue, "test_db");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(queue->child_queues);

    // Clean up
    database_queue_destroy(queue);
}


int main(void) {
    UNITY_BEGIN();

    // Tests for individual helper functions to improve coverage
    RUN_TEST(test_database_queue_allocate_basic_null_database_name);
    RUN_TEST(test_database_queue_allocate_basic_null_connection_string);
    RUN_TEST(test_database_queue_allocate_basic_valid_parameters);
    RUN_TEST(test_database_queue_init_lead_sync_primitives_null_queue);
    RUN_TEST(test_database_queue_init_lead_sync_primitives_null_database_name);
    RUN_TEST(test_database_queue_init_lead_sync_primitives_valid_parameters);

    // Tests for new synchronization primitive functions
    RUN_TEST(test_database_queue_init_basic_sync_primitives_null_queue);
    RUN_TEST(test_database_queue_init_basic_sync_primitives_valid_queue);
    RUN_TEST(test_database_queue_init_children_management_null_queue);
    RUN_TEST(test_database_queue_init_children_management_valid_queue);
    RUN_TEST(test_database_queue_init_connection_sync_null_queue);
    RUN_TEST(test_database_queue_init_connection_sync_valid_queue);
    RUN_TEST(test_database_queue_init_bootstrap_sync_null_queue);
    RUN_TEST(test_database_queue_init_bootstrap_sync_valid_queue);
    RUN_TEST(test_database_queue_init_initial_connection_sync_null_queue);
    RUN_TEST(test_database_queue_init_initial_connection_sync_valid_queue);

    return UNITY_END();
}