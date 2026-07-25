/*
 * Unity Test File: database_execute_test_find_max_query_age_across_queues
 * This file contains unit tests for find_max_query_age_across_queues helper function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_execute.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/queue/queue.h>

// Forward declarations for functions being tested
time_t find_max_query_age_across_queues(void);

// Helper: create a Queue with one element whose timestamp is in the past
static Queue* create_test_queue_with_item(const char* name) {
    QueueAttributes attrs = {0};
    attrs.initial_memory = 1024;
    attrs.chunk_size = 256;
    attrs.warning_limit = 8192;

    Queue* q = queue_create(name, &attrs);
    if (!q) return NULL;

    bool enqueued = queue_enqueue(q, "test_data", 9, 1);
    if (!enqueued) {
        queue_destroy(q);
        return NULL;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    q->head->timestamp.tv_sec = now.tv_sec - 5;
    q->head->timestamp.tv_nsec = now.tv_nsec;

    return q;
}

// Test function prototypes
void test_find_max_query_age_across_queues_no_manager(void);
void test_find_max_query_age_across_queues_empty_manager(void);
void test_find_max_query_age_across_queues_with_queues(void);

void setUp(void) {
    database_subsystem_init();
    database_queue_system_init();
}

void tearDown(void) {
    if (global_queue_manager) {
        global_queue_manager->database_count = 0;
    }
    database_subsystem_shutdown();
}

// Test: global_queue_manager is NULL
void test_find_max_query_age_across_queues_no_manager(void) {
    database_queue_system_destroy();
    time_t result = find_max_query_age_across_queues();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: empty queue manager (no databases)
void test_find_max_query_age_across_queues_empty_manager(void) {
    time_t result = find_max_query_age_across_queues();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: queue manager with databases containing items
void test_find_max_query_age_across_queues_with_queues(void) {
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    Queue* test_queue = create_test_queue_with_item("max_age_test_queue");
    TEST_ASSERT_NOT_NULL(test_queue);

    DatabaseQueue db_queue = {0};
    pthread_mutex_init(&db_queue.children_lock, NULL);
    db_queue.queue = test_queue;

    global_queue_manager->databases[0] = &db_queue;
    global_queue_manager->database_count = 1;

    time_t result = find_max_query_age_across_queues();
    TEST_ASSERT_GREATER_THAN(0, result);

    pthread_mutex_destroy(&db_queue.children_lock);
    queue_destroy(test_queue);

    global_queue_manager->databases[0] = NULL;
    global_queue_manager->database_count = 0;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_find_max_query_age_across_queues_no_manager);
    RUN_TEST(test_find_max_query_age_across_queues_empty_manager);
    RUN_TEST(test_find_max_query_age_across_queues_with_queues);

    return UNITY_END();
}
