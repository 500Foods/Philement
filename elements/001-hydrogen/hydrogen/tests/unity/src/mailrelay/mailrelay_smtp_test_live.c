/*
 * Live (integration) test for mailrelay_smtp: performs a REAL libcurl send to a
 * running SMTP sink. Gated by MAILRELAY_LIVE_SMTP=host:port; skips cleanly when
 * unset so it stays safe in the normal unit-test battery (e.g. with `mku`).
 *
 * Drive it manually with the project's mailval fixture:
 *   cd extras/mailval && cmake -S . -B build && cmake --build build &&
 *     ./build/mailval --smtp-port 5570 --data-dir /tmp/mailval &
 *   MAILRELAY_LIVE_SMTP=127.0.0.1:5570 mku mailrelay_smtp_live_test
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#include <unity.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_live_send_to_sink(void) {
    const char* target = getenv("MAILRELAY_LIVE_SMTP");
    if (!target || !*target) {
        TEST_PASS_MESSAGE("MAILRELAY_LIVE_SMTP not set; skipping live SMTP send");
        return;
    }

    char host[256] = {0};
    char* port = NULL;
    strncpy(host, target, sizeof(host) - 1);
    port = strchr(host, ':');
    if (port) {
        *port = '\0';
        port++;
    }

    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "live-test@hydrogen.local");
    mailrelay_message_add_to(&m, "sink@hydrogen.local");
    m.subject = strdup("Live SMTP Send");
    m.text_body = strdup("Delivered by mailrelay_smtp via libcurl.");

    OutboundServer s;
    memset(&s, 0, sizeof(s));
    s.Host = host;
    s.Port = port ? port : (char*)"25";
    s.TLSMode = MAIL_TLS_MODE_NONE;
    s.AuthMode = MAIL_AUTH_MODE_NONE;
    s.TimeoutSeconds = 10;

    MailRelayResult out;
    bool ok = mailrelay_smtp_send(&m, &s, NULL, "hydrogen", &out);

    mailrelay_message_free(&m);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(250, out.smtp_code);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_live_send_to_sink);
    return UNITY_END();
}
