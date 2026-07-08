/*
 * Unity Test File: mailrelay_events_test.c
 *
 * Tests for the Mail Relay event emission API.
 * Covers enabled/disabled state, rate limiting, built-in system event handlers,
 * and handler return-table dispatch. Uses the event dispatcher seam to avoid
 * requiring a live database or SMTP server.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_events.h>
#include <src/mailrelay/mailrelay_internal.h>

#include <stdlib.h>
#include <string.h>

// Forward declarations
void test_event_emit_disabled_subsystem(void);
void test_event_emit_disabled_events(void);
void test_event_emit_unknown_event(void);
void test_event_emit_rate_limit_blocks_burst(void);
void test_event_emit_server_started_dispatches(void);
void test_event_emit_server_stopped_dispatches(void);
void test_event_emit_no_admin_recipients_no_send(void);
void test_event_emit_params_forwarded(void);

static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;

static char g_server_name[] = "hydrogen-events-test";
static char g_default_from[] = "sender@example.com";
static char g_admin1[] = "admin1@example.com";
static char g_admin2[] = "admin2@example.com";

static MailRelaySendTemplateRequest g_captured_request = {0};
static int g_dispatch_call_count = 0;

static void free_captured_request(void);

static MailRelayStatus mock_dispatcher(const MailRelaySendTemplateRequest* req,
                                       MailRelaySendTemplateResponse* resp,
                                       char* err,
                                       size_t err_cap) {
    (void)resp;
    g_dispatch_call_count++;

    free_captured_request();
    memset(&g_captured_request, 0, sizeof(g_captured_request));
    if (req->template_key) {
        g_captured_request.template_key = strdup(req->template_key);
    }
    if (req->to_count > 0 && req->to) {
        g_captured_request.to_count = req->to_count;
        char** to_copy = calloc((size_t)req->to_count, sizeof(char*));
        TEST_ASSERT_NOT_NULL(to_copy);
        for (int i = 0; i < req->to_count; i++) {
            to_copy[i] = strdup(req->to[i]);
        }
        g_captured_request.to = (const char* const*)to_copy;
    }
    if (req->params) {
        MailRelayTemplateParams* params_copy = malloc(sizeof(MailRelayTemplateParams));
        TEST_ASSERT_NOT_NULL(params_copy);
        mailrelay_template_params_init(params_copy);
        for (int i = 0; i < req->params->count; i++) {
            mailrelay_template_params_add(params_copy,
                                          req->params->items[i].key,
                                          req->params->items[i].value);
        }
        g_captured_request.params = params_copy;
    }
    g_captured_request.priority = req->priority;

    snprintf(err, err_cap, "mock dispatch");
    return MAILRELAY_OK;
}

static void free_captured_request(void) {
    free((void*)g_captured_request.template_key);
    g_captured_request.template_key = NULL;
    if (g_captured_request.to) {
        char** to = (char**)g_captured_request.to;
        for (int i = 0; i < g_captured_request.to_count; i++) {
            free(to[i]);
        }
        free(to);
        g_captured_request.to = NULL;
    }
    g_captured_request.to_count = 0;
    if (g_captured_request.params) {
        MailRelayTemplateParams* params = (MailRelayTemplateParams*)g_captured_request.params;
        mailrelay_template_params_free(params);
        free(params);
        g_captured_request.params = NULL;
    }
}

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.server.server_name = g_server_name;
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Events.Enabled = true;
    g_test_config.mail_relay.Events.MaxEventsPerInterval = 10;
    g_test_config.mail_relay.Events.EventIntervalSeconds = 60;
    g_test_config.mail_relay.DefaultFrom = g_default_from;
    g_test_config.mail_relay.AdminRecipients[0] = g_admin1;
    g_test_config.mail_relay.AdminRecipients[1] = g_admin2;
    g_test_config.mail_relay.AdminRecipientCount = 2;
    app_config = &g_test_config;

    memset(&g_captured_request, 0, sizeof(g_captured_request));
    g_dispatch_call_count = 0;

    TEST_ASSERT_TRUE(mailrelay_init());
    mailrelay_event_set_dispatcher(mock_dispatcher);
}

void tearDown(void) {
    mailrelay_event_set_dispatcher(NULL);
    mailrelay_shutdown();
    free_captured_request();
    app_config = g_saved_app_config;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_event_emit_disabled_subsystem);
    RUN_TEST(test_event_emit_disabled_events);
    RUN_TEST(test_event_emit_unknown_event);
    RUN_TEST(test_event_emit_rate_limit_blocks_burst);
    RUN_TEST(test_event_emit_server_started_dispatches);
    RUN_TEST(test_event_emit_server_stopped_dispatches);
    RUN_TEST(test_event_emit_no_admin_recipients_no_send);
    RUN_TEST(test_event_emit_params_forwarded);

    return UNITY_END();
}

void test_event_emit_disabled_subsystem(void) {
    g_test_config.mail_relay.Enabled = false;
    bool ok = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(0, g_dispatch_call_count);
}

void test_event_emit_disabled_events(void) {
    g_test_config.mail_relay.Events.Enabled = false;
    bool ok = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(0, g_dispatch_call_count);
}

void test_event_emit_unknown_event(void) {
    bool ok = mailrelay_event_emit("system.unknown_event", NULL);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(0, g_dispatch_call_count);
}

void test_event_emit_rate_limit_blocks_burst(void) {
    g_test_config.mail_relay.Events.MaxEventsPerInterval = 2;
    g_test_config.mail_relay.Events.EventIntervalSeconds = 60;

    bool ok1 = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_TRUE(ok1);
    TEST_ASSERT_EQUAL_INT(1, g_dispatch_call_count);

    bool ok2 = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_TRUE(ok2);
    TEST_ASSERT_EQUAL_INT(2, g_dispatch_call_count);

    bool ok3 = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_FALSE(ok3);
    TEST_ASSERT_EQUAL_INT(2, g_dispatch_call_count);

    // A different event key has its own bucket.
    bool ok4 = mailrelay_event_emit("system.server_stopped", NULL);
    TEST_ASSERT_TRUE(ok4);
    TEST_ASSERT_EQUAL_INT(3, g_dispatch_call_count);
}

void test_event_emit_server_started_dispatches(void) {
    bool ok = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(1, g_dispatch_call_count);
    TEST_ASSERT_NOT_NULL(g_captured_request.template_key);
    TEST_ASSERT_EQUAL_STRING("system.server_started", g_captured_request.template_key);
    TEST_ASSERT_EQUAL_INT(2, g_captured_request.to_count);
    TEST_ASSERT_EQUAL_STRING(g_admin1, g_captured_request.to[0]);
    TEST_ASSERT_EQUAL_STRING(g_admin2, g_captured_request.to[1]);
    TEST_ASSERT_NOT_NULL(g_captured_request.params);
    TEST_ASSERT_NOT_NULL(mailrelay_template_params_get(g_captured_request.params, "SERVER_NAME"));
    TEST_ASSERT_EQUAL_STRING(g_server_name,
                             mailrelay_template_params_get(g_captured_request.params, "SERVER_NAME"));
}

void test_event_emit_server_stopped_dispatches(void) {
    bool ok = mailrelay_event_emit("system.server_stopped", NULL);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(1, g_dispatch_call_count);
    TEST_ASSERT_NOT_NULL(g_captured_request.template_key);
    TEST_ASSERT_EQUAL_STRING("system.server_stopped", g_captured_request.template_key);
    TEST_ASSERT_EQUAL_INT(2, g_captured_request.to_count);
}

void test_event_emit_no_admin_recipients_no_send(void) {
    g_test_config.mail_relay.AdminRecipientCount = 0;
    bool ok = mailrelay_event_emit("system.server_started", NULL);
    TEST_ASSERT_TRUE(ok); // Handler chose not to send; not a failure.
    TEST_ASSERT_EQUAL_INT(0, g_dispatch_call_count);
}

void test_event_emit_params_forwarded(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "CUSTOM", "value"));

    bool ok = mailrelay_event_emit("system.server_started", &params);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(1, g_dispatch_call_count);
    TEST_ASSERT_NOT_NULL(g_captured_request.params);
    TEST_ASSERT_EQUAL_STRING("value",
                             mailrelay_template_params_get(g_captured_request.params, "CUSTOM"));

    mailrelay_template_params_free(&params);
}
