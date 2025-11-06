/*
 * Unity Test File: lead_reverse_test_find_next_reverse_migration
 * This file contains unit tests for database_queue_find_next_reverse_migration_to_apply function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_cache.h>

// Forward declarations for functions being tested
long long database_queue_find_next_reverse_migration_to_apply(DatabaseQueue* lead_queue);

// Test function prototypes
void test_database_queue_find_next_reverse_migration_to_apply_null_queue(void);
void test_database_queue_find_next_reverse_migration_to_apply_no_cache(void);
void test_database_queue_find_next_reverse_migration_to_apply_zero_apply(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(const char* db_name) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup(db_name);
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->latest_applied_migration = 0;
    queue->query_cache = NULL;

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    if (queue->query_cache) {
        query_cache_destroy(queue->query_cache, "testdb");
    }
    free(queue);
}

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test database_queue_find_next_reverse_migration_to_apply with NULL queue
void test_database_queue_find_next_reverse_migration_to_apply_null_queue(void) {
    long long result = database_queue_find_next_reverse_migration_to_apply(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

// Test database_queue_find_next_reverse_migration_to_apply with no cache
void test_database_queue_find_next_reverse_migration_to_apply_no_cache(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Ensure no query cache
    queue->query_cache = NULL;
    
    long long result = database_queue_find_next_reverse_migration_to_apply(queue);
    TEST_ASSERT_EQUAL(0, result);
    
    destroy_mock_lead_queue(queue);
}

// Test database_queue_find_next_reverse_migration_to_apply with zero APPLY value
void test_database_queue_find_next_reverse_migration_to_apply_zero_apply(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create an empty query cache
    queue->query_cache = query_cache_create("testdb");
    queue->latest_applied_migration = 0;
    
    // No applied migrations, should return 0
    long long result = database_queue_find_next_reverse_migration_to_apply(queue);
    TEST_ASSERT_EQUAL(0, result);
    
    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_find_next_reverse_migration_to_apply_null_queue);
    RUN_TEST(test_database_queue_find_next_reverse_migration_to_apply_no_cache);
    RUN_TEST(test_database_queue_find_next_reverse_migration_to_apply_zero_apply);

    return UNITY_END();
}