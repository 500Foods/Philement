/*
 * Unity Test File: fanout_entry Function Tests
 * Tests src/logging/log_fanout.c :: fanout_entry()
 *
 * Exercises JSON parsing and per-destination gating. Mailrelay delivery is
 * intercepted via the mailrelay_send_direct seam; console output is silenced
 * during each test by redirecting the underlying file descriptor (fd 1) to
 * /dev/null and restoring it in tearDown, so Unity's summary still prints to
 * the real stdout.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>
#include <src/config/config_defaults.h>
#include <src/mailrelay/mailrelay.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int g_calls = 0;
static int g_saved_stdout = -1;

/*
 * Redirect fd 1 to /dev/null for the duration of a test. We save the original
 * fd so tearDown can restore it before UNITY_END() prints the summary, which
 * would otherwise be lost if stdout itself were rebound with freopen.
 */
static void silence_stdout(void) {
    if (g_saved_stdout != -1) return;
    g_saved_stdout = dup(1);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull != -1) {
        dup2(devnull, 1);
        close(devnull);
    }
}

static void restore_stdout(void) {
    if (g_saved_stdout == -1) return;
    dup2(g_saved_stdout, 1);
    close(g_saved_stdout);
    g_saved_stdout = -1;
}

static MailRelayStatus mock_send_direct(const MailRelaySendDirectRequest* req,
                                        MailRelaySendTemplateResponse* resp,
                                        char* err,
                                        size_t err_cap) {
    (void)req; (void)resp; (void)err; (void)err_cap;
    g_calls++;
    return MAILRELAY_OK;
}

static AppConfig g_config;
static char g_admin_addr[64];

static void test_fanout_entry_null(void);
static void test_fanout_entry_empty(void);
static void test_fanout_entry_invalid_json(void);
static void test_fanout_entry_notify_routes_to_mailrelay(void);
static void test_fanout_entry_no_notify_no_mailrelay(void);

void setUp(void) {
    memset(&g_config, 0, sizeof(g_config));
    initialize_config_defaults(&g_config);
    app_config = &g_config;
    g_calls = 0;
    memset(g_admin_addr, 0, sizeof(g_admin_addr));
    strncpy(g_admin_addr, "admin@example.com", sizeof(g_admin_addr) - 1);
    mailrelay_send_direct_set_fn(mock_send_direct);
    silence_stdout();
}

void tearDown(void) {
    restore_stdout();
    mailrelay_send_direct_set_fn(NULL);
    app_config = NULL;
}

void test_fanout_entry_null(void) {
    fanout_entry(NULL);
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_entry_empty(void) {
    fanout_entry("");
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_entry_invalid_json(void) {
    fanout_entry("this is not json");
    TEST_ASSERT_EQUAL(0, g_calls);
}

void test_fanout_entry_notify_routes_to_mailrelay(void) {
    g_config.mail_relay.Enabled = true;
    g_config.mail_relay.AdminRecipientCount = 1;
    g_config.mail_relay.AdminRecipients[0] = g_admin_addr;
    g_config.logging.notify.enabled = true;
    g_config.logging.notify.default_level = LOG_LEVEL_TRACE;

    const char* msg =
        "{\"subsystem\":\"Test\",\"details\":\"alert!\",\"priority\":4,"
        "\"LogConsole\":false,\"LogFile\":false,\"LogNotify\":true}";

    fanout_entry(msg);

    TEST_ASSERT_EQUAL(1, g_calls);
}

void test_fanout_entry_no_notify_no_mailrelay(void) {
    const char* msg =
        "{\"subsystem\":\"Test\",\"details\":\"hi\",\"priority\":2,"
        "\"LogConsole\":true,\"LogFile\":false,\"LogNotify\":false}";

    fanout_entry(msg);

    TEST_ASSERT_EQUAL(0, g_calls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fanout_entry_null);
    RUN_TEST(test_fanout_entry_empty);
    RUN_TEST(test_fanout_entry_invalid_json);
    RUN_TEST(test_fanout_entry_notify_routes_to_mailrelay);
    RUN_TEST(test_fanout_entry_no_notify_no_mailrelay);
    return UNITY_END();
}
