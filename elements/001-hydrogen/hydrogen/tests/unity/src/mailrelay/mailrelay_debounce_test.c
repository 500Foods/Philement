/*
 * Unit tests for Mail Relay debounce / coalescing.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_test_seams.h>

#include <unity.h>
#include <string.h>
#include <time.h>

// Forward declarations for test functions
void test_debounce_submit_no_key_enqueues_normally(void);
void test_debounce_submit_disabled_window_enqueues_normally(void);
void test_debounce_submit_creates_pending_entry(void);
void test_debounce_submit_coalesces_same_key(void);
void test_debounce_process_expired_replaces_placeholders(void);
void test_debounce_process_expired_respects_window(void);
void test_debounce_flush_enqueues_immediately(void);
void test_debounce_max_pending(void);

static time_t g_test_time = 1000;

static time_t test_time_fn(void) {
    return g_test_time;
}

static void build_test_message(MailRelayMessage* msg,
                               const char* key,
                               const char* subject,
                               const char* body) {
    mailrelay_message_init(msg);
    mailrelay_message_set_from(msg, "from@example.com");
    mailrelay_message_add_to(msg, "to@example.com");
    msg->subject = strdup(subject);
    msg->text_body = strdup(body);
    if (key && key[0] != '\0') {
        msg->debounce_key = strdup(key);
    }
}

void setUp(void) {
    g_test_time = 1000;
    mailrelay_set_seam_time(test_time_fn);
}

void tearDown(void) {
    mailrelay_reset_seams();
}

void test_debounce_submit_no_key_enqueues_normally(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, NULL, "Subject", "Body");

    MailRelayStatus status = MAILRELAY_OK;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 5, &status);
    TEST_ASSERT_FALSE(consumed);

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_submit_disabled_window_enqueues_normally(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");

    MailRelayStatus status = MAILRELAY_OK;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 0, &status);
    TEST_ASSERT_FALSE(consumed);

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_submit_creates_pending_entry(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");

    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 5, &status);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_submit_coalesces_same_key(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    for (int i = 0; i < 5; i++) {
        MailRelayMessage msg;
        build_test_message(&msg, "event.1", "Subject", "Body");
        MailRelayStatus status = MAILRELAY_INVALID_ARGS;
        bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 5, &status);
        TEST_ASSERT_TRUE(consumed);
        TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
        mailrelay_message_free(&msg);
    }

    // No messages should have been enqueued yet.
    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_process_expired_replaces_placeholders(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    for (int i = 0; i < 3; i++) {
        MailRelayMessage msg;
        build_test_message(&msg, "event.1", "Alert: %COUNT% - %SUMMARY%",
                           "There were %COUNT% events.");
        MailRelayStatus status = MAILRELAY_INVALID_ARGS;
        bool consumed = mailrelay_debounce_submit(state, queue, &msg, 5, 10, &status);
        TEST_ASSERT_TRUE(consumed);
        TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
        mailrelay_message_free(&msg);
    }

    g_test_time = 1011; // past the 10-second window
    int next_ms = mailrelay_debounce_process_expired(state, queue, g_test_time);
    TEST_ASSERT_EQUAL_INT(-1, next_ms);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    MailRelayQueueItem item;
    MailRelayStatus deq_status = mailrelay_queue_dequeue(queue, 100, &item);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, deq_status);
    TEST_ASSERT_NOT_NULL(item.message.subject);
    TEST_ASSERT_NOT_NULL(item.message.text_body);
    TEST_ASSERT_TRUE(strstr(item.message.subject, "3") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.text_body, "3") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.subject, "3 events") != NULL);

    mailrelay_message_free(&item.message);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_process_expired_respects_window(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");
    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 10, &status);
    TEST_ASSERT_TRUE(consumed);
    mailrelay_message_free(&msg);

    g_test_time = 1005; // still inside window
    int next_ms = mailrelay_debounce_process_expired(state, queue, g_test_time);
    TEST_ASSERT_TRUE(next_ms > 0);
    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_flush_enqueues_immediately(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Flush %COUNT%", "Flush body");
    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 60, &status);
    TEST_ASSERT_TRUE(consumed);
    mailrelay_message_free(&msg);

    mailrelay_debounce_flush(state, queue);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    MailRelayQueueItem item;
    MailRelayStatus deq_status = mailrelay_queue_dequeue(queue, 100, &item);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, deq_status);
    TEST_ASSERT_TRUE(strstr(item.message.subject, "1") != NULL);

    mailrelay_message_free(&item.message);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_max_pending(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(
        MAILRELAY_DEBOUNCE_MAX_PENDING + 10);
    TEST_ASSERT_NOT_NULL(queue);

    for (int i = 0; i < MAILRELAY_DEBOUNCE_MAX_PENDING + 2; i++) {
        char key[64];
        snprintf(key, sizeof(key), "event.%d", i);
        MailRelayMessage msg;
        build_test_message(&msg, key, "Subject", "Body");
        MailRelayStatus status = MAILRELAY_OK;
        bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 10, &status);
        mailrelay_message_free(&msg);
        if (i < MAILRELAY_DEBOUNCE_MAX_PENDING) {
            TEST_ASSERT_TRUE(consumed);
            TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
        } else {
            TEST_ASSERT_TRUE(consumed);
            TEST_ASSERT_EQUAL_INT(MAILRELAY_QUEUE_FULL, status);
        }
    }

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_debounce_submit_no_key_enqueues_normally);
    RUN_TEST(test_debounce_submit_disabled_window_enqueues_normally);
    RUN_TEST(test_debounce_submit_creates_pending_entry);
    RUN_TEST(test_debounce_submit_coalesces_same_key);
    RUN_TEST(test_debounce_process_expired_replaces_placeholders);
    RUN_TEST(test_debounce_process_expired_respects_window);
    RUN_TEST(test_debounce_flush_enqueues_immediately);
    RUN_TEST(test_debounce_max_pending);
    return UNITY_END();
}
