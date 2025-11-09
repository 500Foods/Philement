/*
 * Unity Test File: lead_test_shutdown_queue_flow
 * Tests the complete flow of spawning and shutting down child queues
 * to cover lines 590-668 in database_queue_shutdown_child_queue
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>

// Forward declarations
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type);
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type);

// Test function prototypes
void test_shutdown_child_queue_after_spawn(void);
void test_shutdown_child_queue_not_found(void);
void test_shutdown_child_queue_with_null_queue_type_in_child(void);

// Helper to create a child queue manually (simulating what spawn does)
static DatabaseQueue* create_mock_child_queue(const char* queue_type, int queue_number) {
    DatabaseQueue* child = calloc(1, sizeof(DatabaseQueue));
    if (!child) return NULL;

    child->database_name = strdup("testdb");
    child->is_lead_queue = false;
    child->queue_type = strdup(queue_type);
    child->queue_number = queue_number;
    child->worker_thread_started = false; // Not started - easier to test
    
    pthread_mutex_init(&child->children_lock, NULL);
    
    return child;
}

// Helper to create mock lead queue
static DatabaseQueue* create_shutdown_test_lead_queue(void) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup("testdb");
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->queue_number = 0;
    queue->max_child_queues = 10;
    queue->child_queue_count = 0;
    queue->child_queues = calloc((size_t)queue->max_child_queues, sizeof(DatabaseQueue*));

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->children_lock, NULL);

    return queue;
}

// Helper to destroy mock queue
static void destroy_shutdown_test_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    // Clean up any remaining children
    for (int i = 0; i < queue->child_queue_count; i++) {
        if (queue->child_queues[i]) {
            free(queue->child_queues[i]->database_name);
            free(queue->child_queues[i]->queue_type);
            pthread_mutex_destroy(&queue->child_queues[i]->children_lock);
            free(queue->child_queues[i]);
        }
    }

    free(queue->database_name);
    free(queue->queue_type);
    free(queue->child_queues);

    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->children_lock);

    free(queue);
}

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test the full shutdown flow: spawn a child then shut it down
void test_shutdown_child_queue_after_spawn(void) {
    DatabaseQueue* lead_queue = create_shutdown_test_lead_queue();
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Manually add a child queue (simpler than full spawn which needs threads)
    DatabaseQueue* child = create_mock_child_queue(QUEUE_TYPE_FAST, 1);
    TEST_ASSERT_NOT_NULL(child);
    
    lead_queue->child_queues[0] = child;
    lead_queue->child_queue_count = 1;

    // Now test shutdown - this should exercise lines 590-668
    bool result = database_queue_shutdown_child_queue(lead_queue, QUEUE_TYPE_FAST);
    TEST_ASSERT_TRUE(result);
    
    // Verify the child was removed
    TEST_ASSERT_EQUAL(0, lead_queue->child_queue_count);
    TEST_ASSERT_NULL(lead_queue->child_queues[0]);

    destroy_shutdown_test_lead_queue(lead_queue);
}

// Test shutdown when the queue type is not found (lines 646-650)
void test_shutdown_child_queue_not_found(void) {
    DatabaseQueue* lead_queue = create_shutdown_test_lead_queue();
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Add a FAST queue
    DatabaseQueue* child = create_mock_child_queue(QUEUE_TYPE_FAST, 1);
    TEST_ASSERT_NOT_NULL(child);
    
    lead_queue->child_queues[0] = child;
    lead_queue->child_queue_count = 1;

    // Try to shutdown a SLOW queue (not present) - exercises lines 637-650
    bool result = database_queue_shutdown_child_queue(lead_queue, QUEUE_TYPE_SLOW);
    TEST_ASSERT_FALSE(result);
    
    // Original child should still be there
    TEST_ASSERT_EQUAL(1, lead_queue->child_queue_count);

    destroy_shutdown_test_lead_queue(lead_queue);
}

// Test shutdown with child that has NULL queue_type (defensive check on line 639)
void test_shutdown_child_queue_with_null_queue_type_in_child(void) {
    DatabaseQueue* lead_queue = create_shutdown_test_lead_queue();
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Add a child with NULL queue_type
    DatabaseQueue* child = create_mock_child_queue(QUEUE_TYPE_FAST, 1);
    TEST_ASSERT_NOT_NULL(child);
    
    // Make queue_type NULL to test line 639
    free(child->queue_type);
    child->queue_type = NULL;
    
    lead_queue->child_queues[0] = child;
    lead_queue->child_queue_count = 1;

    // Try to shutdown - should handle NULL gracefully
    bool result = database_queue_shutdown_child_queue(lead_queue, QUEUE_TYPE_FAST);
    TEST_ASSERT_FALSE(result); // Should not find it due to NULL type
    
    // Child should still be there
    TEST_ASSERT_EQUAL(1, lead_queue->child_queue_count);

    destroy_shutdown_test_lead_queue(lead_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_child_queue_after_spawn);
    RUN_TEST(test_shutdown_child_queue_not_found);
    RUN_TEST(test_shutdown_child_queue_with_null_queue_type_in_child);

    return UNITY_END();
}