/*
 * Unity Test File: mailrelay_render_test.c
 *
 * Tests for the Mail Relay RFC 5322 renderer.
 * Uses test seams to make Date and Message-ID deterministic.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay_render.h>
#include <src/mailrelay/mailrelay_test_seams.h>
#include <src/mailrelay/mailrelay_message.h>

// Deterministic seam implementations for testing
static time_t test_fixed_time;
static char test_fixed_mid[256];

static time_t fixed_time_seam(void) {
    return test_fixed_time;
}

static void fixed_mid_seam(char* buffer, size_t buflen, const char* app_name) {
    (void)snprintf(buffer, buflen, "%s@%s", test_fixed_mid, app_name && app_name[0] ? app_name : "hydrogen");
}

void setUp(void) {
    test_fixed_time = 1700000000;
    snprintf(test_fixed_mid, sizeof(test_fixed_mid), "test-mid-001");
    mailrelay_set_seam_time(fixed_time_seam);
    mailrelay_set_seam_message_id(fixed_mid_seam);
}

void tearDown(void) {
    mailrelay_reset_seams();
}

static MailRelayMessage build_message(const char* from, const char* reply_to, const char* subject,
                                      const char* text, const char* html,
                                      const char* const* to, int to_count,
                                      const char* const* cc, int cc_count,
                                      const char* const* bcc, int bcc_count) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    if (from) mailrelay_message_set_from(&m, from);
    if (reply_to) mailrelay_message_set_reply_to(&m, reply_to);
    if (to) {
        for (int i = 0; i < to_count; i++) {
            if (to[i]) mailrelay_message_add_to(&m, to[i]);
        }
    }
    if (cc) {
        for (int i = 0; i < cc_count; i++) {
            if (cc[i]) mailrelay_message_add_cc(&m, cc[i]);
        }
    }
    if (bcc) {
        for (int i = 0; i < bcc_count; i++) {
            if (bcc[i]) mailrelay_message_add_bcc(&m, bcc[i]);
        }
    }
    m.subject = subject ? strdup(subject) : NULL;
    m.text_body = text ? strdup(text) : NULL;
    m.html_body = html ? strdup(html) : NULL;
    return m;
}

static bool render_and_check(const MailRelayMessage* m, const char* default_from, const char* app_name, char** out) {
    bool ok = mailrelay_render_message(m, default_from, app_name, out);
    if (ok) {
        TEST_ASSERT_NOT_NULL(*out);
    }
    return ok;
}

// Forward declarations
void test_render_null_message_fails(void);
void test_render_no_from_no_default_fails(void);
void test_render_text_only(void);
void test_render_html_only(void);
void test_render_multipart(void);
void test_render_header_order(void);
void test_render_uses_crlf(void);
void test_render_bcc_not_in_headers(void);
void test_render_cc_present(void);
void test_render_uses_default_from(void);
void test_render_deterministic_boundary(void);
void test_render_deterministic_message_id(void);

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_render_null_message_fails);
    RUN_TEST(test_render_no_from_no_default_fails);
    RUN_TEST(test_render_text_only);
    RUN_TEST(test_render_html_only);
    RUN_TEST(test_render_multipart);
    RUN_TEST(test_render_header_order);
    RUN_TEST(test_render_uses_crlf);
    RUN_TEST(test_render_bcc_not_in_headers);
    RUN_TEST(test_render_cc_present);
    RUN_TEST(test_render_uses_default_from);
    RUN_TEST(test_render_deterministic_boundary);
    RUN_TEST(test_render_deterministic_message_id);

    return UNITY_END();
}

void test_render_null_message_fails(void) {
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_render_message(NULL, "from@example.com", "app", &out));
}

void test_render_no_from_no_default_fails(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.subject = strdup("Hello");
    m.text_body = strdup("Body");
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_render_message(&m, NULL, "app", &out));
    mailrelay_message_free(&m);
}

void test_render_text_only(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text body", NULL, to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "Content-Type: text/plain"));
    TEST_ASSERT_NULL(strstr(out, "multipart/alternative"));
    TEST_ASSERT_NOT_NULL(strstr(out, "text body"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_html_only(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", NULL, "<p>html</p>", to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "Content-Type: text/html"));
    TEST_ASSERT_NULL(strstr(out, "multipart/alternative"));
    TEST_ASSERT_NOT_NULL(strstr(out, "<p>html</p>"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_multipart(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text body", "<p>html</p>", to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "multipart/alternative"));
    TEST_ASSERT_NOT_NULL(strstr(out, "Content-Type: text/plain"));
    TEST_ASSERT_NOT_NULL(strstr(out, "Content-Type: text/html"));
    TEST_ASSERT_NOT_NULL(strstr(out, "text body"));
    TEST_ASSERT_NOT_NULL(strstr(out, "<p>html</p>"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_header_order(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", "reply@example.com", "Subject", "text", NULL, to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    const char* date_pos = strstr(out, "Date: ");
    const char* from_pos = strstr(out, "From: ");
    const char* reply_pos = strstr(out, "Reply-To: ");
    const char* to_pos = strstr(out, "To: ");
    const char* subject_pos = strstr(out, "Subject: ");
    const char* mid_pos = strstr(out, "Message-ID: ");
    TEST_ASSERT_NOT_NULL(date_pos);
    TEST_ASSERT_NOT_NULL(from_pos);
    TEST_ASSERT_NOT_NULL(reply_pos);
    TEST_ASSERT_NOT_NULL(to_pos);
    TEST_ASSERT_NOT_NULL(subject_pos);
    TEST_ASSERT_NOT_NULL(mid_pos);
    TEST_ASSERT_TRUE(date_pos < from_pos);
    TEST_ASSERT_TRUE(from_pos < reply_pos);
    TEST_ASSERT_TRUE(reply_pos < to_pos);
    TEST_ASSERT_TRUE(to_pos < subject_pos);
    TEST_ASSERT_TRUE(subject_pos < mid_pos);
    free(out);
    mailrelay_message_free(&m);
}

void test_render_uses_crlf(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text", NULL, to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    const char* p = out;
    bool has_lf_only = false;
    while (*p) {
        if (*p == '\n' && (p == out || *(p - 1) != '\r')) {
            has_lf_only = true;
            break;
        }
        p++;
    }
    TEST_ASSERT_FALSE(has_lf_only);
    free(out);
    mailrelay_message_free(&m);
}

void test_render_bcc_not_in_headers(void) {
    const char* to[] = {"to@example.com"};
    const char* bcc[] = {"bcc@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text", NULL, to, 1, NULL, 0, bcc, 1);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NULL(strstr(out, "Bcc:"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_cc_present(void) {
    const char* to[] = {"to@example.com"};
    const char* cc[] = {"cc@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text", NULL, to, 1, cc, 1, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "Cc: cc@example.com"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_uses_default_from(void) {
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message(NULL, NULL, "Subject", "text", NULL, to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, "default@example.com", "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "From: default@example.com"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_deterministic_boundary(void) {
    test_fixed_mid[0] = '\0';
    snprintf(test_fixed_mid, sizeof(test_fixed_mid), "fixed-boundary-mid");
    
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text part", "<p>html part</p>", to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "app", &out));
    TEST_ASSERT_NOT_NULL(strstr(out, "multipart/alternative"));
    TEST_ASSERT_NOT_NULL(strstr(out, "boundary=\"--fixed-boundary-midapp\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "--fixed-boundary-midapp\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(out, "--fixed-boundary-midapp--"));
    free(out);
    mailrelay_message_free(&m);
}

void test_render_deterministic_message_id(void) {
    test_fixed_mid[0] = '\0';
    snprintf(test_fixed_mid, sizeof(test_fixed_mid), "my-custom-mid");
    
    const char* to[] = {"to@example.com"};
    MailRelayMessage m = build_message("from@example.com", NULL, "Subject", "text", NULL, to, 1, NULL, 0, NULL, 0);
    char* out = NULL;
    TEST_ASSERT_TRUE(render_and_check(&m, NULL, "myapp", &out));
    char expected[256];
    snprintf(expected, sizeof(expected), "Message-ID: my-custom-mid@myapp");
    TEST_ASSERT_NOT_NULL(strstr(out, expected));
    free(out);
    mailrelay_message_free(&m);
}
