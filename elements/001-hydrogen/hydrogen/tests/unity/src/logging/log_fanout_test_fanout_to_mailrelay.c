/*
 * Unity Test File: fanout_to_mailrelay Function Tests
 * Tests src/logging/log_fanout.c :: fanout_to_mailrelay()
 *
 * Uses the mailrelay_send_direct test seam (mailrelay_send_direct_set_fn) to
 * capture the request without touching SMTP.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>
#include <src/config/config_defaults.h>
#include <src/mailrelay/mailrelay.h>

// Capture state shared with the mock seam.
static int g_calls = 0;
static const MailRelaySendDirectRequest* g_last_req = NULL;
static int g_last_to_count = 0;
static char g_last_to0[64];
static const char* g_last_subject = NULL;
static const char* g_last_text = NULL;
static int g_last_priority = 0;

static MailRelayStatus mock_send_direct(const MailRelaySendDirectRequest* req,
                                        MailRelaySendTemplateResponse* resp,
                                        char* err,
                                        size_t err_cap) {
    (void)resp; (void)err; (void)err_cap;
    g_calls++;
    g_last_req = req;
    g_last_to_count = req->to_count;
    g_last_to0[0] = '\0';
    if (req->to && req->to_count > 0 && req->to[0]) {
        strncpy(g_last_to0, req->to[0], sizeof(g_last_to0) - 1);
    }
    g_last_subject = req->subject;
    g_last_text = req->text_body;
    g_last_priority = req->priority;
    return MAILRELAY_OK;
}

static AppConfig g_config;
static char g_admin_addr[64];

static void test_fanout_to_mailrelay_null_app_config(void);
static void test_fanout_to_mailrelay_disabled(void);
static void test_fanout_to_mailrelay_no_recipients(void);
static void test_fanout_to_mailrelay_sends(void);

void setUp(void) {
    memset(&g_config, 0, sizeof(g_config));
    initialize_config_defaults(&g_config);
    app_config = &g_config;
    g_calls = 0;
    g_last_req = NULL;
    g_last_to_count = 0;
    g_last_to0[0] = '\0';
    g_last_subject = NULL;
    g_last_text = NULL;
    g_last_priority = 0;
    memset(g_admin_addr, 0, sizeof(g_admin_addr));
    strncpy(g_admin_addr, "admin@example.com", sizeof(g_admin_addr) - 1);
    mailrelay_send_direct_set_fn(mock_send_direct);
}

void tearDown(void) {
    mailrelay_send_direct_set_fn(NULL);
    app_config = NULL;
}

void test_fanout_to_mailrelay_null_app_config(void) {
    app_config = NULL;
    fanout_to_mailrelay("Sub", "details", LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_to_mailrelay_disabled(void) {
    g_config.mail_relay.Enabled = false;
    g_config.mail_relay.AdminRecipientCount = 1;
    g_config.mail_relay.AdminRecipients[0] = g_admin_addr;
    fanout_to_mailrelay("Sub", "details", LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_to_mailrelay_no_recipients(void) {
    g_config.mail_relay.Enabled = true;
    g_config.mail_relay.AdminRecipientCount = 0;
    fanout_to_mailrelay("Sub", "details", LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_to_mailrelay_sends(void) {
    g_config.mail_relay.Enabled = true;
    g_config.mail_relay.AdminRecipientCount = 1;
    g_config.mail_relay.AdminRecipients[0] = g_admin_addr;

    fanout_to_mailrelay("Subsystem", "something happened", LOG_LEVEL_ERROR);

    TEST_ASSERT_EQUAL(1, g_calls);
    TEST_ASSERT_NOT_NULL(g_last_req);
    TEST_ASSERT_EQUAL(1, g_last_to_count);
    TEST_ASSERT_EQUAL_STRING("admin@example.com", g_last_to0);
    TEST_ASSERT_EQUAL_STRING("Hydrogen log alert", g_last_subject);
    TEST_ASSERT_EQUAL_STRING("something happened", g_last_text);
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, g_last_priority);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fanout_to_mailrelay_null_app_config);
    RUN_TEST(test_fanout_to_mailrelay_disabled);
    RUN_TEST(test_fanout_to_mailrelay_no_recipients);
    RUN_TEST(test_fanout_to_mailrelay_sends);
    return UNITY_END();
}
