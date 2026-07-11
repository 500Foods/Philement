/*
 * Unity Test File: mailrelay_otp_verify_test.c
 *
 * Phase 8.5: mailrelay_otp_verify with mocked repo executor.
 * No plaintext OTP in errors; exercises 113/114/115/128 paths.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <src/mailrelay/mailrelay_otp.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/utils/utils_time.h>

void test_otp_verify_success(void);
void test_otp_verify_wrong_code_increments(void);
void test_otp_verify_wrong_code_max_attempts(void);
void test_otp_verify_not_found(void);
void test_otp_verify_expired(void);
void test_otp_verify_already_at_max(void);
void test_otp_verify_null_args(void);
void test_otp_verify_disabled(void);
void test_otp_verify_bad_email(void);
void test_otp_verify_lookup_failure(void);

static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;
static mailrelay_repo_execute_fn g_original_executor = NULL;

static int g_get_active_calls = 0;
static int g_consume_calls = 0;
static int g_increment_calls = 0;
static int g_mark_max_calls = 0;
static long long g_last_otp_id = 0;

static bool g_get_active_fail = false;
static bool g_get_active_empty = false;
static bool g_write_fail = false;
static int g_row_attempts = 0;
static int g_row_max_attempts = 5;
static char g_row_hash[MAIL_OTP_HASH_HEX_LEN];
static char g_row_expiry[64];
static long long g_row_otp_id = 99;
static long long g_row_account_id = 5;

static const char* KNOWN_CODE = "123456";

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)params_json;
    TEST_ASSERT_NOT_NULL(callback);

    MailRelayRepoResult result = {0};
    result.status = MAILRELAY_REPO_OK;
    result.affected_rows = 1;

    if (query_ref == MAILRELAY_QREF_OTP_GET_ACTIVE) {
        g_get_active_calls = g_get_active_calls + 1;
        if (g_get_active_fail) {
            result.status = MAILRELAY_REPO_QUERY_ERROR;
            result.error_message = "simulated get_active failure";
            callback(&result, user_data);
            return true;
        }
        if (g_get_active_empty) {
            json_t* data = json_array();
            result.data = data;
            callback(&result, user_data);
            json_decref(data);
            return true;
        }

        json_t* data = json_array();
        json_t* row = json_object();
        json_object_set_new(row, "otp_id", json_integer(g_row_otp_id));
        json_object_set_new(row, "account_id", json_integer(g_row_account_id));
        json_object_set_new(row, "attempts", json_integer(g_row_attempts));
        json_object_set_new(row, "max_attempts", json_integer(g_row_max_attempts));
        json_object_set_new(row, "code_hash", json_string(g_row_hash));
        json_object_set_new(row, "expiry_at", json_string(g_row_expiry));
        json_array_append_new(data, row);
        result.data = data;
        callback(&result, user_data);
        json_decref(data);
        return true;
    }

    if (query_ref == MAILRELAY_QREF_OTP_CONSUME) {
        g_consume_calls = g_consume_calls + 1;
        if (params_json != NULL) {
            json_error_t jerr;
            json_t* p = json_loads(params_json, 0, &jerr);
            if (p != NULL) {
                json_t* integers = json_object_get(p, "INTEGER");
                if (json_is_object(integers)) {
                    json_t* id = json_object_get(integers, "OTP_ID");
                    if (json_is_integer(id)) {
                        g_last_otp_id = json_integer_value(id);
                    }
                }
                json_decref(p);
            }
        }
        if (g_write_fail) {
            result.status = MAILRELAY_REPO_QUERY_ERROR;
            result.error_message = "simulated consume failure";
        }
        callback(&result, user_data);
        return true;
    }

    if (query_ref == MAILRELAY_QREF_OTP_INCREMENT_ATTEMPTS) {
        g_increment_calls = g_increment_calls + 1;
        if (g_write_fail) {
            result.status = MAILRELAY_REPO_QUERY_ERROR;
            result.error_message = "simulated increment failure";
        }
        callback(&result, user_data);
        return true;
    }

    if (query_ref == MAILRELAY_QREF_OTP_MARK_MAX_ATTEMPTS) {
        g_mark_max_calls = g_mark_max_calls + 1;
        if (g_write_fail) {
            result.status = MAILRELAY_REPO_QUERY_ERROR;
            result.error_message = "simulated mark max failure";
        }
        callback(&result, user_data);
        return true;
    }

    TEST_FAIL_MESSAGE("unexpected query_ref in OTP verify mock");
    return false;
}

void setUp(void) {
    g_get_active_calls = 0;
    g_consume_calls = 0;
    g_increment_calls = 0;
    g_mark_max_calls = 0;
    g_last_otp_id = 0;
    g_get_active_fail = false;
    g_get_active_empty = false;
    g_write_fail = false;
    g_row_attempts = 0;
    g_row_max_attempts = 5;
    g_row_otp_id = 99;
    g_row_account_id = 5;

    TEST_ASSERT_TRUE(mailrelay_otp_hash_code(KNOWN_CODE, g_row_hash, sizeof(g_row_hash)));
    format_iso_time(time(NULL) + 600, g_row_expiry, sizeof(g_row_expiry));

    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Otp.Digits = 6;
    g_test_config.mail_relay.Otp.ExpirySeconds = 300;
    g_test_config.mail_relay.Otp.MaxAttempts = 5;
    app_config = &g_test_config;

    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);
}

void tearDown(void) {
    mailrelay_repo_set_executor(g_original_executor);
    app_config = g_saved_app_config;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_otp_verify_success);
    RUN_TEST(test_otp_verify_wrong_code_increments);
    RUN_TEST(test_otp_verify_wrong_code_max_attempts);
    RUN_TEST(test_otp_verify_not_found);
    RUN_TEST(test_otp_verify_expired);
    RUN_TEST(test_otp_verify_already_at_max);
    RUN_TEST(test_otp_verify_null_args);
    RUN_TEST(test_otp_verify_disabled);
    RUN_TEST(test_otp_verify_bad_email);
    RUN_TEST(test_otp_verify_lookup_failure);
    return UNITY_END();
}

void test_otp_verify_success(void) {
    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    err[0] = '\0';

    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_OK, st);
    TEST_ASSERT_EQUAL(1, g_get_active_calls);
    TEST_ASSERT_EQUAL(1, g_consume_calls);
    TEST_ASSERT_EQUAL(0, g_increment_calls);
    TEST_ASSERT_EQUAL(0, g_mark_max_calls);
    TEST_ASSERT_EQUAL(99, (int)resp.otp_id);
    TEST_ASSERT_EQUAL(5, (int)resp.account_id);
    TEST_ASSERT_EQUAL(99, (int)g_last_otp_id);
    TEST_ASSERT_TRUE(strstr(err, KNOWN_CODE) == NULL);
}

void test_otp_verify_wrong_code_increments(void) {
    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_EMAIL_VERIFY;
    req.code = "000000";

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(1, g_get_active_calls);
    TEST_ASSERT_EQUAL(0, g_consume_calls);
    TEST_ASSERT_EQUAL(1, g_increment_calls);
    TEST_ASSERT_EQUAL(0, g_mark_max_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_OTP_INVALID") != NULL);
    TEST_ASSERT_TRUE(strstr(err, "000000") == NULL);
    TEST_ASSERT_TRUE(strstr(err, KNOWN_CODE) == NULL);
}

void test_otp_verify_wrong_code_max_attempts(void) {
    g_row_attempts = 4;
    g_row_max_attempts = 5;

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = "000000";

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(1, g_get_active_calls);
    TEST_ASSERT_EQUAL(0, g_consume_calls);
    TEST_ASSERT_EQUAL(0, g_increment_calls);
    TEST_ASSERT_EQUAL(1, g_mark_max_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_OTP_MAX_ATTEMPTS") != NULL);
    TEST_ASSERT_TRUE(strstr(err, "000000") == NULL);
}

void test_otp_verify_not_found(void) {
    g_get_active_empty = true;

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(0, g_consume_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_OTP_NOT_FOUND") != NULL);
    TEST_ASSERT_TRUE(strstr(err, KNOWN_CODE) == NULL);
}

void test_otp_verify_expired(void) {
    format_iso_time(time(NULL) - 60, g_row_expiry, sizeof(g_row_expiry));

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(0, g_consume_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_OTP_EXPIRED") != NULL);
    TEST_ASSERT_TRUE(strstr(err, KNOWN_CODE) == NULL);
}

void test_otp_verify_already_at_max(void) {
    g_row_attempts = 5;
    g_row_max_attempts = 5;

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_EQUAL(0, g_consume_calls);
    TEST_ASSERT_EQUAL(0, g_increment_calls);
    TEST_ASSERT_EQUAL(0, g_mark_max_calls);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_OTP_MAX_ATTEMPTS") != NULL);
}

void test_otp_verify_null_args(void) {
    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(NULL, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_PARAM_MISSING") != NULL);
    TEST_ASSERT_EQUAL(0, g_get_active_calls);
}

void test_otp_verify_disabled(void) {
    g_test_config.mail_relay.Enabled = false;

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_DISABLED, st);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_DISABLED") != NULL);
    TEST_ASSERT_EQUAL(0, g_get_active_calls);
}

void test_otp_verify_bad_email(void) {
    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "not-an-email";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_INVALID_ARGS, st);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_RECIPIENT_INVALID") != NULL);
    TEST_ASSERT_EQUAL(0, g_get_active_calls);
}

void test_otp_verify_lookup_failure(void) {
    g_get_active_fail = true;

    MailRelayOtpVerifyRequest req;
    memset(&req, 0, sizeof(req));
    req.email = "user@example.com";
    req.purpose_a66 = MAIL_OTP_PURPOSE_LOGIN_MFA;
    req.code = KNOWN_CODE;

    MailRelayOtpVerifyResponse resp;
    char err[256];
    MailRelayStatus st = mailrelay_otp_verify(&req, &resp, err, sizeof(err));
    TEST_ASSERT_EQUAL(MAILRELAY_PERSIST_FAILED, st);
    TEST_ASSERT_TRUE(strstr(err, "MAIL_PERSIST_FAILED") != NULL);
    TEST_ASSERT_TRUE(strstr(err, KNOWN_CODE) == NULL);
}
