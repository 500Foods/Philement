/*
 * Unity Test File: mailrelay_producer_test.c
 *
 * Tests for the Mail Relay templated-send producer API.
 * Covers successful enqueue, idempotency, template not found, validation
 * failures, and disabled state. Uses a mock repository executor and a
 * real in-memory queue initialized via mailrelay_init().
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_repository.h>

// Third-party includes
#include <jansson.h>

#include <stdlib.h>
#include <string.h>

// Forward declarations
void test_mailrelay_send_template_success(void);
void test_mailrelay_send_template_idempotency(void);
void test_mailrelay_send_template_not_found(void);
void test_mailrelay_send_template_missing_recipient(void);
void test_mailrelay_send_template_missing_from(void);
void test_mailrelay_send_template_disabled(void);

static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;
static mailrelay_repo_execute_fn g_original_executor = NULL;
static json_t* g_template_result = NULL;
static json_t* g_idempotency_result = NULL;
static int g_idempotency_lookup_count = 0;

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)params_json;
    TEST_ASSERT_NOT_NULL(callback);

    MailRelayRepoResult result = { 0 };
    result.status = MAILRELAY_REPO_OK;
    result.affected_rows = 1;

    if (query_ref == MAILRELAY_QREF_TEMPLATE_GET_BY_KEY) {
        result.data = g_template_result;
        callback(&result, user_data);
        return true;
    }

    if (query_ref == MAILRELAY_QREF_QUEUE_GET_BY_IDEMPOTENCY) {
        g_idempotency_lookup_count++;
        result.data = g_idempotency_result;
        callback(&result, user_data);
        return true;
    }

    TEST_FAIL_MESSAGE("unexpected query_ref in mock executor");
    return false;
}

static void set_template_result(const char* subject, const char* text, const char* html) {
    if (g_template_result) {
        json_decref(g_template_result);
    }
    g_template_result = json_array();
    json_t* row = json_object();
    json_object_set_new(row, "subject_template", json_string(subject));
    if (text) {
        json_object_set_new(row, "text_template", json_string(text));
    }
    if (html) {
        json_object_set_new(row, "html_template", json_string(html));
    }
    json_array_append_new(g_template_result, row);
}

static void set_idempotency_not_found(void) {
    if (g_idempotency_result) {
        json_decref(g_idempotency_result);
    }
    g_idempotency_result = json_array();
}

static void set_idempotency_found(const char* message_uuid, int status_a63) {
    if (g_idempotency_result) {
        json_decref(g_idempotency_result);
    }
    g_idempotency_result = json_array();
    json_t* row = json_object();
    json_object_set_new(row, "message_uuid", json_string(message_uuid));
    json_object_set_new(row, "status_a63", json_integer(status_a63));
    json_array_append_new(g_idempotency_result, row);
}

static char g_default_from[] = "sender@example.com";
static char g_default_reply_to[] = "reply@example.com";

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.MaxInMemory = 10;
    g_test_config.mail_relay.DefaultFrom = g_default_from;
    g_test_config.mail_relay.DefaultReplyTo = g_default_reply_to;
    app_config = &g_test_config;

    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);

    set_template_result("Hello %NAME%", "Text: %NAME%", "<p>HTML: %NAME%</p>");
    set_idempotency_not_found();
    g_idempotency_lookup_count = 0;

    TEST_ASSERT_TRUE(mailrelay_init());
}

void tearDown(void) {
    mailrelay_shutdown();
    mailrelay_repo_set_executor(g_original_executor);
    app_config = g_saved_app_config;
    if (g_template_result) {
        json_decref(g_template_result);
        g_template_result = NULL;
    }
    if (g_idempotency_result) {
        json_decref(g_idempotency_result);
        g_idempotency_result = NULL;
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mailrelay_send_template_success);
    RUN_TEST(test_mailrelay_send_template_idempotency);
    RUN_TEST(test_mailrelay_send_template_not_found);
    RUN_TEST(test_mailrelay_send_template_missing_recipient);
    RUN_TEST(test_mailrelay_send_template_missing_from);
    RUN_TEST(test_mailrelay_send_template_disabled);

    return UNITY_END();
}

void test_mailrelay_send_template_success(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    const char* to[] = { "to@example.com" };
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "mail.test";
    req.to = to;
    req.to_count = 1;
    req.params = &params;
    req.priority = 5;
    req.app_name = "Hydrogen";

    MailRelaySendTemplateResponse resp = { 0 };
    char err[256] = { 0 };

    MailRelayStatus status = mailrelay_send_template(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
    TEST_ASSERT_NOT_NULL(resp.message_id);
    TEST_ASSERT_EQUAL_STRING("queued", resp.status);
    TEST_ASSERT_TRUE(strlen(resp.message_id) > 0);

    mailrelay_send_template_response_free(&resp);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_send_template_idempotency(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    const char* to[] = { "to@example.com" };
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "mail.test";
    req.to = to;
    req.to_count = 1;
    req.params = &params;
    req.idempotency_key = "unique-key-123";
    req.priority = 5;

    MailRelaySendTemplateResponse resp1 = { 0 };
    char err1[256] = { 0 };
    MailRelayStatus status1 = mailrelay_send_template(&req, &resp1, err1, sizeof(err1));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status1);
    TEST_ASSERT_NOT_NULL(resp1.message_id);
    TEST_ASSERT_EQUAL_STRING("queued", resp1.status);

    // Second call with the same idempotency key should return the existing message.
    set_idempotency_found(resp1.message_id, 2); /* status_a63 = sent */
    MailRelaySendTemplateResponse resp2 = { 0 };
    char err2[256] = { 0 };
    MailRelayStatus status2 = mailrelay_send_template(&req, &resp2, err2, sizeof(err2));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status2);
    TEST_ASSERT_EQUAL_STRING(resp1.message_id, resp2.message_id);
    TEST_ASSERT_EQUAL_STRING("sent", resp2.status);
    TEST_ASSERT_EQUAL_INT(2, g_idempotency_lookup_count);

    mailrelay_send_template_response_free(&resp1);
    mailrelay_send_template_response_free(&resp2);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_send_template_not_found(void) {
    set_template_result("", NULL, NULL); /* Will not match; preview sees empty subject as missing */
    /* Actually, make the template result empty so preview returns not found. */
    if (g_template_result) {
        json_decref(g_template_result);
    }
    g_template_result = json_array();

    const char* to[] = { "to@example.com" };
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "missing.template";
    req.to = to;
    req.to_count = 1;
    req.priority = 5;

    MailRelaySendTemplateResponse resp = { 0 };
    char err[256] = { 0 };
    MailRelayStatus status = mailrelay_send_template(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, status);
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_TEMPLATE_NOT_FOUND"));
    TEST_ASSERT_NULL(resp.message_id);
    TEST_ASSERT_NULL(resp.status);
}

void test_mailrelay_send_template_missing_recipient(void) {
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "mail.test";
    req.priority = 5;

    MailRelaySendTemplateResponse resp = { 0 };
    char err[256] = { 0 };
    MailRelayStatus status = mailrelay_send_template(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, status);
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_RECIPIENT_INVALID"));
}

void test_mailrelay_send_template_missing_from(void) {
    g_test_config.mail_relay.DefaultFrom = NULL;

    const char* to[] = { "to@example.com" };
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "mail.test";
    req.to = to;
    req.to_count = 1;
    req.priority = 5;

    MailRelaySendTemplateResponse resp = { 0 };
    char err[256] = { 0 };
    MailRelayStatus status = mailrelay_send_template(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_INVALID_ARGS, status);
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_PARAM_MISSING"));
}

void test_mailrelay_send_template_disabled(void) {
    g_test_config.mail_relay.Enabled = false;

    const char* to[] = { "to@example.com" };
    MailRelaySendTemplateRequest req = { 0 };
    req.template_key = "mail.test";
    req.to = to;
    req.to_count = 1;
    req.priority = 5;

    MailRelaySendTemplateResponse resp = { 0 };
    char err[256] = { 0 };
    MailRelayStatus status = mailrelay_send_template(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT(MAILRELAY_DISABLED, status);
}
