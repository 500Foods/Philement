/*
 * Unit tests for mailrelay_smtp: render + resolved request via a mock transport.
 * The real libcurl transport is exercised separately by the (env-gated) live test.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#include <unity.h>
#include <stdlib.h>
#include <string.h>

static MailRelaySmtpRequest g_req;
static char* g_payload;
static bool g_called;
static bool g_fail;

static bool mock_transport(const MailRelaySmtpRequest* req, MailRelayResult* out) {
    g_called = true;
    g_req = *req;
    free(g_payload);
    g_payload = req->payload ? strdup(req->payload) : NULL;
    g_req.payload = g_payload;
    mailrelay_result_init(out);
    if (g_fail) {
        out->success = false;
        out->smtp_code = 554;
        snprintf(out->smtp_text, sizeof(out->smtp_text), "Transaction failed");
        return false;
    }
    out->success = true;
    out->smtp_code = 250;
    snprintf(out->smtp_text, sizeof(out->smtp_text), "OK");
    return true;
}

static bool recipient_present(const MailRelaySmtpRequest* req, const char* addr) {
    for (int i = 0; i < req->recipient_count; i++) {
        if (req->recipients[i] && strcmp(req->recipients[i], addr) == 0) return true;
    }
    return false;
}

static char SRV_HOST[] = "smtp.example.com";
static char SRV_PORT[] = "587";
static char SRV_USER[] = "user@example.com";
static char SRV_PASS[] = "s3cret";

static MailRelayMessage make_msg(bool with_from) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    if (with_from) mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    mailrelay_message_add_cc(&m, "cc@example.com");
    mailrelay_message_add_bcc(&m, "bcc@example.com");
    m.subject = strdup("Hello Subject");
    m.text_body = strdup("Body line one");
    return m;
}

static void free_msg(MailRelayMessage* m) {
    mailrelay_message_free(m);
}

static OutboundServer make_server(int tls_mode, bool use_tls) {
    OutboundServer s;
    memset(&s, 0, sizeof(s));
    s.Host = SRV_HOST;
    s.Port = SRV_PORT;
    s.UseTLS = use_tls;
    s.TLSMode = tls_mode;
    s.AuthMode = MAIL_AUTH_MODE_NONE;
    s.TimeoutSeconds = 15;
    return s;
}

void setUp(void) {
    g_called = false;
    g_fail = false;
    memset(&g_req, 0, sizeof(g_req));
    mailrelay_smtp_reset_transport();
    mailrelay_smtp_set_transport(mock_transport);
}

void tearDown(void) {
    free(g_payload);
    g_payload = NULL;
    mailrelay_smtp_reset_transport();
}

static void test_send_invokes_transport_and_renders(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_STARTTLS, false);
    MailRelayResult out;
    bool ok = mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_TRUE(g_called);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(250, out.smtp_code);
    TEST_ASSERT_EQUAL_STRING("smtp://smtp.example.com:587", g_req.url);
    TEST_ASSERT_EQUAL_INT(1, g_req.use_ssl);
    TEST_ASSERT_EQUAL_INT(3, g_req.recipient_count);
    TEST_ASSERT_TRUE(recipient_present(&g_req, "to@example.com"));
    TEST_ASSERT_TRUE(recipient_present(&g_req, "cc@example.com"));
    TEST_ASSERT_TRUE(recipient_present(&g_req, "bcc@example.com"));
    TEST_ASSERT_EQUAL_STRING("sender@example.com", g_req.mail_from);
    TEST_ASSERT_NOT_NULL(g_req.payload);
    TEST_ASSERT_NOT_NULL(strstr(g_req.payload, "Subject: Hello Subject"));
    TEST_ASSERT_NOT_NULL(strstr(g_req.payload, "To: to@example.com"));
    TEST_ASSERT_NOT_NULL(strstr(g_req.payload, "Body line one"));
    TEST_ASSERT_NULL(strstr(g_req.payload, "Bcc:"));
    free_msg(&m);
}

static void test_smtps_scheme_is_implicit_tls(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_SMTPS, false);
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_EQUAL_STRING("smtps://smtp.example.com:587", g_req.url);
    TEST_ASSERT_EQUAL_INT(0, g_req.use_ssl);
    free_msg(&m);
}

static void test_tls_none_with_useltls_hint_is_starttls(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, true);
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_EQUAL_STRING("smtp://smtp.example.com:587", g_req.url);
    TEST_ASSERT_EQUAL_INT(1, g_req.use_ssl);
    free_msg(&m);
}

static void test_tls_none_no_hint_is_plaintext(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, false);
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_EQUAL_STRING("smtp://smtp.example.com:587", g_req.url);
    TEST_ASSERT_EQUAL_INT(0, g_req.use_ssl);
    free_msg(&m);
}

static void test_starttls_required_sets_use_ssl_all(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_STARTTLS_REQUIRED, false);
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_EQUAL_INT(2, g_req.use_ssl);
    free_msg(&m);
}

static void test_auth_credentials_and_mode_resolved(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_STARTTLS, false);
    s.Username = SRV_USER;
    s.Password = SRV_PASS;
    s.AuthMode = MAIL_AUTH_MODE_PLAIN;
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_EQUAL_STRING("user@example.com", g_req.username);
    TEST_ASSERT_EQUAL_STRING("s3cret", g_req.password);
    TEST_ASSERT_EQUAL_INT(MAIL_AUTH_MODE_PLAIN, g_req.auth_mode);
    free_msg(&m);
}

static void test_default_from_fallback(void) {
    MailRelayMessage m = make_msg(false); /* no From */
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, false);
    MailRelayResult out;
    mailrelay_smtp_send(&m, &s, "default@example.com", "app", &out);

    TEST_ASSERT_EQUAL_STRING("default@example.com", g_req.mail_from);
    free_msg(&m);
}

static void test_permanent_failure_propagates(void) {
    g_fail = true;
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, false);
    MailRelayResult out;
    bool ok = mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_TRUE(g_called);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(554, out.smtp_code);
    free_msg(&m);
}

static void test_invalid_args_return_false(void) {
    MailRelayMessage m = make_msg(true);
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, false);
    MailRelayResult out;

    TEST_ASSERT_FALSE(mailrelay_smtp_send(NULL, &s, NULL, "app", &out));
    TEST_ASSERT_FALSE(mailrelay_smtp_send(&m, NULL, NULL, "app", &out));
    TEST_ASSERT_FALSE(mailrelay_smtp_send(&m, &s, NULL, "app", NULL));
    free_msg(&m);
}

static void test_render_failure_without_from_returns_false(void) {
    MailRelayMessage m = make_msg(false); /* no From, no default_from */
    OutboundServer s = make_server(MAIL_TLS_MODE_NONE, false);
    MailRelayResult out;
    bool ok = mailrelay_smtp_send(&m, &s, NULL, "app", &out);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(strstr(out.error, "RENDER_FAILED"));
    free_msg(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_invokes_transport_and_renders);
    RUN_TEST(test_smtps_scheme_is_implicit_tls);
    RUN_TEST(test_tls_none_with_useltls_hint_is_starttls);
    RUN_TEST(test_tls_none_no_hint_is_plaintext);
    RUN_TEST(test_starttls_required_sets_use_ssl_all);
    RUN_TEST(test_auth_credentials_and_mode_resolved);
    RUN_TEST(test_default_from_fallback);
    RUN_TEST(test_permanent_failure_propagates);
    RUN_TEST(test_invalid_args_return_false);
    RUN_TEST(test_render_failure_without_from_returns_false);
    return UNITY_END();
}
