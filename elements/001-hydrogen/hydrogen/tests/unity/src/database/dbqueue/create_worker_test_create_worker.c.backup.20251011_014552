/*
 * Unity Test File: database_queue_create_test_create_worker
 * This file contains unit tests for database_queue_create_worker function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_create_worker_valid_parameters(void);
void test_database_queue_create_worker_null_database_name(void);
void test_database_queue_create_worker_null_connection_string(void);
void test_database_queue_create_worker_null_queue_type(void);
void test_database_queue_create_worker_different_queue_types(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_create_worker_valid_parameters(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb", "sqlite:///tmp/test.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_FALSE(queue->is_lead_queue);
    TEST_ASSERT_FALSE(queue->can_spawn_queues);
    TEST_ASSERT_EQUAL_STRING("testdb", queue->database_name);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_MEDIUM, queue->queue_type);
    TEST_ASSERT_EQUAL_STRING("M", queue->tags);
    TEST_ASSERT_EQUAL(-1, queue->queue_number);
    database_queue_destroy(queue);
}

void test_database_queue_create_worker_null_database_name(void) {
    DatabaseQueue* queue = database_queue_create_worker(NULL, "sqlite:///tmp/test.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NULL(queue);
}

void test_database_queue_create_worker_null_connection_string(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb", NULL, QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NULL(queue);
}

void test_database_queue_create_worker_null_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb", "sqlite:///tmp/test.db", NULL, NULL);
    TEST_ASSERT_NULL(queue);
}

void test_database_queue_create_worker_different_queue_types(void) {
    DatabaseQueue* slow_queue = database_queue_create_worker("testdb1", "sqlite:///tmp/test1.db", QUEUE_TYPE_SLOW, NULL);
    TEST_ASSERT_NOT_NULL(slow_queue);
    TEST_ASSERT_EQUAL_STRING("S", slow_queue->tags);
    database_queue_destroy(slow_queue);

    DatabaseQueue* fast_queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(fast_queue);
    TEST_ASSERT_EQUAL_STRING("F", fast_queue->tags);
    database_queue_destroy(fast_queue);

    DatabaseQueue* cache_queue = database_queue_create_worker("testdb3", "sqlite:///tmp/test3.db", QUEUE_TYPE_CACHE, NULL);
    TEST_ASSERT_NOT_NULL(cache_queue);
    TEST_ASSERT_EQUAL_STRING("C", cache_queue->tags);
    database_queue_destroy(cache_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_create_worker_valid_parameters);
    RUN_TEST(test_database_queue_create_worker_null_database_name);
    RUN_TEST(test_database_queue_create_worker_null_connection_string);
    RUN_TEST(test_database_queue_create_worker_null_queue_type);
    RUN_TEST(test_database_queue_create_worker_different_queue_types);

    return UNITY_END();
}