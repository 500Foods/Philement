/*
 * Unity Test File: database_queue_select_test
 * This file contains unit tests for the database queue selection algorithm
 * from src/database/database_queue_select.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source headers
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_queue_select.h>

// Test function prototypes
void test_select_optimal_queue_null_parameters(void);
void test_select_optimal_queue_no_databases(void);
void test_select_optimal_queue_single_queue(void);
void test_select_optimal_queue_multiple_queues_same_depth(void);
void test_select_optimal_queue_different_depths(void);
void test_select_optimal_queue_with_queue_type_filter(void);
void test_select_optimal_queue_no_matching_database(void);
void test_select_optimal_queue_no_matching_queue_type(void);
void test_update_queue_last_request_time_null_queue(void);
void test_update_queue_last_request_time_valid_queue(void);

// Test setup and teardown
void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No teardown needed for these tests
}

// Test: NULL parameters should return NULL
void test_select_optimal_queue_null_parameters(void) {
    DatabaseQueue* result = select_optimal_queue(NULL, NULL, NULL);
    TEST_ASSERT_NULL(result);

    DatabaseQueueManager manager = {0};
    result = select_optimal_queue(NULL, "fast", &manager);
    TEST_ASSERT_NULL(result);

    result = select_optimal_queue("testdb", NULL, NULL);
    TEST_ASSERT_NULL(result);
}

// Test: Empty manager should return NULL
void test_select_optimal_queue_no_databases(void) {
    DatabaseQueueManager manager = {0};
    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_NULL(result);
}

// Test: Single queue should be selected
void test_select_optimal_queue_single_queue(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 1;
    manager.databases = calloc(1, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) { free(manager.databases); return; } // Handle allocation failure
    queue->database_name = strdup("testdb");
    if (!queue->database_name) { free(queue); free(manager.databases); return; }
    queue->queue_type = strdup("fast");
    if (!queue->queue_type) { free(queue->database_name); free(queue); free(manager.databases); return; }
    queue->current_queue_depth = 0;
    queue->last_request_time = 1000;

    manager.databases[0] = queue;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_EQUAL_PTR(queue, result);

    // Cleanup
    free(queue->database_name);
    free(queue->queue_type);
    free(queue);
    free(manager.databases);
}

// Test: Multiple queues with same depth should select earliest timestamp
void test_select_optimal_queue_multiple_queues_same_depth(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 2;
    manager.databases = calloc(2, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    // Create first queue (earlier timestamp)
    DatabaseQueue* queue1 = calloc(1, sizeof(DatabaseQueue));
    if (!queue1) { free(manager.databases); return; }
    queue1->database_name = strdup("testdb");
    if (!queue1->database_name) { free(queue1); free(manager.databases); return; }
    queue1->queue_type = strdup("fast");
    if (!queue1->queue_type) { free(queue1->database_name); free(queue1); free(manager.databases); return; }
    queue1->current_queue_depth = 1;
    queue1->last_request_time = 1000;

    // Create second queue (later timestamp)
    DatabaseQueue* queue2 = calloc(1, sizeof(DatabaseQueue));
    if (!queue2) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(manager.databases);
        return;
    }
    queue2->database_name = strdup("testdb");
    if (!queue2->database_name) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(queue2);
        free(manager.databases);
        return;
    }
    queue2->queue_type = strdup("fast");
    if (!queue2->queue_type) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(queue2->database_name);
        free(queue2);
        free(manager.databases);
        return;
    }
    queue2->current_queue_depth = 1;
    queue2->last_request_time = 2000;

    manager.databases[0] = queue1;
    manager.databases[1] = queue2;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_EQUAL_PTR(queue1, result); // Should select queue1 due to earlier timestamp

    // Cleanup
    free(queue1->database_name);
    free(queue1->queue_type);
    free(queue1);
    free(queue2->database_name);
    free(queue2->queue_type);
    free(queue2);
    free(manager.databases);
}

// Test: Different depths should select lowest depth
void test_select_optimal_queue_different_depths(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 2;
    manager.databases = calloc(2, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    // Create queue with lower depth
    DatabaseQueue* queue1 = calloc(1, sizeof(DatabaseQueue));
    if (!queue1) { free(manager.databases); return; }
    queue1->database_name = strdup("testdb");
    if (!queue1->database_name) { free(queue1); free(manager.databases); return; }
    queue1->queue_type = strdup("fast");
    if (!queue1->queue_type) { free(queue1->database_name); free(queue1); free(manager.databases); return; }
    queue1->current_queue_depth = 1;
    queue1->last_request_time = 2000;

    // Create queue with higher depth
    DatabaseQueue* queue2 = calloc(1, sizeof(DatabaseQueue));
    if (!queue2) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(manager.databases);
        return;
    }
    queue2->database_name = strdup("testdb");
    if (!queue2->database_name) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(queue2);
        free(manager.databases);
        return;
    }
    queue2->queue_type = strdup("fast");
    if (!queue2->queue_type) {
        free(queue1->database_name);
        free(queue1->queue_type);
        free(queue1);
        free(queue2->database_name);
        free(queue2);
        free(manager.databases);
        return;
    }
    queue2->current_queue_depth = 3;
    queue2->last_request_time = 1000;

    manager.databases[0] = queue1;
    manager.databases[1] = queue2;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_EQUAL_PTR(queue1, result); // Should select queue1 due to lower depth

    // Cleanup
    free(queue1->database_name);
    free(queue1->queue_type);
    free(queue1);
    free(queue2->database_name);
    free(queue2->queue_type);
    free(queue2);
    free(manager.databases);
}

// Test: Queue type filtering
void test_select_optimal_queue_with_queue_type_filter(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 2;
    manager.databases = calloc(2, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    // Create fast queue
    DatabaseQueue* fast_queue = calloc(1, sizeof(DatabaseQueue));
    if (!fast_queue) { free(manager.databases); return; }
    fast_queue->database_name = strdup("testdb");
    if (!fast_queue->database_name) { free(fast_queue); free(manager.databases); return; }
    fast_queue->queue_type = strdup("fast");
    if (!fast_queue->queue_type) { free(fast_queue->database_name); free(fast_queue); free(manager.databases); return; }
    fast_queue->current_queue_depth = 0;
    fast_queue->last_request_time = 1000;

    // Create slow queue
    DatabaseQueue* slow_queue = calloc(1, sizeof(DatabaseQueue));
    if (!slow_queue) {
        free(fast_queue->database_name);
        free(fast_queue->queue_type);
        free(fast_queue);
        free(manager.databases);
        return;
    }
    slow_queue->database_name = strdup("testdb");
    if (!slow_queue->database_name) {
        free(fast_queue->database_name);
        free(fast_queue->queue_type);
        free(fast_queue);
        free(slow_queue);
        free(manager.databases);
        return;
    }
    slow_queue->queue_type = strdup("slow");
    if (!slow_queue->queue_type) {
        free(fast_queue->database_name);
        free(fast_queue->queue_type);
        free(fast_queue);
        free(slow_queue->database_name);
        free(slow_queue);
        free(manager.databases);
        return;
    }
    slow_queue->current_queue_depth = 0;
    slow_queue->last_request_time = 1000;

    manager.databases[0] = fast_queue;
    manager.databases[1] = slow_queue;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_EQUAL_PTR(fast_queue, result); // Should select fast queue

    // Cleanup
    free(fast_queue->database_name);
    free(fast_queue->queue_type);
    free(fast_queue);
    free(slow_queue->database_name);
    free(slow_queue->queue_type);
    free(slow_queue);
    free(manager.databases);
}

// Test: No matching database should return NULL
void test_select_optimal_queue_no_matching_database(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 1;
    manager.databases = calloc(1, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) { free(manager.databases); return; }
    queue->database_name = strdup("otherdb");
    if (!queue->database_name) { free(queue); free(manager.databases); return; }
    queue->queue_type = strdup("fast");
    if (!queue->queue_type) { free(queue->database_name); free(queue); free(manager.databases); return; }
    queue->current_queue_depth = 0;
    queue->last_request_time = 1000;

    manager.databases[0] = queue;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_NULL(result); // Should return NULL for non-matching database

    // Cleanup
    free(queue->database_name);
    free(queue->queue_type);
    free(queue);
    free(manager.databases);
}

// Test: No matching queue type should return NULL
void test_select_optimal_queue_no_matching_queue_type(void) {
    DatabaseQueueManager manager = {0};
    manager.database_count = 1;
    manager.databases = calloc(1, sizeof(DatabaseQueue*));
    if (!manager.databases) return;

    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) { free(manager.databases); return; }
    queue->database_name = strdup("testdb");
    if (!queue->database_name) { free(queue); free(manager.databases); return; }
    queue->queue_type = strdup("slow");
    if (!queue->queue_type) { free(queue->database_name); free(queue); free(manager.databases); return; }
    queue->current_queue_depth = 0;
    queue->last_request_time = 1000;

    manager.databases[0] = queue;

    DatabaseQueue* result = select_optimal_queue("testdb", "fast", &manager);
    TEST_ASSERT_NULL(result); // Should return NULL for non-matching queue type

    // Cleanup
    free(queue->database_name);
    free(queue->queue_type);
    free(queue);
    free(manager.databases);
}

// Test: NULL queue parameter should not crash
void test_update_queue_last_request_time_null_queue(void) {
    update_queue_last_request_time(NULL);
    // Test passes if no crash occurs
}

// Test: Valid queue should update timestamp
void test_update_queue_last_request_time_valid_queue(void) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    time_t before = time(NULL);

    update_queue_last_request_time(queue);

    time_t after = time(NULL);
    TEST_ASSERT_GREATER_OR_EQUAL(before, queue->last_request_time);
    TEST_ASSERT_LESS_OR_EQUAL(after, queue->last_request_time);

    free(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_select_optimal_queue_null_parameters);
    RUN_TEST(test_select_optimal_queue_no_databases);
    RUN_TEST(test_select_optimal_queue_single_queue);
    RUN_TEST(test_select_optimal_queue_multiple_queues_same_depth);
    RUN_TEST(test_select_optimal_queue_different_depths);
    RUN_TEST(test_select_optimal_queue_with_queue_type_filter);
    RUN_TEST(test_select_optimal_queue_no_matching_database);
    RUN_TEST(test_select_optimal_queue_no_matching_queue_type);
    RUN_TEST(test_update_queue_last_request_time_null_queue);
    RUN_TEST(test_update_queue_last_request_time_valid_queue);

    return UNITY_END();
}