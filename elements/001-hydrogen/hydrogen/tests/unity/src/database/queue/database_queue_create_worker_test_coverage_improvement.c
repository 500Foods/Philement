/*
 * Unity Test File: database_queue_create_worker_test
 * This file contains unit tests for database_queue_create_worker functions
 * focusing on error paths and edge cases to improve code coverage.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing failure scenarios
#define USE_MOCK_SYSTEM
#include <tests/unity/mocks/mock_system.h>

// Include source headers after mocks
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_allocate_worker_basic_null_database_name(void);
void test_database_queue_allocate_worker_basic_null_connection_string(void);
void test_database_queue_allocate_worker_basic_null_queue_type(void);
void test_database_queue_init_worker_properties_null_queue(void);
void test_database_queue_init_worker_properties_null_queue_type(void);
void test_database_queue_create_worker_underlying_queue_null_queue(void);
void test_database_queue_create_worker_underlying_queue_null_database_name(void);
void test_database_queue_create_worker_underlying_queue_null_queue_type(void);
void test_database_queue_init_worker_sync_primitives_null_queue(void);
void test_database_queue_init_worker_sync_primitives_null_queue_type(void);
void test_database_queue_init_worker_final_flags_null_queue(void);
void test_database_queue_create_worker_null_database_name(void);
void test_database_queue_create_worker_null_connection_string(void);
void test_database_queue_create_worker_null_queue_type(void);

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

// Test database_queue_allocate_worker_basic with NULL database_name
void test_database_queue_allocate_worker_basic_null_database_name(void) {
    DatabaseQueue* result = database_queue_allocate_worker_basic(NULL, "test_conn", "slow");
    TEST_ASSERT_NULL(result);
}

// Test database_queue_allocate_worker_basic with NULL connection_string
void test_database_queue_allocate_worker_basic_null_connection_string(void) {
    DatabaseQueue* result = database_queue_allocate_worker_basic("test_db", NULL, "slow");
    TEST_ASSERT_NULL(result);
}

// Test database_queue_allocate_worker_basic with NULL queue_type
void test_database_queue_allocate_worker_basic_null_queue_type(void) {
    DatabaseQueue* result = database_queue_allocate_worker_basic("test_db", "test_conn", NULL);
    TEST_ASSERT_NULL(result);
}


// Test database_queue_init_worker_properties with NULL queue
void test_database_queue_init_worker_properties_null_queue(void) {
    bool result = database_queue_init_worker_properties(NULL, "slow");
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_worker_properties with NULL queue_type
void test_database_queue_init_worker_properties_null_queue_type(void) {
    DatabaseQueue queue = {0};
    bool result = database_queue_init_worker_properties(&queue, NULL);
    TEST_ASSERT_FALSE(result);
}


// Test database_queue_create_worker_underlying_queue with NULL queue
void test_database_queue_create_worker_underlying_queue_null_queue(void) {
    bool result = database_queue_create_worker_underlying_queue(NULL, "test_db", "slow", NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_create_worker_underlying_queue with NULL database_name
void test_database_queue_create_worker_underlying_queue_null_database_name(void) {
    DatabaseQueue queue = {0};
    bool result = database_queue_create_worker_underlying_queue(&queue, NULL, "slow", NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_create_worker_underlying_queue with NULL queue_type
void test_database_queue_create_worker_underlying_queue_null_queue_type(void) {
    DatabaseQueue queue = {0};
    bool result = database_queue_create_worker_underlying_queue(&queue, "test_db", NULL, NULL);
    TEST_ASSERT_FALSE(result);
}


// Test database_queue_init_worker_sync_primitives with NULL queue
void test_database_queue_init_worker_sync_primitives_null_queue(void) {
    bool result = database_queue_init_worker_sync_primitives(NULL, "slow");
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_init_worker_sync_primitives with NULL queue_type
void test_database_queue_init_worker_sync_primitives_null_queue_type(void) {
    DatabaseQueue queue = {0};
    bool result = database_queue_init_worker_sync_primitives(&queue, NULL);
    TEST_ASSERT_FALSE(result);
}


// Test database_queue_init_worker_final_flags with NULL queue
void test_database_queue_init_worker_final_flags_null_queue(void) {
    database_queue_init_worker_final_flags(NULL);
    // Should not crash
}

// Test database_queue_create_worker with NULL database_name
void test_database_queue_create_worker_null_database_name(void) {
    DatabaseQueue* result = database_queue_create_worker(NULL, "test_conn", "slow", NULL);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_create_worker with NULL connection_string
void test_database_queue_create_worker_null_connection_string(void) {
    DatabaseQueue* result = database_queue_create_worker("test_db", NULL, "slow", NULL);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_create_worker with NULL queue_type
void test_database_queue_create_worker_null_queue_type(void) {
    DatabaseQueue* result = database_queue_create_worker("test_db", "test_conn", NULL, NULL);
    TEST_ASSERT_NULL(result);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_allocate_worker_basic_null_database_name);
    RUN_TEST(test_database_queue_allocate_worker_basic_null_connection_string);
    RUN_TEST(test_database_queue_allocate_worker_basic_null_queue_type);
    RUN_TEST(test_database_queue_init_worker_properties_null_queue);
    RUN_TEST(test_database_queue_init_worker_properties_null_queue_type);
    RUN_TEST(test_database_queue_create_worker_underlying_queue_null_queue);
    RUN_TEST(test_database_queue_create_worker_underlying_queue_null_database_name);
    RUN_TEST(test_database_queue_create_worker_underlying_queue_null_queue_type);
    RUN_TEST(test_database_queue_init_worker_sync_primitives_null_queue);
    RUN_TEST(test_database_queue_init_worker_sync_primitives_null_queue_type);
    RUN_TEST(test_database_queue_init_worker_final_flags_null_queue);
    RUN_TEST(test_database_queue_create_worker_null_database_name);
    RUN_TEST(test_database_queue_create_worker_null_connection_string);
    RUN_TEST(test_database_queue_create_worker_null_queue_type);

    return UNITY_END();
}