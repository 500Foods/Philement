/*
 * Unit tests for Mail Relay worker pool.
 *
 * Verifies that worker threads dequeue messages, send them via the SMTP
 * transport seam, update counters, and shut down cleanly. Also covers the
 * worker persistence callbacks and failure paths (no servers, claim lost,
 * retry-enqueue failure, permanent failure).
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_repository.h>
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
void test_workers_start_stop(void);
void test_workers_process_messages(void);
void test_workers_update_counters(void);
void test_workers_no_workers_start_with_zero_count(void);

void test_worker_repo_noop_cb(void);
void test_worker_persist_outcome_null_item(void);
void test_worker_persist_outcome_queue_id_zero(void);
void test_worker_persist_outcome_persist_disabled(void);
void test_worker_persist_outcome_sent(void);
void test_worker_persist_outcome_retrying(void);
void test_worker_persist_outcome_failed(void);

void test_workers_start_not_initialized(void);
void test_workers_stop_null_runtime(void);
void test_worker_thread_no_servers_configured(void);
void test_worker_thread_claim_lost(void);
void test_worker_thread_retry_enq_failed(void);
void test_worker_thread_permanent_failure(void);

static AppConfig test_config;
static pthread_mutex_t g_transport_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_send_count = 0;
static char g_last_subject[256];
static bool g_fail = false;
static bool g_fail_retryable = false;
static bool g_shutdown_queue_on_fail = false;

// Executor mock state for persistence callbacks.
static int g_persist_captured_query_ref;
static char* g_persist_captured_params_json;
static bool g_persist_executor_called;
static int g_persist_call_count;
static int g_persist_query_refs[16];
static int g_claim_next_affected_rows;
static bool g_insert_returns_queue_id;

static bool mock_transport(const MailRelaySmtpRequest* req, MailRelayResult* out) {
    pthread_mutex_lock(&g_transport_mutex);
    g_send_count++;
    if (req && req->payload) {
        const char* subj = strstr(req->payload, "Subject: ");
        if (subj) {
            strncpy(g_last_subject, subj + 9, sizeof(g_last_subject) - 1);
            g_last_subject[sizeof(g_last_subject) - 1] = '\0';
            char* nl = strchr(g_last_subject, '\r');
            if (nl) *nl = '\0';
        }
    }
    pthread_mutex_unlock(&g_transport_mutex);

    mailrelay_result_init(out);
    if (g_fail) {
        out->success = false;
        out->smtp_code = g_fail_retryable ? 450 : 554;
        snprintf(out->error, sizeof(out->error), "mock failure");
        out->retryable = g_fail_retryable;
        if (g_shutdown_queue_on_fail && mailrelay_runtime) {
            mailrelay_queue_shutdown(mailrelay_runtime->queue);
        }
        return false;
    }
    out->success = true;
    out->smtp_code = 250;
    snprintf(out->smtp_text, sizeof(out->smtp_text), "OK");
    return true;
}

static void setup_app_config(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.mail_relay.Enabled = true;
    test_config.mail_relay.Workers = 2;
    test_config.mail_relay.OutboundServerCount = 1;
    test_config.mail_relay.Servers[0].Host = strdup("smtp.example.com");
    test_config.mail_relay.Servers[0].Port = strdup("587");
    test_config.mail_relay.Servers[0].Username = strdup("user@example.com");
    test_config.mail_relay.Servers[0].Password = strdup("s3cret");
    test_config.mail_relay.Servers[0].AuthMode = MAIL_AUTH_MODE_PLAIN;
    test_config.mail_relay.Servers[0].TLSMode = MAIL_TLS_MODE_STARTTLS;
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

static void reset_transport(void) {
    pthread_mutex_lock(&g_transport_mutex);
    g_send_count = 0;
    g_last_subject[0] = '\0';
    g_fail = false;
    g_fail_retryable = false;
    g_shutdown_queue_on_fail = false;
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

static void reset_persist_mock_state(void) {
    g_persist_captured_query_ref = -1;
    free(g_persist_captured_params_json);
    g_persist_captured_params_json = NULL;
    g_persist_executor_called = false;
    g_persist_call_count = 0;
    memset(g_persist_query_refs, 0, sizeof(g_persist_query_refs));
    g_claim_next_affected_rows = 0;
    g_insert_returns_queue_id = false;
}

static bool mock_executor(int query_ref, const char* params_json,
                        mailrelay_repo_callback_fn callback, void* user_data) {
    (void)callback;
    (void)user_data;
    g_persist_captured_query_ref = query_ref;
    free(g_persist_captured_params_json);
    g_persist_captured_params_json = params_json ? strdup(params_json) : NULL;
    g_persist_executor_called = true;
    if (g_persist_call_count < 16) {
        g_persist_query_refs[g_persist_call_count] = query_ref;
    }
    g_persist_call_count++;

    int affected_rows = 1;
    if (query_ref == MAILRELAY_QREF_QUEUE_CLAIM_NEXT) {
        affected_rows = g_claim_next_affected_rows;
    }

    json_t* data = NULL;
    if (query_ref == MAILRELAY_QREF_QUEUE_INSERT && g_insert_returns_queue_id) {
        data = json_array();
        if (data) {
            json_t* row = json_object();
            if (row) {
                json_object_set_new(row, "queue_id", json_integer(42));
                json_array_append_new(data, row);
            }
        }
    }

    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .error_message = NULL,
        .data = data,
        .affected_rows = affected_rows
    };
    if (callback) {
        callback(&result, user_data);
    }
    if (data) {
        json_decref(data);
    }
    return true;
}

void setUp(void) {
    reset_transport();
    setup_app_config();
    reset_persist_mock_state();
    mailrelay_repo_set_executor(mock_executor);
    mailrelay_init();
}

void tearDown(void) {
    mailrelay_shutdown();
    mailrelay_smtp_reset_transport();
    mailrelay_repo_set_executor(NULL);
    cleanup_app_config();
}

void test_workers_start_stop(void) {
    TEST_ASSERT_TRUE(mailrelay_workers_start(2));
    // Allow threads to start and register.
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000; // 50 ms
    nanosleep(&ts, NULL);

    update_service_thread_metrics(&mailrelay_threads);
    TEST_ASSERT_EQUAL_INT(2, mailrelay_threads.thread_count);

    mailrelay_shutdown();

    // Wait for drain.
    for (int i = 0; i < 50; i++) {
        update_service_thread_metrics(&mailrelay_threads);
        if (mailrelay_threads.thread_count == 0) break;
        nanosleep(&ts, NULL);
    }
    TEST_ASSERT_EQUAL_INT(0, mailrelay_threads.thread_count);
}

void test_workers_process_messages(void) {
    TEST_ASSERT_TRUE(mailrelay_workers_start(2));

    MailRelayMessage m1 = make_message("worker one");
    MailRelayMessage m2 = make_message("worker two");
    MailRelayMessage m3 = make_message("worker three");

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m1, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m2, 0));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m3, 0));

    // Wait for workers to process all messages.
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&g_transport_mutex);
        int count = g_send_count;
        pthread_mutex_unlock(&g_transport_mutex);
        if (count >= 3) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&g_transport_mutex);
    TEST_ASSERT_EQUAL_INT(3, g_send_count);
    pthread_mutex_unlock(&g_transport_mutex);

    free_message(&m1);
    free_message(&m2);
    free_message(&m3);
}

void test_workers_update_counters(void) {
    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("counter test");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t sent = mailrelay_runtime->sent_count;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (sent >= 1) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->sent_count);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->queued_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->failed_count);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    free_message(&m);
}

void test_workers_no_workers_start_with_zero_count(void) {
    TEST_ASSERT_FALSE(mailrelay_workers_start(0));
}

// ----------------------------------------------------------------------------
// worker_repo_noop_cb
// ----------------------------------------------------------------------------

void test_worker_repo_noop_cb(void) {
    // The noop callback should accept any result and user_data without crashing.
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .error_message = NULL,
        .data = NULL,
        .affected_rows = 1
    };
    worker_repo_noop_cb(&result, NULL);
    worker_repo_noop_cb(NULL, NULL);
}

// ----------------------------------------------------------------------------
// worker_persist_outcome tests
// ----------------------------------------------------------------------------

void test_worker_persist_outcome_null_item(void) {
    worker_persist_outcome(NULL, false, false, NULL);
    TEST_ASSERT_FALSE(g_persist_executor_called);
}

void test_worker_persist_outcome_queue_id_zero(void) {
    MailRelayQueueItem item;
    memset(&item, 0, sizeof(item));
    item.message.queue_id = 0;
    // queue_id <= 0 should early-return without calling executor.
    worker_persist_outcome(&item, true, false, NULL);
    TEST_ASSERT_FALSE(g_persist_executor_called);
}

void test_worker_persist_outcome_persist_disabled(void) {
    test_config.mail_relay.Queue.Persist = false;
    MailRelayQueueItem item;
    memset(&item, 0, sizeof(item));
    mailrelay_message_init(&item.message);
    item.message.queue_id = 42;
    worker_persist_outcome(&item, true, false, NULL);
    // Persist disabled -> no executor calls.
    TEST_ASSERT_FALSE(g_persist_executor_called);
    mailrelay_message_free(&item.message);
}

void test_worker_persist_outcome_sent(void) {
    test_config.mail_relay.Queue.Persist = true;
    MailRelayQueueItem item;
    memset(&item, 0, sizeof(item));
    mailrelay_message_init(&item.message);
    item.message.queue_id = 42;
    item.attempts = 0;

    MailRelayResult result;
    mailrelay_result_init(&result);
    result.success = true;
    result.smtp_code = 250;
    strcpy(result.smtp_text, "OK");

    worker_persist_outcome(&item, true, false, &result);
    TEST_ASSERT_TRUE(g_persist_executor_called);
    TEST_ASSERT_EQUAL_INT(2, g_persist_call_count);
    // First call: mark_sent
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_MARK_SENT, g_persist_query_refs[0]);
    // Second call: attempt_insert
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ATTEMPT_INSERT, g_persist_query_refs[1]);
    mailrelay_message_free(&item.message);
}

void test_worker_persist_outcome_retrying(void) {
    test_config.mail_relay.Queue.Persist = true;
    MailRelayQueueItem item;
    memset(&item, 0, sizeof(item));
    mailrelay_message_init(&item.message);
    item.message.queue_id = 42;
    item.attempts = 1;
    item.next_attempt_at.tv_sec = time(NULL) + 10;
    item.next_attempt_at.tv_nsec = 0;

    MailRelayResult result;
    mailrelay_result_init(&result);
    result.success = false;
    result.smtp_code = 450;
    strcpy(result.smtp_text, "transient");
    result.retryable = true;

    worker_persist_outcome(&item, false, true, &result);
    // Should call reschedule (QREF_QUEUE_RESCHEDULE) and then attempt_insert.
    TEST_ASSERT_TRUE(g_persist_executor_called);
    // The last captured query ref should be the attempt insert.
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ATTEMPT_INSERT, g_persist_captured_query_ref);
    mailrelay_message_free(&item.message);
}

void test_worker_persist_outcome_failed(void) {
    test_config.mail_relay.Queue.Persist = true;
    MailRelayQueueItem item;
    memset(&item, 0, sizeof(item));
    mailrelay_message_init(&item.message);
    item.message.queue_id = 42;
    item.attempts = 1;

    MailRelayResult result;
    mailrelay_result_init(&result);
    result.success = false;
    result.smtp_code = 554;
    strcpy(result.smtp_text, "permanent");
    result.retryable = false;

    worker_persist_outcome(&item, false, false, &result);
    // Should call mark_failed (QREF_QUEUE_MARK_FAILED) and then attempt_insert.
    TEST_ASSERT_TRUE(g_persist_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ATTEMPT_INSERT, g_persist_captured_query_ref);
    mailrelay_message_free(&item.message);
}

// ----------------------------------------------------------------------------
// mailrelay_workers_start / stop edge cases
// ----------------------------------------------------------------------------

void test_workers_start_not_initialized(void) {
    // Shut down the runtime that setUp() created.
    mailrelay_shutdown();
    TEST_ASSERT_FALSE(mailrelay_workers_start(1));
}

void test_workers_stop_null_runtime(void) {
    // mailrelay_shutdown in tearDown sets runtime to NULL. Calling stop on
    // a NULL runtime should be a safe no-op.
    mailrelay_shutdown();
    mailrelay_workers_stop();
}

// ----------------------------------------------------------------------------
// Worker thread failure-path tests
// ----------------------------------------------------------------------------

void test_worker_thread_no_servers_configured(void) {
    test_config.mail_relay.OutboundServerCount = 0;
    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("no server");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t failed = mailrelay_runtime->failed_count;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (failed >= 1) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->failed_count);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->permanent_failures_count);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    free_message(&m);
}

void test_worker_thread_claim_lost(void) {
    test_config.mail_relay.Queue.Persist = true;
    test_config.mail_relay.Database = (char*)"acuranzo";

    // Make the mock executor return a queue_id for inserts and affected_rows=0
    // for claims (so the worker sees the claim as lost).
    g_insert_returns_queue_id = true;
    g_claim_next_affected_rows = 0;

    // Re-init runtime with persistence enabled.
    mailrelay_shutdown();
    mailrelay_init();

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    // Enqueue a message; with Persist=true and the mock returning queue_id=42,
    // the worker will attempt an atomic claim (queue_id > 0), which the mock
    // reports as lost (affected_rows=0).
    MailRelayMessage m = make_message("claim lost");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    // Wait for the worker to process the claim-lost path.
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t pfail = mailrelay_runtime->permanent_failures_count;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (pfail >= 1) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    // Claim lost -> permanent failure path (lines 162-171):
    // only permanent_failures_count is incremented, not failed_count.
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->permanent_failures_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->failed_count);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    free_message(&m);
}

void test_worker_thread_retry_enq_failed(void) {
    test_config.mail_relay.Queue.Persist = false;
    test_config.mail_relay.Queue.RetryAttempts = 3;
    test_config.mail_relay.Queue.InitialDelaySeconds = 1;
    test_config.mail_relay.Queue.MaxDelaySeconds = 5;

    // Re-init runtime.
    mailrelay_shutdown();
    mailrelay_init();

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    // Make the transport return a retryable failure and shut down the queue
    // so the worker's re-enqueue for retry fails (lines 217-225).
    pthread_mutex_lock(&g_transport_mutex);
    g_fail = true;
    g_fail_retryable = true;
    g_shutdown_queue_on_fail = true;
    pthread_mutex_unlock(&g_transport_mutex);

    MailRelayMessage m = make_message("retry enq fail");
    m.queue_id = 0;
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t failed = mailrelay_runtime->failed_count;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (failed >= 1) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    // Retry enqueue failed -> permanent failure path (lines 217-225):
    // failed_count + permanent_failures_count incremented, last_error set.
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->failed_count);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->permanent_failures_count);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    free_message(&m);
}

void test_worker_thread_permanent_failure(void) {
    test_config.mail_relay.Queue.Persist = false;
    // Non-retryable failure (smtp_code 554, retryable=false) -> permanent failure.
    pthread_mutex_lock(&g_transport_mutex);
    g_fail = true;
    g_fail_retryable = false;
    pthread_mutex_unlock(&g_transport_mutex);

    TEST_ASSERT_TRUE(mailrelay_workers_start(1));

    MailRelayMessage m = make_message("permanent fail");
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 ms
    for (int i = 0; i < 50; i++) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        size_t failed = mailrelay_runtime->failed_count;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
        if (failed >= 1) break;
        nanosleep(&ts, NULL);
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    // Permanent failure path (lines 230-238): failed + permanent_failures + last_error.
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->failed_count);
    TEST_ASSERT_EQUAL_size_t(1, mailrelay_runtime->permanent_failures_count);
    TEST_ASSERT_EQUAL_INT(554, mailrelay_runtime->last_error.smtp_code);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    free_message(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_workers_start_stop);
    RUN_TEST(test_workers_process_messages);
    RUN_TEST(test_workers_update_counters);
    RUN_TEST(test_workers_no_workers_start_with_zero_count);

    RUN_TEST(test_worker_repo_noop_cb);
    RUN_TEST(test_worker_persist_outcome_null_item);
    RUN_TEST(test_worker_persist_outcome_queue_id_zero);
    RUN_TEST(test_worker_persist_outcome_persist_disabled);
    RUN_TEST(test_worker_persist_outcome_sent);
    RUN_TEST(test_worker_persist_outcome_retrying);
    RUN_TEST(test_worker_persist_outcome_failed);

    RUN_TEST(test_workers_start_not_initialized);
    RUN_TEST(test_workers_stop_null_runtime);
    RUN_TEST(test_worker_thread_no_servers_configured);
    RUN_TEST(test_worker_thread_claim_lost);
    RUN_TEST(test_worker_thread_retry_enq_failed);
    RUN_TEST(test_worker_thread_permanent_failure);
    return UNITY_END();
}
