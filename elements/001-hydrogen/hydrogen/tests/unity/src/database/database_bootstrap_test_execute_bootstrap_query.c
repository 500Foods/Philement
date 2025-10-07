/*
 * Unity Test File: database_bootstrap_test_execute_bootstrap_query
 * This file contains unit tests for database_queue_execute_bootstrap_query function
 * from src/database/database_bootstrap.c
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database_bootstrap.h"
#include "../../../../src/database/database_queue.h"

// Function prototypes for test functions
void test_database_queue_execute_bootstrap_query_null_queue(void);
void test_database_queue_execute_bootstrap_query_non_lead_queue(void);
void test_database_queue_execute_bootstrap_query_lead_queue_no_connection(void);

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
void test_database_queue_execute_bootstrap_query_null_queue(void) {
    // Should not crash with NULL queue
    database_queue_execute_bootstrap_query(NULL);
    // Test passes if no crash occurs
}

// Test with non-lead queue
void test_database_queue_execute_bootstrap_query_non_lead_queue(void) {
    // Create a worker queue (non-lead)
    DatabaseQueue* worker_queue = database_queue_create_worker("testdb1",
        "postgresql://user:pass@host:5432/db", QUEUE_TYPE_MEDIUM);
    if (worker_queue) {
        // Should not crash with non-lead queue
        database_queue_execute_bootstrap_query(worker_queue);
        // Test passes if no crash occurs
        database_queue_destroy(worker_queue);
    }
}

// Test with lead queue but no connection
void test_database_queue_execute_bootstrap_query_lead_queue_no_connection(void) {
    // Create a lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb2",
        "postgresql://user:pass@host:5432/db", NULL);
    if (lead_queue) {
        // Ensure no persistent connection
        lead_queue->persistent_connection = NULL;
        lead_queue->is_connected = false;

        // Should attempt bootstrap but fail gracefully due to no connection
        database_queue_execute_bootstrap_query(lead_queue);
        // Test passes if no crash occurs

        database_queue_destroy(lead_queue);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_execute_bootstrap_query_null_queue);
    RUN_TEST(test_database_queue_execute_bootstrap_query_non_lead_queue);
    RUN_TEST(test_database_queue_execute_bootstrap_query_lead_queue_no_connection);

    return UNITY_END();
}