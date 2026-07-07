/*
 * Unit tests for Mail Relay worker pool.
 *
 * Verifies that worker threads dequeue messages, send them via the SMTP
 * transport seam, update counters, and shut down cleanly.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_workers.h>

#include <src/threads/threads.h>

#include <unity.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_workers_start_stop(void);
void test_workers_process_messages(void);
void test_workers_update_counters(void);
void test_workers_no_workers_start_with_zero_count(void);

static AppConfig test_config;
static pthread_mutex_t g_transport_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_send_count = 0;
static char g_last_subject[256];
static bool g_fail = false;

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
        out->smtp_code = 554;
        snprintf(out->error, sizeof(out->error), "mock failure");
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

void setUp(void) {
    reset_transport();
    setup_app_config();
    mailrelay_init();
}

void tearDown(void) {
    mailrelay_shutdown();
    mailrelay_smtp_reset_transport();
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_workers_start_stop);
    RUN_TEST(test_workers_process_messages);
    RUN_TEST(test_workers_update_counters);
    RUN_TEST(test_workers_no_workers_start_with_zero_count);
    return UNITY_END();
}
