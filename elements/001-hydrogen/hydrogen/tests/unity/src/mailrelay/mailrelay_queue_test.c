/*
 * Unit tests for Mail Relay bounded in-memory queue.
 *
 * Verifies create/destroy, enqueue/dequeue ordering, capacity limiting,
 * priority ordering, shutdown signaling, and cleanup.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_message.h>

#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Forward declarations for test functions
void test_queue_create_destroy(void);
void test_queue_enqueue_dequeue_order(void);
void test_queue_capacity_returns_full(void);
void test_queue_priority_ordering(void);
void test_queue_shutdown_wakes_dequeue(void);
void test_queue_destroy_frees_items(void);
void test_queue_invalid_args(void);

static MailRelayMessage make_message(const char* subject, int priority) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "recipient@example.com");
    m.subject = strdup(subject);
    m.text_body = strdup("test body");
    m.priority = priority;
    return m;
}

static void free_message(MailRelayMessage* m) {
    mailrelay_message_free(m);
}

void setUp(void) {
}

void tearDown(void) {
}

void test_queue_create_destroy(void) {
    MailRelayQueue* queue = mailrelay_queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_INT(5, mailrelay_queue_capacity(queue));
    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));
    mailrelay_queue_destroy(queue);

    // Default capacity when <= 0.
    MailRelayQueue* default_queue = mailrelay_queue_create(0);
    TEST_ASSERT_NOT_NULL(default_queue);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QUEUE_DEFAULT_CAPACITY,
                          mailrelay_queue_capacity(default_queue));
    mailrelay_queue_destroy(default_queue);
}

void test_queue_enqueue_dequeue_order(void) {
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage m1 = make_message("first", 0);
    MailRelayMessage m2 = make_message("second", 0);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &m1, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &m2, 0));
    TEST_ASSERT_EQUAL_INT(2, mailrelay_queue_size(queue));

    MailRelayQueueItem item;
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(queue, 100, &item));
    TEST_ASSERT_EQUAL_STRING("first", item.message.subject);
    free_message(&item.message);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(queue, 100, &item));
    TEST_ASSERT_EQUAL_STRING("second", item.message.subject);
    free_message(&item.message);

    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));

    free_message(&m1);
    free_message(&m2);
    mailrelay_queue_destroy(queue);
}

void test_queue_capacity_returns_full(void) {
    MailRelayQueue* queue = mailrelay_queue_create(2);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage m1 = make_message("one", 0);
    MailRelayMessage m2 = make_message("two", 0);
    MailRelayMessage m3 = make_message("three", 0);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &m1, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &m2, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QUEUE_FULL, mailrelay_queue_enqueue(queue, &m3, 0));
    TEST_ASSERT_EQUAL_INT(2, mailrelay_queue_size(queue));

    free_message(&m1);
    free_message(&m2);
    free_message(&m3);
    mailrelay_queue_destroy(queue);
}

void test_queue_priority_ordering(void) {
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage low = make_message("low", 1);
    MailRelayMessage high = make_message("high", 10);
    MailRelayMessage medium = make_message("medium", 5);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &low, 1));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &high, 10));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(queue, &medium, 5));

    MailRelayQueueItem item;
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(queue, 100, &item));
    TEST_ASSERT_EQUAL_STRING("high", item.message.subject);
    free_message(&item.message);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(queue, 100, &item));
    TEST_ASSERT_EQUAL_STRING("medium", item.message.subject);
    free_message(&item.message);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(queue, 100, &item));
    TEST_ASSERT_EQUAL_STRING("low", item.message.subject);
    free_message(&item.message);

    free_message(&low);
    free_message(&high);
    free_message(&medium);
    mailrelay_queue_destroy(queue);
}

typedef struct {
    MailRelayQueue* queue;
    int result;
} ShutdownTestContext;

static void* dequeue_after_delay(void* arg) {
    ShutdownTestContext* ctx = (ShutdownTestContext*)arg;
    MailRelayQueueItem item;
    ctx->result = mailrelay_queue_dequeue(ctx->queue, 5000, &item);
    if (ctx->result == MAILRELAY_OK) {
        free_message(&item.message);
    }
    return NULL;
}

void test_queue_shutdown_wakes_dequeue(void) {
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    ShutdownTestContext ctx;
    ctx.queue = queue;
    ctx.result = -1;

    pthread_t thread;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, dequeue_after_delay, &ctx));

    // Give the worker time to enter the wait.
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    nanosleep(&ts, NULL);

    mailrelay_queue_shutdown(queue);
    pthread_join(thread, NULL);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_SHUTDOWN, ctx.result);
    mailrelay_queue_destroy(queue);
}

void test_queue_destroy_frees_items(void) {
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage m1 = make_message("orphan", 0);
    MailRelayMessage m2 = make_message("another", 0);
    mailrelay_queue_enqueue(queue, &m1, 0);
    mailrelay_queue_enqueue(queue, &m2, 0);

    free_message(&m1);
    free_message(&m2);

    // Destroying the queue must not leak the copied items.
    mailrelay_queue_destroy(queue);
}

void test_queue_invalid_args(void) {
    MailRelayMessage m = make_message("x", 0);
    MailRelayQueueItem item;

    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, mailrelay_queue_enqueue(NULL, &m, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, mailrelay_queue_dequeue(NULL, 100, &item));

    free_message(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_queue_create_destroy);
    RUN_TEST(test_queue_enqueue_dequeue_order);
    RUN_TEST(test_queue_capacity_returns_full);
    RUN_TEST(test_queue_priority_ordering);
    RUN_TEST(test_queue_shutdown_wakes_dequeue);
    RUN_TEST(test_queue_destroy_frees_items);
    RUN_TEST(test_queue_invalid_args);
    return UNITY_END();
}
