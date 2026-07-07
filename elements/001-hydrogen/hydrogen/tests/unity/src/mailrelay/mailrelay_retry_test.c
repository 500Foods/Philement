/*
 * Unit tests for Mail Relay retry policy and time-aware queue.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_retry.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_workers.h>

#include <src/threads/threads.h>

#include <unity.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations for test functions
void test_retry_classify_transient(void);
void test_retry_classify_permanent(void);
void test_retry_classify_smtp_code_fallback(void);
void test_retry_compute_delay(void);
void test_retry_should_retry(void);
void test_queue_dequeue_skips_future_items(void);
void test_worker_retries_transient_failure_then_succeeds(void);
void test_worker_permanent_failure_no_retry(void);
void test_worker_exhausts_retry_attempts(void);

static AppConfig test_config;
static pthread_mutex_t g_transport_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_send_count = 0;
static int g_fail_count = 0; // Number of failures to inject before success
static bool g_failure_retryable = false;

static bool mock_transport(const MailRelaySmtpRequest* req, MailRelayResult* out) {
    (void)req;
    pthread_mutex_lock(&g_transport_mutex);
    g_send_count++;
    bool retryable = g_failure_retryable;
    int fail_remaining = g_fail_count;
    if (fail_remaining > 0) {
        g_fail_count--;
    }
    pthread_mutex_unlock(&g_transport_mutex);

    mailrelay_result_init(out);
    if (fail_remaining > 0) {
        out->success = false;
        out->retryable = retryable;
        out->smtp_code = retryable ? 421 : 554;
        snprintf(out->error, sizeof(out->error), "mock failure");
        return false;
    }
    out->success = true;
    out->smtp_code = 250;
    snprintf(out->smtp_text, sizeof(out->smtp_text), "OK");
    return true;
}

static void setup_app_config(int retry_attempts, int initial_delay, int max_delay) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.mail_relay.Enabled = true;
    test_config.mail_relay.Workers = 1;
    test_config.mail_relay.OutboundServerCount = 1;
    test_config.mail_relay.Servers[0].Host = strdup("smtp.example.com");
    test_config.mail_relay.Servers[0].Port = strdup("587");
    test_config.mail_relay.Servers[0].Username = strdup("user@example.com");
    test_config.mail_relay.Servers[0].Password = strdup("s3cret");
    test_config.mail_relay.Servers[0].AuthMode = MAIL_AUTH_MODE_PLAIN;
    test_config.mail_relay.Servers[0].TLSMode = MAIL_TLS_MODE_STARTTLS;
    test_config.mail_relay.Queue.RetryAttempts = retry_attempts;
    test_config.mail_relay.Queue.InitialDelaySeconds = initial_delay;
    test_config.mail_relay.Queue.MaxDelaySeconds = max_delay;
    test_config.server.server_name = strdup("hydrogen-test");
    app_config = &test_config;
}

static void cleanup_app_config(void) {
    if (app_config != &test_config) {
        return;
    }
    free(test_config.mail_relay.Servers[0].Host);
    free(test_config.mail_relay.Servers[0].Port);
    free(test_config.mail_relay.Servers[0].Username);
    free(test_config.mail_relay.Servers[0].Password);
    free(test_config.server.server_name);
    app_config = NULL;
}

static void reset_transport(int fail_count, bool retryable) {
    pthread_mutex_lock(&g_transport_mutex);
    g_send_count = 0;
    g_fail_count = fail_count;
    g_failure_retryable = retryable;
    pthread_mutex_unlock(&g_transport_mutex);
    mailrelay_smtp_reset_transport();
    mailrelay_smtp_set_transport(mock_transport);
}

static MailRelayMessage make_message(const char* subject) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "recipient@example.com");
    m.subject = strdup(subject);
    m.text_body = strdup("test body");
    return m;
}

static void free_message(MailRelayMessage* m) {
    mailrelay_message_free(m);
}

static void wait_for(const size_t* counter, size_t target, int timeout_ms) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10000000; // 10 ms
    int iterations = timeout_ms / 10;
    if (iterations < 1) iterations = 1;
    for (int i = 0; i < iterations; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t value = *counter;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (value >= target) break;
        nanosleep(&ts, NULL);
    }
}

void setUp(void) {
    reset_transport(0, false);
    setup_app_config(3, 0, 3600);
    mailrelay_init();
}

void tearDown(void) {
    mailrelay_shutdown();
    mailrelay_smtp_reset_transport();
    cleanup_app_config();
}

void test_retry_classify_transient(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);
    r.success = false;
    r.retryable = true;
    TEST_ASSERT_EQUAL(MAIL_FAILURE_TRANSIENT, mailrelay_retry_classify(&r));
    TEST_ASSERT_TRUE(mailrelay_retry_is_transient(&r));
}

void test_retry_classify_permanent(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);
    r.success = false;
    r.retryable = false;
    TEST_ASSERT_EQUAL(MAIL_FAILURE_PERMANENT, mailrelay_retry_classify(&r));
    TEST_ASSERT_FALSE(mailrelay_retry_is_transient(&r));
}

void test_retry_classify_smtp_code_fallback(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);
    r.success = false;
    r.retryable = false;
    r.smtp_code = 421;
    TEST_ASSERT_EQUAL(MAIL_FAILURE_TRANSIENT, mailrelay_retry_classify(&r));
    r.smtp_code = 554;
    TEST_ASSERT_EQUAL(MAIL_FAILURE_PERMANENT, mailrelay_retry_classify(&r));
}

void test_retry_compute_delay(void) {
    TEST_ASSERT_EQUAL_INT(0, mailrelay_retry_compute_delay(0, 0, 60));
    TEST_ASSERT_EQUAL_INT(1, mailrelay_retry_compute_delay(0, 1, 60));
    TEST_ASSERT_EQUAL_INT(2, mailrelay_retry_compute_delay(1, 1, 60));
    TEST_ASSERT_EQUAL_INT(4, mailrelay_retry_compute_delay(2, 1, 60));
    TEST_ASSERT_EQUAL_INT(8, mailrelay_retry_compute_delay(3, 1, 60));
    TEST_ASSERT_EQUAL_INT(60, mailrelay_retry_compute_delay(10, 1, 60));
    TEST_ASSERT_EQUAL_INT(10, mailrelay_retry_compute_delay(0, 10, 3600));
    TEST_ASSERT_EQUAL_INT(20, mailrelay_retry_compute_delay(1, 10, 3600));
    TEST_ASSERT_EQUAL_INT(3600, mailrelay_retry_compute_delay(20, 10, 3600));
}

void test_retry_should_retry(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);
    r.success = false;
    r.retryable = true;
    TEST_ASSERT_TRUE(mailrelay_retry_should_retry(&r, 0, 3));
    TEST_ASSERT_TRUE(mailrelay_retry_should_retry(&r, 2, 3));
    TEST_ASSERT_FALSE(mailrelay_retry_should_retry(&r, 3, 3));
    TEST_ASSERT_FALSE(mailrelay_retry_should_retry(&r, 4, 3));
    r.retryable = false;
    TEST_ASSERT_FALSE(mailrelay_retry_should_retry(&r, 0, 3));
}

void test_queue_dequeue_skips_future_items(void) {
    MailRelayQueue* q = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(q);

    MailRelayMessage m1 = make_message("immediate");
    MailRelayMessage m2 = make_message("delayed");

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_enqueue(q, &m1, 0));

    struct timespec future;
    clock_gettime(CLOCK_REALTIME, &future);
    future.tv_sec += 2;
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK,
                          mailrelay_queue_enqueue_scheduled(q, &m2, 0, 0, &future));

    MailRelayQueueItem item;
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(q, 100, &item));
    TEST_ASSERT_EQUAL_STRING("immediate", item.message.subject);
    mailrelay_message_free(&item.message);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_TIMEOUT, mailrelay_queue_dequeue(q, 100, &item));

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_queue_dequeue(q, 3000, &item));
    TEST_ASSERT_EQUAL_STRING("delayed", item.message.subject);
    mailrelay_message_free(&item.message);

    mailrelay_queue_destroy(q);
    free_message(&m1);
    free_message(&m2);
}

void test_worker_retries_transient_failure_then_succeeds(void) {
    reset_transport(2, true); // 2 transient failures, then success
    setup_app_config(3, 0, 3600); // Re-apply config after reset
    // mailrelay_init already ran in setUp; config was captured, but workers use
    // app_config pointer at runtime. app_config is already set.

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("retry success test");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    wait_for(&mailrelay_runtime->sent_count, 1, 2000);

    pthread_mutex_lock(&g_transport_mutex);
    int sends = g_send_count;
    pthread_mutex_unlock(&g_transport_mutex);
    TEST_ASSERT_EQUAL_INT(3, sends); // initial + 2 retries

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    size_t sent = mailrelay_runtime->sent_count;
    size_t retries = mailrelay_runtime->retry_count;
    size_t failed = mailrelay_runtime->failed_count;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);
    TEST_ASSERT_EQUAL_size_t(1, sent);
    TEST_ASSERT_EQUAL_size_t(2, retries);
    TEST_ASSERT_EQUAL_size_t(0, failed);

    free_message(&m);
}

void test_worker_permanent_failure_no_retry(void) {
    reset_transport(1, false); // 1 permanent failure
    setup_app_config(3, 0, 3600);

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("permanent failure test");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    wait_for(&mailrelay_runtime->failed_count, 1, 2000);

    pthread_mutex_lock(&g_transport_mutex);
    int sends = g_send_count;
    pthread_mutex_unlock(&g_transport_mutex);
    TEST_ASSERT_EQUAL_INT(1, sends); // only initial attempt

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    size_t sent = mailrelay_runtime->sent_count;
    size_t retries = mailrelay_runtime->retry_count;
    size_t failed = mailrelay_runtime->failed_count;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);
    TEST_ASSERT_EQUAL_size_t(0, sent);
    TEST_ASSERT_EQUAL_size_t(0, retries);
    TEST_ASSERT_EQUAL_size_t(1, failed);

    free_message(&m);
}

void test_worker_exhausts_retry_attempts(void) {
    reset_transport(5, true); // more failures than max attempts
    setup_app_config(2, 0, 3600); // max 2 retries

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("exhaust retries test");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    wait_for(&mailrelay_runtime->failed_count, 1, 2000);

    pthread_mutex_lock(&g_transport_mutex);
    int sends = g_send_count;
    pthread_mutex_unlock(&g_transport_mutex);
    TEST_ASSERT_EQUAL_INT(3, sends); // initial + 2 retries

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    size_t sent = mailrelay_runtime->sent_count;
    size_t retries = mailrelay_runtime->retry_count;
    size_t failed = mailrelay_runtime->failed_count;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);
    TEST_ASSERT_EQUAL_size_t(0, sent);
    TEST_ASSERT_EQUAL_size_t(2, retries);
    TEST_ASSERT_EQUAL_size_t(1, failed);

    free_message(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_retry_classify_transient);
    RUN_TEST(test_retry_classify_permanent);
    RUN_TEST(test_retry_classify_smtp_code_fallback);
    RUN_TEST(test_retry_compute_delay);
    RUN_TEST(test_retry_should_retry);
    RUN_TEST(test_queue_dequeue_skips_future_items);
    RUN_TEST(test_worker_retries_transient_failure_then_succeeds);
    RUN_TEST(test_worker_permanent_failure_no_retry);
    RUN_TEST(test_worker_exhausts_retry_attempts);
    return UNITY_END();
}
