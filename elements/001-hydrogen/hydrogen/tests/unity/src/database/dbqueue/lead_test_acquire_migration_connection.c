/*
 * Unity Test File: lead_test_acquire_migration_connection
 * This file contains unit tests for database_queue_lead_acquire_migration_connection function
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
bool database_queue_lead_acquire_migration_connection(DatabaseQueue* lead_queue, char* dqm_label);

// Test function prototypes
void test_database_queue_lead_acquire_migration_connection_lock_failure(void);
void test_database_queue_lead_acquire_migration_connection_no_persistent_connection(void);
void test_database_queue_lead_acquire_migration_connection_success(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(const char* db_name, bool has_persistent_connection) {
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

    if (has_persistent_connection) {
        // Create a mock persistent connection
        queue->persistent_connection = calloc(1, sizeof(DatabaseHandle));
        if (queue->persistent_connection) {
            queue->persistent_connection->engine_type = DB_ENGINE_SQLITE;
            pthread_mutex_init(&queue->persistent_connection->connection_lock, NULL);
        }
    }

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    free(queue->connection_string);
    free(queue->child_queues);

    // Clean up persistent connection
    if (queue->persistent_connection) {
        pthread_mutex_destroy(&queue->persistent_connection->connection_lock);
        free(queue->persistent_connection);
    }

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

// Test database_queue_lead_acquire_migration_connection with lock failure
void test_database_queue_lead_acquire_migration_connection_lock_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb", true);
    const char* dqm_label = "test_label";

    // Lock the mutex first to simulate lock failure
    if (pthread_mutex_lock(&queue->connection_lock) != 0) {
        TEST_FAIL_MESSAGE("Failed to lock mutex for test setup");
    }

    bool result = database_queue_lead_acquire_migration_connection(queue, (char*)dqm_label);
    TEST_ASSERT_FALSE(result);

    // Unlock for cleanup
    pthread_mutex_unlock(&queue->connection_lock);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_acquire_migration_connection with no persistent connection
void test_database_queue_lead_acquire_migration_connection_no_persistent_connection(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb", false); // No persistent connection
    const char* dqm_label = "test_label";

    bool result = database_queue_lead_acquire_migration_connection(queue, (char*)dqm_label);
    TEST_ASSERT_FALSE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_acquire_migration_connection success
void test_database_queue_lead_acquire_migration_connection_success(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb", true);
    const char* dqm_label = "test_label";

    bool result = database_queue_lead_acquire_migration_connection(queue, (char*)dqm_label);
    TEST_ASSERT_TRUE(result);

    // Release the lock that was acquired
    database_queue_lead_release_migration_connection(queue);

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_acquire_migration_connection_lock_failure);
    RUN_TEST(test_database_queue_lead_acquire_migration_connection_no_persistent_connection);
    RUN_TEST(test_database_queue_lead_acquire_migration_connection_success);

    return UNITY_END();
}