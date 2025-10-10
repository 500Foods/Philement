/*
 * Unity Test File: database_queue_submit_test_submit_query
 * This file contains unit tests for database_queue_submit_query function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_submit_query_null_queue(void);
void test_database_queue_submit_query_null_query(void);
void test_database_queue_submit_query_worker_queue(void);
void test_database_queue_submit_query_lead_queue_no_children(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures and add small delay for threading
    usleep(1000);  // 1ms delay
}

void test_database_queue_submit_query_null_queue(void) {
    DatabaseQuery query = {
        .query_id = strdup("test_query_1"),
        .query_template = strdup("SELECT 1"),
        .parameter_json = strdup("{}"),
        .queue_type_hint = DB_QUEUE_MEDIUM,
        .submitted_at = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    bool result = database_queue_submit_query(NULL, &query);
    TEST_ASSERT_FALSE(result);

    // Clean up
    free((char*)query.query_id);
    free((char*)query.query_template);
    free((char*)query.parameter_json);
}

void test_database_queue_submit_query_null_query(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb1", "sqlite:///tmp/test1.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_submit_query(queue, NULL);
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(queue);
}

void test_database_queue_submit_query_worker_queue(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    DatabaseQuery query = {
        .query_id = strdup("test_query_2"),
        .query_template = strdup("SELECT 1"),
        .parameter_json = strdup("{}"),
        .queue_type_hint = DB_QUEUE_MEDIUM,
        .submitted_at = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    bool result = database_queue_submit_query(queue, &query);
    TEST_ASSERT_TRUE(result);

    // Clean up
    free((char*)query.query_id);
    free((char*)query.query_template);
    free((char*)query.parameter_json);
    database_queue_destroy(queue);
}

void test_database_queue_submit_query_lead_queue_no_children(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb3", "sqlite:///tmp/test3.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    DatabaseQuery query = {
        .query_id = strdup("test_query_3"),
        .query_template = strdup("SELECT 1"),
        .parameter_json = strdup("{}"),
        .queue_type_hint = DB_QUEUE_MEDIUM,
        .submitted_at = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    bool result = database_queue_submit_query(queue, &query);
    TEST_ASSERT_TRUE(result);

    // Clean up
    free((char*)query.query_id);
    free((char*)query.query_template);
    free((char*)query.parameter_json);
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_submit_query_null_queue);
    RUN_TEST(test_database_queue_submit_query_null_query);
    RUN_TEST(test_database_queue_submit_query_worker_queue);
    RUN_TEST(test_database_queue_submit_query_lead_queue_no_children);

    return UNITY_END();
}