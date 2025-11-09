/*
 * Unity Test File: lead_test_release_migration_connection
 * This file contains unit tests for database_queue_lead_release_migration_connection function
 * focusing on improving code coverage for uncovered lines.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/utils/utils_time.h>

// Forward declarations for functions being tested
void database_queue_lead_release_migration_connection(DatabaseQueue* lead_queue);

// Test function prototypes
void test_database_queue_lead_release_migration_connection_basic(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(const char* db_name) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup(db_name);
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->can_spawn_queues = true;
    queue->max_child_queues = 10;
    queue->child_queue_count = 0;
    queue->child_queues = calloc((size_t)queue->max_child_queues, sizeof(DatabaseQueue*));

    // Initialize connection lock mutex
    pthread_mutex_init(&queue->connection_lock, NULL);

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    free(queue->connection_string);
    free(queue->child_queues);

    // Clean up connection lock
    pthread_mutex_destroy(&queue->connection_lock);

    free(queue);
}

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test database_queue_lead_release_migration_connection basic functionality
void test_database_queue_lead_release_migration_connection_basic(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    // First acquire the lock to test releasing it
    if (pthread_mutex_lock(&queue->connection_lock) != 0) {
        TEST_FAIL_MESSAGE("Failed to lock mutex for test setup");
    }

    // Now release it through the function
    database_queue_lead_release_migration_connection(queue);

    // Verify the mutex is unlocked by trying to lock it again
    if (pthread_mutex_trylock(&queue->connection_lock) != 0) {
        TEST_FAIL_MESSAGE("Mutex was not properly unlocked");
    }

    // Unlock it again for cleanup
    pthread_mutex_unlock(&queue->connection_lock);

    destroy_mock_lead_queue(queue);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_release_migration_connection_basic);

    return UNITY_END();
}