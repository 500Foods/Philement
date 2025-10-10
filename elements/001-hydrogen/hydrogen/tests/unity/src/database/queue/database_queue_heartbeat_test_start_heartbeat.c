/*
 * Unity Test File: database_queue_heartbeat_test_start_heartbeat
 * This file contains unit tests for database_queue_start_heartbeat function
 * from src/database/database_queue_heartbeat.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Function prototypes for test functions
void test_database_queue_start_heartbeat_null_queue(void);
void test_database_queue_start_heartbeat_valid_queue(void);
void test_database_queue_start_heartbeat_connection_attempt(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test NULL queue parameter
void test_database_queue_start_heartbeat_null_queue(void) {
    // Should not crash with NULL queue
    database_queue_start_heartbeat(NULL);
    // Test passes if no crash occurs
}

// Test with valid queue
void test_database_queue_start_heartbeat_valid_queue(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb1",
        "postgresql://user:pass@host:5432/db", NULL);
    if (test_queue) {
        // Ensure initial state
        test_queue->is_connected = false;
        test_queue->persistent_connection = NULL;

        // Start heartbeat monitoring
        database_queue_start_heartbeat(test_queue);

        // Should update timestamps and attempt initial connection
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);
        TEST_ASSERT_TRUE(test_queue->last_connection_attempt > 0);

        database_queue_destroy(test_queue);
    }
}

// Test heartbeat start with connection attempt
void test_database_queue_start_heartbeat_connection_attempt(void) {
    // Create a test queue with different connection string
    DatabaseQueue* test_queue = database_queue_create_lead("testdb2",
        "mysql://user:pass@host:3306/db", NULL);
    if (test_queue) {
        // Ensure initial state
        test_queue->is_connected = false;
        test_queue->persistent_connection = NULL;

        // Start heartbeat monitoring
        database_queue_start_heartbeat(test_queue);

        // Should update timestamps
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);
        TEST_ASSERT_TRUE(test_queue->last_connection_attempt > 0);

        // Should have attempted connection (even if failed)
        TEST_ASSERT_TRUE(test_queue->last_connection_attempt > 0);

        database_queue_destroy(test_queue);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_start_heartbeat_null_queue);
    RUN_TEST(test_database_queue_start_heartbeat_valid_queue);
    RUN_TEST(test_database_queue_start_heartbeat_connection_attempt);

    return UNITY_END();
}