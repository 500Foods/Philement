/*
 * Unity Test File: database_queue_safe
 * This file contains unit tests for database_queue functions that are safe to test
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Function prototypes for test functions
void test_database_queue_safe_functions(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Safe test that only tests functions that don't involve threading or complex state
void test_database_queue_safe_functions(void) {
    // Test database_queue_type_to_string - these are completely safe
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

    // Test database_queue_type_from_string - completely safe
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

    // Test database_queue_select_type - completely safe
    DatabaseQueueType selected_type = database_queue_select_type(QUEUE_TYPE_SLOW);
    TEST_ASSERT_EQUAL(DB_QUEUE_SLOW, selected_type);

    selected_type = database_queue_select_type(NULL);
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, selected_type); // Default for NULL

    selected_type = database_queue_select_type("invalid");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, selected_type); // Default for invalid

    // Test database_queue_get_depth with NULL - safe
    size_t depth = database_queue_get_depth(NULL);
    TEST_ASSERT_EQUAL(0, depth); // Should return 0 for NULL

    // Test database_queue_get_stats with NULL - safe
    char buffer[256];
    database_queue_get_stats(NULL, buffer, sizeof(buffer));
    // Should not crash with NULL queue

    database_queue_get_stats(NULL, NULL, sizeof(buffer));
    // Should not crash with NULL buffer

    database_queue_get_stats(NULL, buffer, 0);
    // Should not crash with zero buffer size

    // Test database_queue_health_check with NULL - safe
    bool result = database_queue_health_check(NULL);
    TEST_ASSERT_FALSE(result); // Should return false for NULL

    // Test database_queue_submit_query with NULL - safe
    result = database_queue_submit_query(NULL, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue

    // Test database_queue_process_next with NULL - safe
    DatabaseQuery* processed_query = database_queue_process_next(NULL);
    TEST_ASSERT_NULL(processed_query); // Should return NULL for NULL queue

    // Test database_queue_start_worker with NULL - safe
    result = database_queue_start_worker(NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL

    // Test database_queue_stop_worker with NULL - safe
    database_queue_stop_worker(NULL); // Should not crash with NULL

    // Test database_queue_spawn_child_queue with NULL - safe
    result = database_queue_spawn_child_queue(NULL, QUEUE_TYPE_FAST);
    TEST_ASSERT_FALSE(result); // Should fail with NULL lead queue

    result = database_queue_spawn_child_queue(NULL, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue type

    // Test database_queue_shutdown_child_queue with NULL - safe
    result = database_queue_shutdown_child_queue(NULL, QUEUE_TYPE_FAST);
    TEST_ASSERT_FALSE(result); // Should fail with NULL lead queue

    result = database_queue_shutdown_child_queue(NULL, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue type

    // Test database_queue_manage_child_queues with NULL - safe
    database_queue_manage_child_queues(NULL); // Should not crash with NULL

    // Test database_queue_manager_create - this should be safe
    DatabaseQueueManager* manager = database_queue_manager_create(4);
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_TRUE(manager->initialized);
    TEST_ASSERT_EQUAL(0, manager->database_count);
    TEST_ASSERT_EQUAL(4, manager->max_databases);

    // Test database_queue_manager_get_database with NULL manager - safe
    DatabaseQueue* found_queue = database_queue_manager_get_database(NULL, "test");
    TEST_ASSERT_NULL(found_queue); // Should return NULL for NULL manager

    found_queue = database_queue_manager_get_database(manager, NULL);
    TEST_ASSERT_NULL(found_queue); // Should return NULL for NULL name

    found_queue = database_queue_manager_get_database(manager, "nonexistent");
    TEST_ASSERT_NULL(found_queue); // Should return NULL for non-existent database

    // Test database_queue_manager_add_database with NULL - safe
    result = database_queue_manager_add_database(NULL, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL manager

    result = database_queue_manager_add_database(manager, NULL);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue

    // Clean up
    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_safe_functions);

    return UNITY_END();
}