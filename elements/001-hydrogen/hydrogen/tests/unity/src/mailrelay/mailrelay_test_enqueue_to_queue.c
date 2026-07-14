/*
 * Unity Test File: mailrelay_test_enqueue_to_queue.c
 *
 * Tests the public mailrelay_enqueue_to_queue() helper that persists (when
 * enabled) and enqueues a message onto a caller-supplied queue. Covers the
 * NULL-argument guards and the happy-path enqueue, including the runtime
 * counter increment when a runtime is present.
 */

// Project header + Unity framework
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_enqueue_to_queue_null_message_returns_invalid(void);
void test_enqueue_to_queue_null_queue_returns_invalid(void);
void test_enqueue_to_queue_valid_enqueues(void);

static MailRelayMessage make_valid_message(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Enqueue Subject");
    m.text_body = strdup("Enqueue body");
    return m;
}

void setUp(void) {
    mailrelay_shutdown();
}

void tearDown(void) {
    mailrelay_shutdown();
}

void test_enqueue_to_queue_null_message_returns_invalid(void) {
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS,
                          mailrelay_enqueue_to_queue(NULL, 0, queue));
    mailrelay_queue_destroy(queue);
}

void test_enqueue_to_queue_null_queue_returns_invalid(void) {
    MailRelayMessage m = make_valid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS,
                          mailrelay_enqueue_to_queue(&m, 0, NULL));
    mailrelay_message_free(&m);
}

void test_enqueue_to_queue_valid_enqueues(void) {
    // No app_config => persistence is skipped, enqueue proceeds.
    app_config = NULL;
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    size_t before = (size_t)mailrelay_runtime->queued_count;
    MailRelayMessage m = make_valid_message();
    MailRelayStatus status = mailrelay_enqueue_to_queue(&m, 0, queue);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));
    TEST_ASSERT_EQUAL_size_t(before + 1, (size_t)mailrelay_runtime->queued_count);

    mailrelay_message_free(&m);
    mailrelay_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_enqueue_to_queue_null_message_returns_invalid);
    RUN_TEST(test_enqueue_to_queue_null_queue_returns_invalid);
    RUN_TEST(test_enqueue_to_queue_valid_enqueues);
    return UNITY_END();
}
