/*
 * Unity Test File: mailrelay_otp_send_test.c
 *
 * Phase 8.4: generate_and_send with mocked repo insert and send_template.
 * Asserts no plaintext OTP in logs/errors; hash-only insert params.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_otp.h>
#include <src/mailrelay/mailrelay_repository.h>

void test_otp_send_success(void);
void test_otp_send_rejects_bad_email(void);
void test_otp_send_rejects_bad_purpose(void);
void test_otp_send_disabled(void);
void test_otp_send_insert_failure(void);
void test_otp_send_template_failure(void);

static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;
static mailrelay_repo_execute_fn g_original_executor = NULL;

static int g_insert_calls = 0;
static int g_send_calls = 0;
static char g_last_code_hash[MAIL_OTP_HASH_HEX_LEN];
static char g_last_email[128];
static char g_last_otp_seen_in_send[MAIL_OTP_MAX_DIGITS + 1];
static bool g_insert_should_fail = false;
static bool g_send_should_fail = false;
static unsigned char g_fake_fill = 7;

static bool fake_random_fill(unsigned char* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = g_fake_fill;
    }
    return true;
}

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    TEST_ASSERT_NOT_NULL(callback);

    MailRelayRepoResult result = {0};
    result.status = MAILRELAY_REPO_OK;
    result.affected_rows = 1;

    if (query_ref == MAILRELAY_QREF_OTP_INSERT) {
        g_insert_calls = g_insert_calls + 1;
        if (params_json != NULL) {
            json_error_t jerr;
            json_t* p = json_loads(params_json, 0, &jerr);
            if (p != NULL) {
                json_t* strings = json_object_get(p, "STRING");
                if (json_is_object(strings)) {
                    json_t* h = json_object_get(strings, "CODE_HASH");
                    json_t* e = json_object_get(strings, "EMAIL");
                    if (json_is_string(h)) {
                        snprintf(g_last_code_hash, sizeof(g_last_code_hash), "%s",
                                 json_string_value(h));
                    }
                    if (json_is_string(e)) {
                        snprintf(g_last_email, sizeof(g_last_email), "%s",
                                 json_string_value(e));
                    }
                }
                json_decref(p);
            }
        }

        if (g_insert_should_fail) {
            result.status = MAILRELAY_REPO_QUERY_ERROR;
            result.error_message = "simulated insert failure";
            callback(&result, user_data);
            return true;
        }

        json_t* data = json_array();
        json_t* row = json_object();
        json_object_set_new(row, "otp_id", json_integer(42));
        json_array_append_new(data, row);
        result.data = data;
        callback(&result, user_data);
        json_decref(data);
        return true;
    }

    TEST_FAIL_MESSAGE("unexpected query_ref in OTP send mock");
    return false;
}

static MailRelayStatus mock_send_template(const MailRelaySendTemplateRequest* req,
                                          MailRelaySendTemplateResponse* resp,
                                          char* err,
                                          size_t err_cap) {
    g_send_calls = g_send_calls + 1;
    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_NOT_NULL(resp);

    if (req->otp_code != NULL) {
        snprintf(g_last_otp_seen_in_send, sizeof(g_last_otp_seen_in_send), "%s",
                 req->otp_code);
    } else {
        g_last_otp_seen_in_send[0] = '\0';
    }

    TEST_ASSERT_EQUAL_STRING(MAIL_OTP_TEMPLATE_KEY, req->template_key);
    TEST_ASSERT_EQUAL(1, req->to_count);

    if (g_send_should_fail) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_TEMPLATE_NOT_FOUND: simulated");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_send_template_response_init(resp);
    resp->message_id = strdup("msg-otp-test");
    resp->status = strdup("queued");
    return MAILRELAY_OK;
}

void setUp(void) {
    g_insert_calls = 0;
    g_send_calls = 0;
    g_last_code_hash[0] = '\0';
    g_last_email[0] = '\0';
    g_last_otp_seen_in_send[0] = '\0';
    g_insert_should_fail = false;
    g_send_should_fail = false;
    g_fake_fill = 7;

    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Otp.Digits = 6;
    g_test_config.mail_relay.Otp.ExpirySeconds = 300;
    g_test_config.mail_relay.Otp.MaxAttempts = 5;
    static char server_name[] = "test-server";
    g_test_config.server.server_name = server_name;
    app_config = &g_test_config;

    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);
    mailrelay_send_template_set_fn(mock_send_template);
    mailrelay_otp_set_random_fn(fake_random_fill);
}

void tearDown(void) {
    mailrelay_otp_set_random_fn(NULL);
    mailrelay_send_template_set_fn(NULL);
    mailrelay_repo_set_executor(g_original_executor);
    app_config = g_saved_app_config;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_otp_send_success);
    RUN_TEST(test_otp_send_rejects_bad_email);
    RUN_TEST(test_otp_send_rejects_bad_purpose);
    RUN_TEST(test_otp_send_disabled);
    RUN_TEST(test_otp_send_insert_failure);
    RUN_TEST(test_otp_send_template_failure);
    return UNITY_END();
}

void test_otp_send_success(void) {
    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.account_id = 5;
    req.app_name = "Hydrogen";

    MailRelayOtpSendResponse resp;
    mailrelay_otp_send_response_init(&resp);
    char err[256];
    err[0] = '\0';

    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_OK, st);
    TEST_ASSERT_EQUAL(1, g_insert_calls);
    TEST_ASSERT_EQUAL(1, g_send_calls);
    TEST_ASSERT_EQUAL(42, (int)resp.otp_id);
    TEST_ASSERT_EQUAL_STRING("msg-otp-test", resp.message_id);
    TEST_ASSERT_EQUAL_STRING("queued", resp.status);
    TEST_ASSERT_EQUAL_STRING("user@example.com", g_last_email);

    /* Known hash of "777777" (fake_fill=7, six digits) */
    char expected_hash[MAIL_OTP_HASH_HEX_LEN];
    TEST_ASSERT_TRUE(mailrelay_otp_hash_code("777777", expected_hash, sizeof(expected_hash)));
    TEST_ASSERT_EQUAL_STRING(expected_hash, g_last_code_hash);
    TEST_ASSERT_EQUAL_STRING("777777", g_last_otp_seen_in_send);

    /* err must not contain the code */
    TEST_ASSERT_TRUE(strstr(err, "777777") == NULL);

    mailrelay_otp_send_response_free(&resp);
}

void test_otp_send_rejects_bad_email(void) {
    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "not-an-email";
    req.purpose_a66 = MAIL_OTP_PURPOSE_EMAIL_VERIFY;

    MailRelayOtpSendResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(0, g_insert_calls);
    TEST_ASSERT_EQUAL(0, g_send_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_RECIPIENT_INVALID") != NULL);
}

void test_otp_send_rejects_bad_purpose(void) {
    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = 99;

    MailRelayOtpSendResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(0, g_insert_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_PARAM_MISSING") != NULL);
}

void test_otp_send_disabled(void) {
    g_test_config.mail_relay.Enabled = false;

    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;

    MailRelayOtpSendResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_DISABLED, st);
    TEST_ASSERT_EQUAL(0, g_insert_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_DISABLED") != NULL);
}

void test_otp_send_insert_failure(void) {
    g_insert_should_fail = true;

    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;

    MailRelayOtpSendResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_PERSIST_FAILED, st);
    TEST_ASSERT_EQUAL(1, g_insert_calls);
    TEST_ASSERT_EQUAL(0, g_send_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_PERSIST_FAILED") != NULL);
    TEST_ASSERT_TRUE(strstr(err, "777777") == NULL);
}

void test_otp_send_template_failure(void) {
    g_send_should_fail = true;

    MailRelayOtpSendRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;

    MailRelayOtpSendResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_generate_and_send(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(1, g_insert_calls);
    TEST_ASSERT_EQUAL(1, g_send_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_TEMPLATE_NOT_FOUND") != NULL);
    TEST_ASSERT_TRUE(strstr(err, "777777") == NULL);
}
