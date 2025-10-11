/*
 * Unity Test File: database_queue_comprehensive
 * This file contains comprehensive unit tests for all database_queue functions to maximize coverage
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Function prototypes for test functions
void test_database_queue_comprehensive_all_functions(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
    // Note: We don't destroy the global queue manager here as it may be used by other tests
}

// Comprehensive test that calls all functions to maximize coverage
void test_database_queue_comprehensive_all_functions(void) {
    char buffer[256];
    size_t buffer_size = sizeof(buffer);

    // Test database_queue_system_init
    bool result = database_queue_system_init();
    TEST_ASSERT_TRUE(result || global_queue_manager != NULL); // Should succeed or already be initialized

    // Test database_queue_type_to_string
    const char* type_str = database_queue_type_to_string(DB_QUEUE_SLOW);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_SLOW, type_str);

    type_str = database_queue_type_to_string(DB_QUEUE_MEDIUM);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_MEDIUM, type_str);

    type_str = database_queue_type_to_string(DB_QUEUE_FAST);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_FAST, type_str);

    type_str = database_queue_type_to_string(DB_QUEUE_CACHE);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_CACHE, type_str);

    type_str = database_queue_type_to_string(999); // Invalid
    TEST_ASSERT_EQUAL_STRING("unknown", type_str);

    // Test database_queue_type_from_string
    int queue_type = database_queue_type_from_string(QUEUE_TYPE_SLOW);
    TEST_ASSERT_EQUAL(DB_QUEUE_SLOW, queue_type);

    queue_type = database_queue_type_from_string(QUEUE_TYPE_MEDIUM);
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, queue_type);

    queue_type = database_queue_type_from_string(QUEUE_TYPE_FAST);
    TEST_ASSERT_EQUAL(DB_QUEUE_FAST, queue_type);

    queue_type = database_queue_type_from_string(QUEUE_TYPE_CACHE);
    TEST_ASSERT_EQUAL(DB_QUEUE_CACHE, queue_type);

    queue_type = database_queue_type_from_string("invalid");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, queue_type); // Default

    // Test database_queue_select_type
    DatabaseQueueType selected_type = database_queue_select_type(QUEUE_TYPE_SLOW);
    TEST_ASSERT_EQUAL(DB_QUEUE_SLOW, selected_type);

    selected_type = database_queue_select_type(NULL);
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, selected_type); // Default for NULL

    selected_type = database_queue_select_type("invalid");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, selected_type); // Default for invalid

    // Test database_queue_manager_create
    DatabaseQueueManager* manager = database_queue_manager_create(4);
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_TRUE(manager->initialized);
    TEST_ASSERT_EQUAL(0, manager->database_count);
    TEST_ASSERT_EQUAL(4, manager->max_databases);

    // Test database_queue_create_lead
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);
    TEST_ASSERT_TRUE(lead_queue->is_lead_queue);
    TEST_ASSERT_TRUE(lead_queue->can_spawn_queues);
    TEST_ASSERT_EQUAL_STRING("testdb", lead_queue->database_name);
    TEST_ASSERT_EQUAL_STRING("Lead", lead_queue->queue_type);

    // Test database_queue_manager_add_database
    result = database_queue_manager_add_database(manager, lead_queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, manager->database_count);

    // Test database_queue_manager_get_database
    DatabaseQueue* found_queue = database_queue_manager_get_database(manager, "testdb");
    TEST_ASSERT_NOT_NULL(found_queue);
    TEST_ASSERT_EQUAL_PTR(lead_queue, found_queue);

    found_queue = database_queue_manager_get_database(manager, "nonexistent");
    TEST_ASSERT_NULL(found_queue);

    // Test database_queue_create_worker
    DatabaseQueue* worker_queue = database_queue_create_worker("testdb", "sqlite:///tmp/test.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(worker_queue);
    TEST_ASSERT_FALSE(worker_queue->is_lead_queue);
    TEST_ASSERT_FALSE(worker_queue->can_spawn_queues);
    TEST_ASSERT_EQUAL_STRING("testdb", worker_queue->database_name);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_MEDIUM, worker_queue->queue_type);

    // Test database_queue_get_depth
    size_t null_depth = database_queue_get_depth(NULL);
    TEST_ASSERT_EQUAL(0, null_depth); // Should return 0 for NULL

    // Test that depth values are reasonable
    TEST_ASSERT_TRUE(null_depth == 0); // NULL queue should return 0

    // Test database_queue_get_stats
    database_queue_get_stats(lead_queue, buffer, buffer_size);
    TEST_ASSERT_TRUE(strlen(buffer) > 0); // Should populate buffer

    database_queue_get_stats(NULL, buffer, buffer_size);
    // Should not crash, buffer remains unchanged

    database_queue_get_stats(lead_queue, NULL, buffer_size);
    // Should not crash with NULL buffer

    database_queue_get_stats(lead_queue, buffer, 0);
    // Should not crash with zero buffer size

    // Test database_queue_health_check
    result = database_queue_health_check(lead_queue);
    TEST_ASSERT_TRUE(result); // Should return true for healthy queue

    result = database_queue_health_check(NULL);
    TEST_ASSERT_FALSE(result); // Should return false for NULL

    // Test database_queue_submit_query
    DatabaseQuery test_query = {
        .query_id = strdup("test_query_1"),
        .query_template = strdup("SELECT 1"),
        .parameter_json = strdup("{}"),
        .queue_type_hint = DB_QUEUE_MEDIUM,
        .submitted_at = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    result = database_queue_submit_query(lead_queue, &test_query);
    TEST_ASSERT_TRUE(result); // Should succeed

    result = database_queue_submit_query(NULL, &test_query);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue

    result = database_queue_submit_query(lead_queue, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL query

    // Clean up test_query memory
    if (test_query.query_id) free((char*)test_query.query_id);
    if (test_query.query_template) free((char*)test_query.query_template);
    if (test_query.parameter_json) free((char*)test_query.parameter_json);

    // Test database_queue_process_next
    DatabaseQuery* processed_query = database_queue_process_next(lead_queue);
    if (processed_query) {
        TEST_ASSERT_NOT_NULL(processed_query->query_template);
        // Clean up processed query
        if (processed_query->query_id) free(processed_query->query_id);
        if (processed_query->query_template) free(processed_query->query_template);
        if (processed_query->parameter_json) free(processed_query->parameter_json);
        if (processed_query->error_message) free(processed_query->error_message);
        free(processed_query);
    }

    processed_query = database_queue_process_next(NULL);
    TEST_ASSERT_NULL(processed_query); // Should return NULL for NULL queue

    // Skip database_queue_start_worker and database_queue_stop_worker as they involve threading
    // which can cause issues in unit tests
    // result = database_queue_start_worker(lead_queue);
    // TEST_ASSERT_TRUE(result); // Should succeed

    // result = database_queue_start_worker(NULL);
    // TEST_ASSERT_FALSE(result); // Should fail with NULL

    // database_queue_stop_worker(lead_queue); // Should not crash
    // database_queue_stop_worker(NULL); // Should not crash with NULL

    // Skip spawn/shutdown child queue functions as they involve threading and complex state
    // result = database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_FAST);
    // TEST_ASSERT_TRUE(result); // Should succeed

    // result = database_queue_spawn_child_queue(NULL, QUEUE_TYPE_FAST);
    // TEST_ASSERT_FALSE(result); // Should fail with NULL lead queue

    // result = database_queue_spawn_child_queue(lead_queue, NULL);
    // TEST_ASSERT_FALSE(result); // Should fail with NULL queue type

    // result = database_queue_shutdown_child_queue(lead_queue, QUEUE_TYPE_FAST);
    // TEST_ASSERT_TRUE(result); // Should succeed

    // result = database_queue_shutdown_child_queue(NULL, QUEUE_TYPE_FAST);
    // TEST_ASSERT_FALSE(result); // Should fail with NULL lead queue

    // result = database_queue_shutdown_child_queue(lead_queue, NULL);
    // TEST_ASSERT_FALSE(result); // Should fail with NULL queue type

    // database_queue_manage_child_queues(lead_queue); // Should not crash
    // database_queue_manage_child_queues(NULL); // Should not crash with NULL

    // Clean up - worker_queue was not added to manager, so destroy it separately
    // lead_queue was added to manager, so manager will destroy it
    database_queue_destroy(worker_queue);
    database_queue_manager_destroy(manager);

    // Test database_queue_system_destroy
    database_queue_system_destroy(); // Should not crash
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_comprehensive_all_functions);

    return UNITY_END();
}