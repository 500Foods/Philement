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
