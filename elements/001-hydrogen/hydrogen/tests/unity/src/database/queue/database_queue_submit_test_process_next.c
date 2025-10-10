/*
 * Unity Test File: database_queue_submit_test_process_next
 * This file contains unit tests for database_queue_process_next function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_process_next_null_queue(void);
void test_database_queue_process_next_empty_queue(void);
void test_database_queue_process_next_with_query(void);

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

void test_database_queue_process_next_null_queue(void) {
    DatabaseQuery* result = database_queue_process_next(NULL);
    TEST_ASSERT_NULL(result);
}

void test_database_queue_process_next_empty_queue(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb1", "sqlite:///tmp/test1.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    DatabaseQuery* result = database_queue_process_next(queue);
    TEST_ASSERT_NULL(result);

    database_queue_destroy(queue);
}

void test_database_queue_process_next_with_query(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb2", "sqlite:///tmp/test2.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Submit a query first
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

    bool submit_result = database_queue_submit_query(queue, &query);
    TEST_ASSERT_TRUE(submit_result);

    // Now process the next query
    DatabaseQuery* processed_query = database_queue_process_next(queue);
    TEST_ASSERT_NOT_NULL(processed_query);
    TEST_ASSERT_NOT_NULL(processed_query->query_template);

    // Clean up
    free((char*)query.query_id);
    free((char*)query.query_template);
    free((char*)query.parameter_json);

    if (processed_query->query_id) free(processed_query->query_id);
    if (processed_query->query_template) free(processed_query->query_template);
    if (processed_query->parameter_json) free(processed_query->parameter_json);
    if (processed_query->error_message) free(processed_query->error_message);
    free(processed_query);

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_process_next_null_queue);
    RUN_TEST(test_database_queue_process_next_empty_queue);
    RUN_TEST(test_database_queue_process_next_with_query);

    return UNITY_END();
}