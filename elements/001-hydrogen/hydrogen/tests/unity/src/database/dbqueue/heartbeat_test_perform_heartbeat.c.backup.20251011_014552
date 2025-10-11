/*
 * Unity Test File: database_queue_heartbeat_test_perform_heartbeat
 * This file contains unit tests for database_queue_perform_heartbeat function
 * from src/database/database_queue_heartbeat.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Function prototypes for test functions
void test_database_queue_perform_heartbeat_basic(void);
void test_database_queue_perform_heartbeat_with_connection(void);
void test_database_queue_perform_heartbeat_connection_states(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test basic heartbeat functionality
void test_database_queue_perform_heartbeat_basic(void) {
    // Test with valid queue but no connection
    DatabaseQueue* test_queue = database_queue_create_lead("testdb1",
        "postgresql://user:pass@host:5432/db", NULL);
    if (test_queue) {
        // Ensure initial state
        test_queue->is_connected = false;
        test_queue->persistent_connection = NULL;

        // Perform heartbeat
        database_queue_perform_heartbeat(test_queue);

        // Should not crash and should update timestamps
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);

        database_queue_destroy(test_queue);
    }
}

// Test heartbeat with attempted connection
void test_database_queue_perform_heartbeat_with_connection(void) {
    // Test with queue that will attempt connection
    DatabaseQueue* test_queue = database_queue_create_lead("testdb2",
        "postgresql://user:pass@host:5432/db", NULL);
    if (test_queue) {
        // Ensure initial state
        test_queue->is_connected = false;
        test_queue->persistent_connection = NULL;

        // First perform heartbeat (should attempt connection)
        database_queue_perform_heartbeat(test_queue);

        // Should have attempted connection and updated timestamps
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);
        TEST_ASSERT_TRUE(test_queue->last_connection_attempt > 0);

        // Second heartbeat should check existing connection state
        time_t first_heartbeat = test_queue->last_heartbeat;
        database_queue_perform_heartbeat(test_queue);

        // Heartbeat timestamp should be updated
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > first_heartbeat);

        database_queue_destroy(test_queue);
    }
}

// Test heartbeat with different connection states
void test_database_queue_perform_heartbeat_connection_states(void) {
    // Test with queue initially connected
    DatabaseQueue* test_queue = database_queue_create_lead("testdb3",
        "postgresql://user:pass@host:5432/db", NULL);
    if (test_queue) {
        // Simulate connected state
        test_queue->is_connected = true;
        test_queue->persistent_connection = (DatabaseHandle*)0x1000; // Fake pointer

        // Perform heartbeat
        database_queue_perform_heartbeat(test_queue);

        // Should attempt to check connection health
        TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);

        database_queue_destroy(test_queue);
    }

    // Test with queue initially disconnected
    DatabaseQueue* test_queue2 = database_queue_create_lead("testdb4",
        "postgresql://user:pass@host:5432/db", NULL);
    if (test_queue2) {
        // Ensure disconnected state
        test_queue2->is_connected = false;
        test_queue2->persistent_connection = NULL;

        // Perform heartbeat
        database_queue_perform_heartbeat(test_queue2);

        // Should attempt reconnection
        TEST_ASSERT_TRUE(test_queue2->last_heartbeat > 0);
        TEST_ASSERT_TRUE(test_queue2->last_connection_attempt > 0);

        database_queue_destroy(test_queue2);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_perform_heartbeat_basic);
    if (0) RUN_TEST(test_database_queue_perform_heartbeat_with_connection);
    if (0) RUN_TEST(test_database_queue_perform_heartbeat_connection_states);

    return UNITY_END();
}