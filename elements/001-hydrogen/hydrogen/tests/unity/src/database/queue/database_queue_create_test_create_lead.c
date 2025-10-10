/*
 * Unity Test File: database_queue_create_test_create_lead
 * This file contains unit tests for database_queue_create_lead function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_create_lead_valid_parameters(void);
void test_database_queue_create_lead_with_bootstrap_query(void);
void test_database_queue_create_lead_null_database_name(void);
void test_database_queue_create_lead_null_connection_string(void);
void test_database_queue_create_lead_empty_database_name(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_create_lead_valid_parameters(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_TRUE(queue->is_lead_queue);
    TEST_ASSERT_TRUE(queue->can_spawn_queues);
    TEST_ASSERT_EQUAL_STRING("testdb", queue->database_name);
    TEST_ASSERT_EQUAL_STRING("Lead", queue->queue_type);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);
    TEST_ASSERT_EQUAL(0, queue->queue_number);
    database_queue_destroy(queue);
}

void test_database_queue_create_lead_with_bootstrap_query(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb2", "sqlite:///tmp/test2.db", "CREATE TABLE test");
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("CREATE TABLE test", queue->bootstrap_query);
    database_queue_destroy(queue);
}

void test_database_queue_create_lead_null_database_name(void) {
    DatabaseQueue* queue = database_queue_create_lead(NULL, "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NULL(queue);
}

void test_database_queue_create_lead_null_connection_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", NULL, NULL);
    TEST_ASSERT_NULL(queue);
}

void test_database_queue_create_lead_empty_database_name(void) {
    DatabaseQueue* queue = database_queue_create_lead("", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NULL(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_create_lead_valid_parameters);
    RUN_TEST(test_database_queue_create_lead_with_bootstrap_query);
    RUN_TEST(test_database_queue_create_lead_null_database_name);
    RUN_TEST(test_database_queue_create_lead_null_connection_string);
    RUN_TEST(test_database_queue_create_lead_empty_database_name);

    return UNITY_END();
}