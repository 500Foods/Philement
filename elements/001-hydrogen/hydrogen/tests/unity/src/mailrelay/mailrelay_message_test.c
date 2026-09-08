/*
 * Unity Test File: mailrelay_message_test.c
 *
 * Tests for the Mail Relay message model and validation.
 * Covers MailRelayMessage init/free, email validation,
 * recipient handling, and message validation rules.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay_message.h>

// Forward declarations
void test_mailrelay_message_init_resets_fields(void);
void test_mailrelay_message_free_nulls_state(void);
void test_mailrelay_is_valid_email_accepts_valid(void);
void test_mailrelay_is_valid_email_rejects_invalid(void);
void test_mailrelay_message_set_from_valid(void);
void test_mailrelay_message_set_from_invalid(void);
void test_mailrelay_message_add_to_recipient(void);
void test_mailrelay_message_add_cc_recipient(void);
void test_mailrelay_message_add_bcc_recipient(void);
void test_mailrelay_message_add_recipient_full_array(void);
void test_mailrelay_message_recipient_count(void);
void test_mailrelay_validate_message_success(void);
void test_mailrelay_validate_message_missing_from(void);
void test_mailrelay_validate_message_no_recipients(void);
void test_mailrelay_validate_message_missing_subject(void);
void test_mailrelay_validate_message_no_body(void);
void test_mailrelay_validate_message_subject_too_long(void);
void test_mailrelay_message_validate_text_only(void);
void test_mailrelay_message_validate_html_only(void);
void test_mailrelay_message_validate_multipart(void);
void test_mailrelay_validate_message_cr_in_from(void);
void test_mailrelay_validate_message_lf_in_from(void);
void test_mailrelay_validate_message_cr_in_subject(void);
void test_mailrelay_validate_message_lf_in_reply_to(void);
void test_mailrelay_validate_message_cr_in_body_rejected(void);
void test_mailrelay_validate_message_text_body_too_large(void);
void test_mailrelay_validate_message_html_body_too_large(void);
void test_mailrelay_is_safe_header_value_rejects_injection(void);

void test_mailrelay_extract_domain_valid(void);
void test_mailrelay_extract_domain_invalid(void);
void test_mailrelay_domain_matches_exact(void);
void test_mailrelay_domain_matches_subdomain(void);
void test_mailrelay_domain_matches_wildcard(void);
void test_mailrelay_domain_matches_no_match(void);
void test_mailrelay_validate_sender_domain_allow_all(void);
void test_mailrelay_validate_sender_domain_allowlist_match(void);
void test_mailrelay_validate_sender_domain_allowlist_reject(void);
void test_mailrelay_validate_sender_domain_blocklist_match(void);
void test_mailrelay_validate_sender_domain_blocklist_reject(void);

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mailrelay_message_init_resets_fields);
    RUN_TEST(test_mailrelay_message_free_nulls_state);
    RUN_TEST(test_mailrelay_is_valid_email_accepts_valid);
    RUN_TEST(test_mailrelay_is_valid_email_rejects_invalid);
    RUN_TEST(test_mailrelay_message_set_from_valid);
    RUN_TEST(test_mailrelay_message_set_from_invalid);
    RUN_TEST(test_mailrelay_message_add_to_recipient);
    RUN_TEST(test_mailrelay_message_add_cc_recipient);
    RUN_TEST(test_mailrelay_message_add_bcc_recipient);
    RUN_TEST(test_mailrelay_message_add_recipient_full_array);
    RUN_TEST(test_mailrelay_message_recipient_count);
    RUN_TEST(test_mailrelay_validate_message_success);
    RUN_TEST(test_mailrelay_validate_message_missing_from);
    RUN_TEST(test_mailrelay_validate_message_no_recipients);
    RUN_TEST(test_mailrelay_validate_message_missing_subject);
    RUN_TEST(test_mailrelay_validate_message_no_body);
    RUN_TEST(test_mailrelay_validate_message_subject_too_long);
    RUN_TEST(test_mailrelay_message_validate_text_only);
    RUN_TEST(test_mailrelay_message_validate_html_only);
    RUN_TEST(test_mailrelay_message_validate_multipart);
    RUN_TEST(test_mailrelay_validate_message_cr_in_from);
    RUN_TEST(test_mailrelay_validate_message_lf_in_from);
    RUN_TEST(test_mailrelay_validate_message_cr_in_subject);
    RUN_TEST(test_mailrelay_validate_message_lf_in_reply_to);
    RUN_TEST(test_mailrelay_validate_message_cr_in_body_rejected);
    RUN_TEST(test_mailrelay_validate_message_text_body_too_large);
    RUN_TEST(test_mailrelay_validate_message_html_body_too_large);
    RUN_TEST(test_mailrelay_is_safe_header_value_rejects_injection);
    RUN_TEST(test_mailrelay_extract_domain_valid);
    RUN_TEST(test_mailrelay_extract_domain_invalid);
    RUN_TEST(test_mailrelay_domain_matches_exact);
    RUN_TEST(test_mailrelay_domain_matches_subdomain);
    RUN_TEST(test_mailrelay_domain_matches_wildcard);
    RUN_TEST(test_mailrelay_domain_matches_no_match);
    RUN_TEST(test_mailrelay_validate_sender_domain_allow_all);
    RUN_TEST(test_mailrelay_validate_sender_domain_allowlist_match);
    RUN_TEST(test_mailrelay_validate_sender_domain_allowlist_reject);
    RUN_TEST(test_mailrelay_validate_sender_domain_blocklist_match);
    RUN_TEST(test_mailrelay_validate_sender_domain_blocklist_reject);

    return UNITY_END();
}

// ===== MESSAGE INIT/FREE =====

void test_mailrelay_message_init_resets_fields(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_NULL(m.from);
    TEST_ASSERT_NULL(m.reply_to);
    TEST_ASSERT_NULL(m.subject);
    TEST_ASSERT_NULL(m.text_body);
    TEST_ASSERT_NULL(m.html_body);
    TEST_ASSERT_NULL(m.idempotency_key);
    TEST_ASSERT_EQUAL(0, m.to_count);
    TEST_ASSERT_EQUAL(0, m.cc_count);
    TEST_ASSERT_EQUAL(0, m.bcc_count);
    TEST_ASSERT_EQUAL(0, m.priority);
}

void test_mailrelay_message_free_nulls_state(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("from@example.com");
    m.subject = strdup("Hello");
    m.text_body = strdup("Text");
    m.to_count = 1;
    m.to[0] = strdup("to@example.com");

    mailrelay_message_free(&m);

    TEST_ASSERT_NULL(m.from);
    TEST_ASSERT_NULL(m.subject);
    TEST_ASSERT_NULL(m.text_body);
    TEST_ASSERT_EQUAL(0, m.to_count);
    TEST_ASSERT_NULL(m.to[0]);
}

// ===== EMAIL VALIDATION =====

void test_mailrelay_is_valid_email_accepts_valid(void) {
    TEST_ASSERT_TRUE(mailrelay_is_valid_email("user@example.com"));
    TEST_ASSERT_TRUE(mailrelay_is_valid_email("john.doe+tag@sub.domain.org"));
    TEST_ASSERT_TRUE(mailrelay_is_valid_email("a@b.c"));
    TEST_ASSERT_TRUE(mailrelay_is_valid_email("user_name-123@test.co.uk"));
}

void test_mailrelay_is_valid_email_rejects_invalid(void) {
    TEST_ASSERT_FALSE(mailrelay_is_valid_email(NULL));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email(""));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("no-at-sign.com"));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("@no-local.org"));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("no-domain@"));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("nodot@domain"));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("no tld@domain.c"));
    TEST_ASSERT_FALSE(mailrelay_is_valid_email("bad char!@example.com"));
}

// ===== FROM / REPLY-TO =====

void test_mailrelay_message_set_from_valid(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_TRUE(mailrelay_message_set_from(&m, "from@example.com"));
    TEST_ASSERT_NOT_NULL(m.from);
    TEST_ASSERT_EQUAL_STRING("from@example.com", m.from);
    mailrelay_message_free(&m);
}

void test_mailrelay_message_set_from_invalid(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_FALSE(mailrelay_message_set_from(&m, "not-an-email"));
    TEST_ASSERT_NULL(m.from);
    mailrelay_message_free(&m);
}

// ===== RECIPIENTS =====

void test_mailrelay_message_add_to_recipient(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_TRUE(mailrelay_message_add_to(&m, "to@example.com"));
    TEST_ASSERT_EQUAL(1, m.to_count);
    TEST_ASSERT_EQUAL_STRING("to@example.com", m.to[0]);
    mailrelay_message_free(&m);
}

void test_mailrelay_message_add_cc_recipient(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_TRUE(mailrelay_message_add_cc(&m, "cc@example.com"));
    TEST_ASSERT_EQUAL(1, m.cc_count);
    TEST_ASSERT_EQUAL_STRING("cc@example.com", m.cc[0]);
    mailrelay_message_free(&m);
}

void test_mailrelay_message_add_bcc_recipient(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_TRUE(mailrelay_message_add_bcc(&m, "bcc@example.com"));
    TEST_ASSERT_EQUAL(1, m.bcc_count);
    TEST_ASSERT_EQUAL_STRING("bcc@example.com", m.bcc[0]);
    mailrelay_message_free(&m);
}

void test_mailrelay_message_add_recipient_full_array(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    for (int i = 0; i < MV_MAX_RECIPIENTS; i++) {
        char addr[64];
        snprintf(addr, sizeof(addr), "to%d@example.com", i);
        TEST_ASSERT_TRUE(mailrelay_message_add_to(&m, addr));
    }
    TEST_ASSERT_EQUAL(MV_MAX_RECIPIENTS, m.to_count);
    TEST_ASSERT_FALSE(mailrelay_message_add_to(&m, "overflow@example.com"));
    TEST_ASSERT_EQUAL(MV_MAX_RECIPIENTS, m.to_count);
    mailrelay_message_free(&m);
}

void test_mailrelay_message_recipient_count(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    TEST_ASSERT_EQUAL(0, mailrelay_message_recipient_count(&m));

    mailrelay_message_add_to(&m, "to@example.com");
    mailrelay_message_add_cc(&m, "cc@example.com");
    mailrelay_message_add_bcc(&m, "bcc@example.com");
    TEST_ASSERT_EQUAL(3, mailrelay_message_recipient_count(&m));
    mailrelay_message_free(&m);
}

// ===== MESSAGE VALIDATION =====

static MailRelayMessage build_full_message(const char* text, const char* html) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "from@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Test Subject");
    if (text) m.text_body = strdup(text);
    if (html) m.html_body = strdup(html);
    return m;
}

void test_mailrelay_validate_message_success(void) {
    MailRelayMessage m = build_full_message("text body", "<p>html</p>");
    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_missing_from(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.from);
    m.from = NULL;
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_no_recipients(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    m.to_count = 0;
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_missing_subject(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.subject);
    m.subject = NULL;
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_no_body(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.text_body);
    m.text_body = NULL;
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_subject_too_long(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.subject);
    m.subject = malloc(1001);
    memset(m.subject, 'X', 999);
    m.subject[999] = '\0';
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_message_validate_text_only(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_message_validate_html_only(void) {
    MailRelayMessage m = build_full_message(NULL, "<p>html</p>");
    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_message_validate_multipart(void) {
    MailRelayMessage m = build_full_message("text body", "<p>html</p>");
    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_cr_in_from(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.from);
    m.from = strdup("from\r\nBcc: attacker@evil.com@example.com");
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    free(m.from);
    m.from = NULL;
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_lf_in_from(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.from);
    m.from = strdup("from\nSubject: injected@example.com");
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    free(m.from);
    m.from = NULL;
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_cr_in_subject(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.subject);
    m.subject = strdup("Hello\r\nBcc: spam@evil.com");
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "CRLF"));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_lf_in_reply_to(void) {
    MailRelayMessage m = build_full_message("text body", NULL);
    free(m.reply_to);
    m.reply_to = strdup("reply\nX-Injected: yes");
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "CRLF"));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_cr_in_body_rejected(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "from@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Subject");
    m.text_body = strdup("line1\r\nline2");
    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_message(&m, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_text_body_too_large(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "from@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Subject");
    m.text_body = malloc(MV_MAX_BODY_LEN + 2);
    memset(m.text_body, 'A', MV_MAX_BODY_LEN + 1);
    m.text_body[MV_MAX_BODY_LEN + 1] = '\0';
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "exceeds maximum"));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_message_html_body_too_large(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    mailrelay_message_set_from(&m, "from@example.com");
    mailrelay_message_add_to(&m, "to@example.com");
    m.subject = strdup("Subject");
    m.html_body = malloc(MV_MAX_BODY_LEN + 2);
    memset(m.html_body, 'B', MV_MAX_BODY_LEN + 1);
    m.html_body[MV_MAX_BODY_LEN + 1] = '\0';
    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_message(&m, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "exceeds maximum"));
    mailrelay_message_free(&m);
}

void test_mailrelay_is_safe_header_value_rejects_injection(void) {
    TEST_ASSERT_TRUE(mailrelay_is_safe_header_value(NULL));
    TEST_ASSERT_TRUE(mailrelay_is_safe_header_value("normal value"));
    TEST_ASSERT_TRUE(mailrelay_is_safe_header_value(""));
    TEST_ASSERT_FALSE(mailrelay_is_safe_header_value("bad\rvalue"));
    TEST_ASSERT_FALSE(mailrelay_is_safe_header_value("bad\nvalue"));
    TEST_ASSERT_FALSE(mailrelay_is_safe_header_value("bad\r\nvalue"));
}

// ===== SENDER DOMAIN EXTRACTION =====

void test_mailrelay_extract_domain_valid(void) {
    char* d = mailrelay_extract_domain("user@example.com");
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_STRING("example.com", d);
    free(d);

    d = mailrelay_extract_domain("a.b@c.d.e");
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_STRING("c.d.e", d);
    free(d);
}

void test_mailrelay_extract_domain_invalid(void) {
    TEST_ASSERT_NULL(mailrelay_extract_domain(NULL));
    TEST_ASSERT_NULL(mailrelay_extract_domain("no-at-sign"));
    TEST_ASSERT_NULL(mailrelay_extract_domain("at@"));
}

// ===== DOMAIN MATCHING =====

void test_mailrelay_domain_matches_exact(void) {
    TEST_ASSERT_TRUE(mailrelay_domain_matches("example.com", "example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("example.com", "other.com"));
}

void test_mailrelay_domain_matches_subdomain(void) {
    TEST_ASSERT_TRUE(mailrelay_domain_matches("mail.example.com", "example.com"));
    TEST_ASSERT_TRUE(mailrelay_domain_matches("a.b.example.com", "example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("example.com", "mail.example.com"));
}

void test_mailrelay_domain_matches_wildcard(void) {
    TEST_ASSERT_TRUE(mailrelay_domain_matches("mail.example.com", "*.example.com"));
    TEST_ASSERT_TRUE(mailrelay_domain_matches("a.mail.example.com", "*.example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("evil.com", "*.example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("notexample.com", "*.example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("example.com", "*.example.com"));
}

void test_mailrelay_domain_matches_no_match(void) {
    TEST_ASSERT_FALSE(mailrelay_domain_matches("evil.com", "example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches(NULL, "example.com"));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("example.com", NULL));
    TEST_ASSERT_FALSE(mailrelay_domain_matches("", "example.com"));
}

// ===== SENDER DOMAIN POLICY =====

void test_mailrelay_validate_sender_domain_allow_all(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("user@anywhere.com");
    m.subject = strdup("Test");
    m.to_count = 1;
    m.to[0] = strdup("to@example.com");
    m.text_body = strdup("body");

    MailRelaySecurity security;
    memset(&security, 0, sizeof(security));
    security.SenderPolicy = MAIL_SENDER_POLICY_ALLOW_ALL;

    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_sender_domain(&m, &security, err, sizeof(err)));
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_sender_domain_allowlist_match(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("user@example.com");
    m.subject = strdup("Test");
    m.to_count = 1;
    m.to[0] = strdup("to@dest.com");
    m.text_body = strdup("body");

    MailRelaySecurity security;
    memset(&security, 0, sizeof(security));
    security.SenderPolicy = MAIL_SENDER_POLICY_ALLOWLIST;
    security.AllowSenders[0] = strdup("example.com");
    security.AllowSenders[1] = strdup("*.trusted.org");
    security.AllowSenderCount = 2;

    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_sender_domain(&m, &security, err, sizeof(err)));

    free(security.AllowSenders[0]);
    free(security.AllowSenders[1]);
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_sender_domain_allowlist_reject(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("user@evil.com");
    m.subject = strdup("Test");
    m.to_count = 1;
    m.to[0] = strdup("to@dest.com");
    m.text_body = strdup("body");

    MailRelaySecurity security;
    memset(&security, 0, sizeof(security));
    security.SenderPolicy = MAIL_SENDER_POLICY_ALLOWLIST;
    security.AllowSenders[0] = strdup("example.com");
    security.AllowSenderCount = 1;

    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_sender_domain(&m, &security, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "not permitted"));

    free(security.AllowSenders[0]);
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_sender_domain_blocklist_match(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("user@example.com");
    m.subject = strdup("Test");
    m.to_count = 1;
    m.to[0] = strdup("to@dest.com");
    m.text_body = strdup("body");

    MailRelaySecurity security;
    memset(&security, 0, sizeof(security));
    security.SenderPolicy = MAIL_SENDER_POLICY_BLOCKLIST;
    security.BlockSenders[0] = strdup("example.com");
    security.BlockSenderCount = 1;

    char err[256];
    TEST_ASSERT_FALSE(mailrelay_validate_sender_domain(&m, &security, err, sizeof(err)));

    free(security.BlockSenders[0]);
    mailrelay_message_free(&m);
}

void test_mailrelay_validate_sender_domain_blocklist_reject(void) {
    MailRelayMessage m;
    mailrelay_message_init(&m);
    m.from = strdup("user@allowed.com");
    m.subject = strdup("Test");
    m.to_count = 1;
    m.to[0] = strdup("to@dest.com");
    m.text_body = strdup("body");

    MailRelaySecurity security;
    memset(&security, 0, sizeof(security));
    security.SenderPolicy = MAIL_SENDER_POLICY_BLOCKLIST;
    security.BlockSenders[0] = strdup("evil.com");
    security.BlockSenderCount = 1;

    char err[256];
    TEST_ASSERT_TRUE(mailrelay_validate_sender_domain(&m, &security, err, sizeof(err)));

    free(security.BlockSenders[0]);
    mailrelay_message_free(&m);
}
