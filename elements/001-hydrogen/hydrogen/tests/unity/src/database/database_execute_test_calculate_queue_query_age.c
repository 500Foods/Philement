/*
 * Unity Test File: database_execute_test_calculate_queue_query_age
 * This file contains unit tests for calculate_queue_query_age helper function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_execute.h>
#include <src/queue/queue.h>

// Forward declarations for functions being tested
time_t calculate_queue_query_age(DatabaseQueue* db_queue);

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

    // Manually backdate the head element timestamp so age > 0
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    q->head->timestamp.tv_sec = now.tv_sec - 5;
    q->head->timestamp.tv_nsec = now.tv_nsec;

    return q;
}

// Test function prototypes
void test_calculate_queue_query_age_null_queue(void);
void test_calculate_queue_query_age_empty_queue(void);
void test_calculate_queue_query_age_with_items(void);
void test_calculate_queue_query_age_with_child_queues(void);
void test_calculate_queue_query_age_child_newer_than_lead(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
}

// Test calculate_queue_query_age function
void test_calculate_queue_query_age_null_queue(void) {
    time_t result = calculate_queue_query_age(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_calculate_queue_query_age_empty_queue(void) {
    DatabaseQueue db_queue = {0};
    pthread_mutex_init(&db_queue.children_lock, NULL);

    time_t result = calculate_queue_query_age(&db_queue);
    TEST_ASSERT_EQUAL(0, result);

    pthread_mutex_destroy(&db_queue.children_lock);
}

void test_calculate_queue_query_age_with_items(void) {
    Queue* test_queue = create_test_queue_with_item("age_test_queue");
    TEST_ASSERT_NOT_NULL(test_queue);

    DatabaseQueue db_queue = {0};
    pthread_mutex_init(&db_queue.children_lock, NULL);
    db_queue.queue = test_queue;

    time_t result = calculate_queue_query_age(&db_queue);
    TEST_ASSERT_GREATER_THAN(0, result);

    pthread_mutex_destroy(&db_queue.children_lock);
    queue_destroy(test_queue);
}

void test_calculate_queue_query_age_with_child_queues(void) {
    Queue* lead_queue = create_test_queue_with_item("age_lead_queue");
    TEST_ASSERT_NOT_NULL(lead_queue);

    Queue* child_queue = create_test_queue_with_item("age_child_queue");
    TEST_ASSERT_NOT_NULL(child_queue);

    DatabaseQueue child_db_queue = {0};
    pthread_mutex_init(&child_db_queue.children_lock, NULL);
    child_db_queue.queue = child_queue;

    DatabaseQueue* child_queues[1] = {&child_db_queue};

    DatabaseQueue db_queue = {0};
    pthread_mutex_init(&db_queue.children_lock, NULL);
    db_queue.queue = lead_queue;
    db_queue.child_queues = child_queues;
    db_queue.child_queue_count = 1;

    time_t result = calculate_queue_query_age(&db_queue);
    TEST_ASSERT_GREATER_THAN(0, result);

    pthread_mutex_destroy(&db_queue.children_lock);
    pthread_mutex_destroy(&child_db_queue.children_lock);
    queue_destroy(lead_queue);
    queue_destroy(child_queue);
}

// Test that child queue with newer items updates query_age
void test_calculate_queue_query_age_child_newer_than_lead(void) {
    // Lead queue with old items (5 seconds ago)
    Queue* lead_queue = create_test_queue_with_item("age_lead_newer");
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Child queue with even older items (10 seconds ago)
    Queue* child_queue = create_test_queue_with_item("age_child_newer");
    TEST_ASSERT_NOT_NULL(child_queue);

    // Backdate child queue items more than lead
    child_queue->head->timestamp.tv_sec -= 5; // Now 10 seconds ago

    DatabaseQueue child_db_queue = {0};
    pthread_mutex_init(&child_db_queue.children_lock, NULL);
    child_db_queue.queue = child_queue;

    DatabaseQueue* child_queues[1] = {&child_db_queue};

    DatabaseQueue db_queue = {0};
    pthread_mutex_init(&db_queue.children_lock, NULL);
    db_queue.queue = lead_queue;
    db_queue.child_queues = child_queues;
    db_queue.child_queue_count = 1;

    time_t result = calculate_queue_query_age(&db_queue);
    TEST_ASSERT_GREATER_THAN(0, result);

    pthread_mutex_destroy(&db_queue.children_lock);
    pthread_mutex_destroy(&child_db_queue.children_lock);
    queue_destroy(lead_queue);
    queue_destroy(child_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_calculate_queue_query_age_null_queue);
    RUN_TEST(test_calculate_queue_query_age_empty_queue);
    RUN_TEST(test_calculate_queue_query_age_with_items);
    RUN_TEST(test_calculate_queue_query_age_with_child_queues);
    RUN_TEST(test_calculate_queue_query_age_child_newer_than_lead);

    return UNITY_END();
}
