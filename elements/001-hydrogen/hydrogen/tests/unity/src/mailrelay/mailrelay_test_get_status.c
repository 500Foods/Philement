/*
 * Unity Test File: mailrelay_test_get_status.c
 *
 * Tests the public mailrelay_get_status() function that snapshots the Mail
 * Relay runtime counters. Covers the NULL-counters guard, the not-initialized
 * path (which still reports the enabled flag from app_config), and the
 * fully-initialized path which populates every counter field.
 */

// Project header + Unity framework
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_get_status_null_counters_returns_false(void);
void test_get_status_not_initialized_returns_false(void);
void test_get_status_not_initialized_reports_enabled_flag(void);
void test_get_status_initialized_populates_counters(void);

// Saved global config so each test can restore app_config afterwards.
static AppConfig* g_saved_app_config = NULL;
static AppConfig g_test_config = { 0 };

static MailRelayMessage make_valid_message(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Status Subject");
    m.text_body = strdup("Status body");
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

void test_get_status_null_counters_returns_false(void) {
    TEST_ASSERT_FALSE(mailrelay_get_status(NULL));
}

void test_get_status_not_initialized_returns_false(void) {
    // Runtime intentionally left uninitialized.
    MailRelayStatusCounters counters = { 0 };
    bool ok = mailrelay_get_status(&counters);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(counters.initialized);
}

void test_get_status_not_initialized_reports_enabled_flag(void) {
    g_test_config.mail_relay.Enabled = true;
    app_config = &g_test_config;

    MailRelayStatusCounters counters = { 0 };
    bool ok = mailrelay_get_status(&counters);

    // Not initialized, but the enabled flag is still copied from app_config.
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(counters.initialized);
    TEST_ASSERT_TRUE(counters.enabled);
}

void test_get_status_initialized_populates_counters(void) {
    g_test_config.mail_relay.Enabled = true;
    app_config = &g_test_config;

    // Seed a queued message so the runtime counters are non-zero.
    TEST_ASSERT_TRUE(mailrelay_init());
    MailRelayMessage m = make_valid_message();
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, mailrelay_enqueue(&m, 0));
    mailrelay_message_free(&m);

    MailRelayStatusCounters counters = { 0 };
    bool ok = mailrelay_get_status(&counters);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(counters.initialized);
    TEST_ASSERT_TRUE(counters.enabled);
    TEST_ASSERT_EQUAL_size_t(1, counters.queued);
    TEST_ASSERT_EQUAL_INT(0, counters.sent);
    TEST_ASSERT_EQUAL_INT(0, counters.failed);

    mailrelay_shutdown();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_status_null_counters_returns_false);
    RUN_TEST(test_get_status_not_initialized_returns_false);
    RUN_TEST(test_get_status_not_initialized_reports_enabled_flag);
    RUN_TEST(test_get_status_initialized_populates_counters);
    return UNITY_END();
}
