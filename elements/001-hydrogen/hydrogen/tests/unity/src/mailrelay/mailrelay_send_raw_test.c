/*
 * Unit tests for mailrelay_send_raw and mailrelay_init.
 * Verifies the stable producer API surface and its NULL/error/success paths.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

static void reset_mock(void) {
    g_called = false;
    g_fail = false;
    memset(&g_req, 0, sizeof(g_req));
    free(g_payload);
    g_payload = NULL;
    mailrelay_smtp_reset_transport();
    mailrelay_smtp_set_transport(mock_transport);
}

static OutboundServer make_server(void) {
    OutboundServer s;
    memset(&s, 0, sizeof(s));
    s.Host = strdup("smtp.example.com");
    s.Port = strdup("587");
    s.Username = strdup("user@example.com");
    s.Password = strdup("s3cret");
    s.UseTLS = true;
    s.TLSMode = MAIL_TLS_MODE_STARTTLS;
    s.AuthMode = MAIL_AUTH_MODE_PLAIN;
    s.TimeoutSeconds = 15;
    return s;
}

static void free_server(OutboundServer* s) {
    if (!s) return;
    free(s->Host);
    free(s->Port);
    free(s->Username);
    free(s->Password);
    memset(s, 0, sizeof(*s));
}

static MailRelayMessage make_msg(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "sender@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Hello Subject");
    m.text_body = strdup("Body line one");
    return m;
}

static void free_msg(MailRelayMessage* m) {
    mailrelay_message_free(m);
}

void setUp(void) {
    reset_mock();
}

void tearDown(void) {
    free(g_payload);
    g_payload = NULL;
    mailrelay_smtp_reset_transport();
}

static void test_mailrelay_init_returns_true(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
}

static void test_send_raw_success_invokes_smtp(void) {
    MailRelayMessage m = make_msg();
    OutboundServer s = make_server();
    MailRelayResult out;
    bool ok = mailrelay_send_raw(&m, &s, "fallback@example.com", "myapp", &out);

    TEST_ASSERT_TRUE(g_called);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(250, out.smtp_code);
    TEST_ASSERT_EQUAL_STRING("OK", out.smtp_text);
    TEST_ASSERT_EQUAL_STRING("smtp://smtp.example.com:587", g_req.url);
    TEST_ASSERT_EQUAL_INT(1, g_req.use_ssl);
    TEST_ASSERT_EQUAL_STRING("sender@example.com", g_req.mail_from);
    TEST_ASSERT_EQUAL_INT(1, g_req.recipient_count);
    TEST_ASSERT_NOT_NULL(g_req.payload);
    free_msg(&m);
    free_server(&s);
}

static void test_send_raw_null_msg_returns_error(void) {
    OutboundServer s = make_server();
    MailRelayResult out;
    bool ok = mailrelay_send_raw(NULL, &s, NULL, "app", &out);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(g_called);
    TEST_ASSERT_EQUAL_STRING("MAIL_RAW_INVALID_ARGS", out.error);
    free_server(&s);
}

static void test_send_raw_null_server_returns_error(void) {
    MailRelayMessage m = make_msg();
    MailRelayResult out;
    bool ok = mailrelay_send_raw(&m, NULL, NULL, "app", &out);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(g_called);
    TEST_ASSERT_EQUAL_STRING("MAIL_RAW_INVALID_ARGS", out.error);
    free_msg(&m);
}

static void test_send_raw_null_out_returns_error(void) {
    MailRelayMessage m = make_msg();
    OutboundServer s = make_server();
    bool ok = mailrelay_send_raw(&m, &s, NULL, "app", NULL);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(g_called);
    free_msg(&m);
    free_server(&s);
}

static void test_send_raw_transport_failure_propagates(void) {
    MailRelayMessage m = make_msg();
    OutboundServer s = make_server();
    g_fail = true;
    MailRelayResult out;
    bool ok = mailrelay_send_raw(&m, &s, NULL, "app", &out);

    TEST_ASSERT_TRUE(g_called);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(554, out.smtp_code);
    TEST_ASSERT_TRUE(strstr(out.smtp_text, "Transaction failed") != NULL);
    free_msg(&m);
    free_server(&s);
}

static void test_send_raw_fallback_from_when_msg_from_missing(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Subject only");
    m.text_body = strdup("Body");

    OutboundServer s = make_server();
    MailRelayResult out;
    bool ok = mailrelay_send_raw(&m, &s, "fallback@example.com", "myapp", &out);

    TEST_ASSERT_TRUE(g_called);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("fallback@example.com", g_req.mail_from);
    free_msg(&m);
    free_server(&s);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mailrelay_init_returns_true);
    RUN_TEST(test_send_raw_success_invokes_smtp);
    RUN_TEST(test_send_raw_null_msg_returns_error);
    RUN_TEST(test_send_raw_null_server_returns_error);
    RUN_TEST(test_send_raw_null_out_returns_error);
    RUN_TEST(test_send_raw_transport_failure_propagates);
    RUN_TEST(test_send_raw_fallback_from_when_msg_from_missing);
    return UNITY_END();
}
