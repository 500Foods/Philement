/*
 * Unity Test File: mailrelay_test_enqueue.c
 *
 * Tests the public mailrelay_enqueue() producer entry point. Covers the NULL
 * guard, the shutdown/uninitialized path, the disabled path, message
 * validation failure, and the successful enqueue path.
 */

// Project header + Unity framework
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_enqueue_null_message_returns_invalid(void);
void test_enqueue_not_initialized_returns_shutdown(void);
void test_enqueue_disabled_returns_disabled(void);
void test_enqueue_invalid_message_returns_invalid(void);
void test_enqueue_valid_returns_ok(void);

// Saved global config so each test can restore app_config afterwards.
static AppConfig* g_saved_app_config = NULL;
static AppConfig g_test_config = { 0 };

static MailRelayMessage make_valid_message(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Enqueue Subject");
    m.text_body = strdup("Enqueue body");
    return m;
}

static MailRelayMessage make_invalid_message(void) {
    // Has a From but no recipients -> fails validation.
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    m.subject = strdup("No Recipients");
    m.text_body = strdup("Body");
    return m;
}

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    mailrelay_shutdown();
}

void tearDown(void) {
    mailrelay_shutdown();
    app_config = g_saved_app_config;
}

void test_enqueue_null_message_returns_invalid(void) {
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, mailrelay_enqueue(NULL, 0));
}

void test_enqueue_not_initialized_returns_shutdown(void) {
    // Runtime intentionally left uninitialized.
    MailRelayMessage m = make_valid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_SHUTDOWN, mailrelay_enqueue(&m, 0));
    mailrelay_message_free(&m);
}

void test_enqueue_disabled_returns_disabled(void) {
    g_test_config.mail_relay.Enabled = false;
    app_config = &g_test_config;
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayMessage m = make_valid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_DISABLED, mailrelay_enqueue(&m, 0));
    mailrelay_message_free(&m);
}

void test_enqueue_invalid_message_returns_invalid(void) {
    g_test_config.mail_relay.Enabled = true;
    app_config = &g_test_config;
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayMessage m = make_invalid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, mailrelay_enqueue(&m, 0));
    mailrelay_message_free(&m);
}

void test_enqueue_valid_returns_ok(void) {
    g_test_config.mail_relay.Enabled = true;
    app_config = &g_test_config;
    TEST_ASSERT_TRUE(mailrelay_init());

    size_t before = (size_t)mailrelay_runtime->queued_count;
    MailRelayMessage m = make_valid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));
    TEST_ASSERT_EQUAL_size_t(before + 1, (size_t)mailrelay_runtime->queued_count);
    mailrelay_message_free(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_enqueue_null_message_returns_invalid);
    RUN_TEST(test_enqueue_not_initialized_returns_shutdown);
    RUN_TEST(test_enqueue_disabled_returns_disabled);
    RUN_TEST(test_enqueue_invalid_message_returns_invalid);
    RUN_TEST(test_enqueue_valid_returns_ok);
    return UNITY_END();
}
