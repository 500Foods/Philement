/*
 * Unit tests for Mail Relay debounce / coalescing.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_test_seams.h>
#include <src/mailrelay/mailrelay_internal.h>

#include <unity.h>

#include <limits.h>
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
void test_debounce_destroy_null(void);
void test_debounce_process_expired_null_state(void);
void test_debounce_flush_null_args(void);
void test_debounce_stop_no_runtime(void);
void test_debounce_build_html_placeholder(void);
void test_debounce_process_expired_removes_middle_entry(void);
void test_debounce_process_expired_large_window_caps_next_ms(void);
void test_debounce_flush_queue_full(void);
void test_debounce_free_entry_null(void);
void test_debounce_find_entry_null_and_missing(void);
void test_debounce_replace_all_edge_cases(void);
void test_debounce_build_coalesced_null_args(void);
void test_debounce_enqueue_coalesced_null_args(void);
void test_debounce_submit_starts_expiry_thread(void);

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

void test_debounce_destroy_null(void) {
    // mailrelay_debounce_destroy(NULL) must be a no-op (line 72).
    mailrelay_debounce_destroy(NULL);
}

void test_debounce_process_expired_null_state(void) {
    // mailrelay_debounce_process_expired(NULL, ...) returns -1 (line 384).
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_INT(-1, mailrelay_debounce_process_expired(NULL, queue, 1000));
    mailrelay_queue_destroy(queue);
}

void test_debounce_flush_null_args(void) {
    // Both NULL-argument guards in mailrelay_debounce_flush return early (line 438).
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    mailrelay_debounce_flush(NULL, queue);
    mailrelay_debounce_flush(state, NULL);

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_stop_no_runtime(void) {
    // mailrelay_debounce_stop() with no runtime returns early (line 540).
    mailrelay_debounce_stop();
}

void test_debounce_build_html_placeholder(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    for (int i = 0; i < 3; i++) {
        MailRelayMessage msg;
        build_test_message(&msg, "event.1",
                           "Alert: %COUNT% - %SUMMARY%",
                           "There were %COUNT% events.");
        msg.html_body = strdup("<p>%COUNT% events: %SUMMARY%</p>");
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
    TEST_ASSERT_NOT_NULL(item.message.html_body);
    TEST_ASSERT_TRUE(strstr(item.message.subject, "3") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.subject, "3 events") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.text_body, "3") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.html_body, "3") != NULL);
    TEST_ASSERT_TRUE(strstr(item.message.html_body, "3 events") != NULL);

    mailrelay_message_free(&item.message);
    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_process_expired_removes_middle_entry(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    // Submit "a" with a short window and "b" with a long window. Because "b"
    // is submitted last it sits at the head of the list, so when "a" expires
    // it is removed via the prev->next reassignment (line 398).
    MailRelayMessage a;
    build_test_message(&a, "event.a", "Subject A", "Body A");
    MailRelayStatus status_a = MAILRELAY_INVALID_ARGS;
    TEST_ASSERT_TRUE(mailrelay_debounce_submit(state, queue, &a, 0, 2, &status_a));
    mailrelay_message_free(&a);

    MailRelayMessage b;
    build_test_message(&b, "event.b", "Subject B", "Body B");
    MailRelayStatus status_b = MAILRELAY_INVALID_ARGS;
    TEST_ASSERT_TRUE(mailrelay_debounce_submit(state, queue, &b, 0, 100, &status_b));
    mailrelay_message_free(&b);

    g_test_time = 1003; // past "a" expiry (1002), before "b" expiry (1100)
    int next_ms = mailrelay_debounce_process_expired(state, queue, g_test_time);
    TEST_ASSERT_TRUE(next_ms > 0);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    // "b" is still pending; flush it out.
    g_test_time = 1101;
    int next_ms2 = mailrelay_debounce_process_expired(state, queue, g_test_time);
    TEST_ASSERT_EQUAL_INT(-1, next_ms2);
    TEST_ASSERT_EQUAL_INT(2, mailrelay_queue_size(queue));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_process_expired_large_window_caps_next_ms(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");
    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    TEST_ASSERT_TRUE(mailrelay_debounce_submit(state, queue, &msg, 0, 3000000, &status));
    mailrelay_message_free(&msg);

    g_test_time = 1000;
    int next_ms = mailrelay_debounce_process_expired(state, queue, g_test_time);
    // diff (3000000) exceeds INT_MAX/1000, so it is capped (lines 430-431).
    TEST_ASSERT_EQUAL_INT((int)(INT_MAX / 1000) * 1000, next_ms);
    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_flush_queue_full(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    // Capacity of 1 so the second enqueue fails and logs (lines 372-374).
    MailRelayQueue* queue = mailrelay_queue_create(1);
    TEST_ASSERT_NOT_NULL(queue);

    for (int i = 0; i < 2; i++) {
        char key[64];
        snprintf(key, sizeof(key), "event.%d", i);
        MailRelayMessage msg;
        build_test_message(&msg, key, "Subject", "Body");
        MailRelayStatus status = MAILRELAY_INVALID_ARGS;
        TEST_ASSERT_TRUE(mailrelay_debounce_submit(state, queue, &msg, 0, 60, &status));
        mailrelay_message_free(&msg);
    }

    mailrelay_debounce_flush(state, queue);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_free_entry_null(void) {
    // mailrelay_debounce_free_entry(NULL) is a no-op (line 63).
    mailrelay_debounce_free_entry(NULL);
}

void test_debounce_find_entry_null_and_missing(void) {
    MailRelayDebounceState* state = mailrelay_debounce_create();
    TEST_ASSERT_NOT_NULL(state);

    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");
    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    TEST_ASSERT_TRUE(mailrelay_debounce_submit(state, queue, &msg, 0, 60, &status));
    mailrelay_message_free(&msg);

    // NULL state / NULL key guards (lines 91-92).
    TEST_ASSERT_NULL(mailrelay_debounce_find_entry(NULL, "event.1"));
    TEST_ASSERT_NULL(mailrelay_debounce_find_entry(state, NULL));
    // Existing key is found; a missing key returns NULL (loop covered).
    TEST_ASSERT_NOT_NULL(mailrelay_debounce_find_entry(state, "event.1"));
    TEST_ASSERT_NULL(mailrelay_debounce_find_entry(state, "does.not.exist"));

    mailrelay_queue_destroy(queue);
    mailrelay_debounce_destroy(state);
}

void test_debounce_replace_all_edge_cases(void) {
    char* out = NULL;

    // NULL out parameter returns false without touching *out (line 219).
    TEST_ASSERT_FALSE(mailrelay_debounce_replace_all("src", "a", "b", NULL));

    // NULL placeholder or value falls back to a plain copy of src (lines 226-227).
    TEST_ASSERT_TRUE(mailrelay_debounce_replace_all("src", NULL, "v", &out));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("src", out);
    free(out);
    out = NULL;

    TEST_ASSERT_TRUE(mailrelay_debounce_replace_all("src", "p", NULL, &out));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("src", out);
    free(out);
    out = NULL;

    // No occurrence of the placeholder: a copy of src is returned.
    TEST_ASSERT_TRUE(mailrelay_debounce_replace_all("hello", "xyz", "abc", &out));
    TEST_ASSERT_EQUAL_STRING("hello", out);
    free(out);
    out = NULL;

    // Replacement occurs for every occurrence.
    TEST_ASSERT_TRUE(mailrelay_debounce_replace_all("a-a", "a", "b", &out));
    TEST_ASSERT_EQUAL_STRING("b-b", out);
    free(out);
}

void test_debounce_build_coalesced_null_args(void) {
    // NULL entry or NULL out is rejected (line 269).
    MailRelayMessage out;
    mailrelay_message_init(&out);
    TEST_ASSERT_FALSE(mailrelay_debounce_build_coalesced_message(NULL, &out));
    TEST_ASSERT_FALSE(mailrelay_debounce_build_coalesced_message(
        (const MailRelayDebounceEntry*)NULL, NULL));
    mailrelay_message_free(&out);
}

void test_debounce_enqueue_coalesced_null_args(void) {
    // NULL entry or NULL queue is a no-op (line 358).
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);
    mailrelay_debounce_enqueue_coalesced(NULL, queue);
    mailrelay_debounce_enqueue_coalesced(
        (MailRelayDebounceEntry*)NULL, NULL);
    mailrelay_queue_destroy(queue);
}

void test_debounce_submit_starts_expiry_thread(void) {
    // A runtime-backed state makes mailrelay_debounce_submit() lazily start the
    // expiry thread (lines 156, 159), exercising mailrelay_debounce_start
    // (508-535) and the thread body (458-505). The thread is detached and
    // self-terminates once shutdown is requested, so we deliberately leak the
    // heap runtime/state/queue: they remain valid for the background thread to
    // finish using after this test returns, and the process exit reclaims them.
    MailRelayQueue* queue = mailrelay_queue_create(10);
    MailRelayDebounceState* state = mailrelay_debounce_create();

    MailRelayRuntime* rt = calloc(1, sizeof(MailRelayRuntime));
    TEST_ASSERT_NOT_NULL(rt); // cppcheck-suppress nullPointerOutOfMemory
    rt->initialized = true;
    rt->queue = queue;
    rt->debounce = state;
    mailrelay_runtime = rt;

    MailRelayMessage msg;
    build_test_message(&msg, "event.1", "Subject", "Body");
    MailRelayStatus status = MAILRELAY_INVALID_ARGS;
    bool consumed = mailrelay_debounce_submit(state, queue, &msg, 0, 5, &status);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);

    mailrelay_message_free(&msg);

    // Give the detached worker a brief window to run an expiry iteration
    // (lines 473-501) before we request shutdown; otherwise the thread may
    // observe shutdown on its very first check and skip the work loop.
    struct timespec spin = {0, 50000000L}; // 50 ms
    nanosleep(&spin, NULL);

    // Signal shutdown so the detached thread wakes and exits on its own.
    mailrelay_debounce_stop();
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
    RUN_TEST(test_debounce_destroy_null);
    RUN_TEST(test_debounce_process_expired_null_state);
    RUN_TEST(test_debounce_flush_null_args);
    RUN_TEST(test_debounce_stop_no_runtime);
    RUN_TEST(test_debounce_build_html_placeholder);
    RUN_TEST(test_debounce_process_expired_removes_middle_entry);
    RUN_TEST(test_debounce_process_expired_large_window_caps_next_ms);
    RUN_TEST(test_debounce_flush_queue_full);
    RUN_TEST(test_debounce_free_entry_null);
    RUN_TEST(test_debounce_find_entry_null_and_missing);
    RUN_TEST(test_debounce_replace_all_edge_cases);
    RUN_TEST(test_debounce_build_coalesced_null_args);
    RUN_TEST(test_debounce_enqueue_coalesced_null_args);
    RUN_TEST(test_debounce_submit_starts_expiry_thread);
    return UNITY_END();
}
