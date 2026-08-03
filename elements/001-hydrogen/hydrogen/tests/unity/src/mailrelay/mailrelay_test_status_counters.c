/*
 * Unity Test File: Mail Relay Status Counters
 *
 * Verifies mailrelay_get_status() returns accurate counter snapshots and
 * handles uninitialized runtime gracefully.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>

#include <string.h>

// Forward declarations for test functions
void test_status_uninitialized_runtime_returns_enabled(void);
void test_status_initialized_runtime_counters_zero(void);
void test_status_counters_track_queued(void);
void test_status_counters_track_sending_sent_failed(void);

void setUp(void) {
    mailrelay_shutdown();
}

void tearDown(void) {
    mailrelay_shutdown();
}

void test_status_uninitialized_runtime_returns_enabled(void) {
    // When runtime is not initialized, mailrelay_get_status should still
    // report enabled state from app_config and return false.
    MailRelayStatusCounters counters;
    memset(&counters, 0xFF, sizeof(counters));

    bool result = mailrelay_get_status(&counters);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(counters.enabled);
    TEST_ASSERT_EQUAL_INT(0, counters.queue_depth);
    TEST_ASSERT_EQUAL_INT(0, counters.worker_count);
}

void test_status_initialized_runtime_counters_zero(void) {
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayStatusCounters counters;
    memset(&counters, 0xFF, sizeof(counters));

    bool result = mailrelay_get_status(&counters);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(counters.initialized);
    TEST_ASSERT_EQUAL_size_t(0, counters.queued);
    TEST_ASSERT_EQUAL_size_t(0, counters.sending);
    TEST_ASSERT_EQUAL_size_t(0, counters.sent);
    TEST_ASSERT_EQUAL_size_t(0, counters.failed);
    TEST_ASSERT_EQUAL_size_t(0, counters.retrying);
    TEST_ASSERT_EQUAL_size_t(0, counters.permanent_failures);
    TEST_ASSERT_EQUAL_INT(0, counters.queue_depth);
    TEST_ASSERT_EQUAL_INT(0, counters.worker_count);
    TEST_ASSERT_EQUAL_INT(0, (int)counters.last_success);
    TEST_ASSERT_EQUAL_INT(0, (int)counters.last_failure);
}

void test_status_counters_track_queued(void) {
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayMessage msg;
    mailrelay_message_init(&msg);
    TEST_ASSERT_TRUE(mailrelay_message_set_from(&msg, "from@example.com"));
    TEST_ASSERT_TRUE(mailrelay_message_add_to(&msg, "to@example.com"));
    msg.subject = strdup("Test");
    msg.text_body = strdup("Body");

    MailRelayStatus status = mailrelay_enqueue(&msg, 0);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);

    MailRelayStatusCounters counters;
    TEST_ASSERT_TRUE(mailrelay_get_status(&counters));
    TEST_ASSERT_EQUAL_size_t(1, counters.queued);
    TEST_ASSERT_EQUAL_INT(1, counters.queue_depth);

    mailrelay_message_free(&msg);
}

void test_status_counters_track_sending_sent_failed(void) {
    // This test directly manipulates runtime counters to verify the snapshot
    // path; actual worker-driven counter wiring is exercised by integration.
    TEST_ASSERT_TRUE(mailrelay_init());

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_runtime->sending_count = 2;
    mailrelay_runtime->sent_count = 3;
    mailrelay_runtime->failed_count = 4;
    mailrelay_runtime->retrying_count = 1;
    mailrelay_runtime->permanent_failures_count = 4;
    mailrelay_runtime->last_success_at = 12345;
    mailrelay_runtime->last_failure_at = 67890;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    MailRelayStatusCounters counters;
    TEST_ASSERT_TRUE(mailrelay_get_status(&counters));
    TEST_ASSERT_EQUAL_size_t(2, counters.sending);
    TEST_ASSERT_EQUAL_size_t(3, counters.sent);
    TEST_ASSERT_EQUAL_size_t(4, counters.failed);
    TEST_ASSERT_EQUAL_size_t(1, counters.retrying);
    TEST_ASSERT_EQUAL_size_t(4, counters.permanent_failures);
    TEST_ASSERT_EQUAL_INT(12345, (int)counters.last_success);
    TEST_ASSERT_EQUAL_INT(67890, (int)counters.last_failure);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_status_uninitialized_runtime_returns_enabled);
    RUN_TEST(test_status_initialized_runtime_counters_zero);
    RUN_TEST(test_status_counters_track_queued);
    RUN_TEST(test_status_counters_track_sending_sent_failed);
    return UNITY_END();
}
