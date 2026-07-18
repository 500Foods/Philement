/*
 * Unity Test File: mailrelay preview helper coverage
 * Tests parse_template_params(), parse_preview_request_json() and
 * parse_producer_error() from src/api/mailrelay/preview/preview.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/api/mailrelay/preview/preview.h>
#include <src/mailrelay/mailrelay_template.h>

/* Forward declarations */
bool parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                           char* err, size_t err_cap);
bool parse_preview_request_json(json_t* request_json,
                                const char** template_key_out,
                                MailRelayTemplateParams* params,
                                char* err, size_t err_cap);
void parse_producer_error(const char* producer_err,
                          char* code, size_t code_cap,
                          char* message, size_t message_cap,
                          unsigned int* http_status);

void test_parse_template_params_null_obj(void);
void test_parse_template_params_not_object(void);
void test_parse_template_params_non_string_value(void);
void test_parse_template_params_valid(void);
void test_parse_preview_request_json_null_args(void);
void test_parse_preview_request_json_missing_key(void);
void test_parse_preview_request_json_key_too_long(void);
void test_parse_preview_request_json_bad_params(void);
void test_parse_preview_request_json_valid_no_params(void);
void test_parse_preview_request_json_valid_with_params(void);
void test_parse_producer_error_null(void);
void test_parse_producer_error_empty(void);
void test_parse_producer_error_no_colon(void);
void test_parse_producer_error_code_truncated(void);
void test_parse_producer_error_template_not_found(void);
void test_parse_producer_error_param_missing(void);
void test_parse_producer_error_recipient_invalid(void);
void test_parse_producer_error_queue_full(void);
void test_parse_producer_error_disabled(void);
void test_parse_producer_error_rate_limited(void);
void test_parse_producer_error_unknown(void);
void test_parse_producer_error_leading_space_in_message(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

/* ===========================================================================
 * parse_template_params
 * =========================================================================== */

void test_parse_template_params_null_obj(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    /* NULL obj is tolerated and returns true without touching params. */
    TEST_ASSERT_TRUE(parse_template_params(NULL, &params, NULL, 0));
    mailrelay_template_params_free(&params);
}

void test_parse_template_params_not_object(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    json_t* arr = json_array();
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_template_params(arr, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'params' must be an object of strings", err);
    json_decref(arr);
    mailrelay_template_params_free(&params);
}

void test_parse_template_params_non_string_value(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    json_t* obj = json_object();
    json_object_set_new(obj, "NAME", json_integer(5));
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_template_params(obj, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'params.NAME' must be a string", err);
    json_decref(obj);
    mailrelay_template_params_free(&params);
}

void test_parse_template_params_valid(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    json_t* obj = json_object();
    json_object_set_new(obj, "NAME", json_string("Ada"));
    json_object_set_new(obj, "CITY", json_string("Paris"));
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_template_params(obj, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Ada", mailrelay_template_params_get(&params, "NAME"));
    TEST_ASSERT_EQUAL_STRING("Paris", mailrelay_template_params_get(&params, "CITY"));
    json_decref(obj);
    mailrelay_template_params_free(&params);
}

/* ===========================================================================
 * parse_preview_request_json
 * =========================================================================== */

void test_parse_preview_request_json_null_args(void) {
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_preview_request_json(NULL, NULL, NULL, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Invalid arguments", err);

    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    json_t* req = json_object();
    TEST_ASSERT_FALSE(parse_preview_request_json(req, &tk, &params, NULL, 0));
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_preview_request_json_missing_key(void) {
    json_t* req = json_object();
    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_preview_request_json(req, &tk, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: template_key is required", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_preview_request_json_key_too_long(void) {
    json_t* req = json_object();
    char long_key[400];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    json_object_set_new(req, "template_key", json_string(long_key));
    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_preview_request_json(req, &tk, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: template_key is too long", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_preview_request_json_bad_params(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "params", json_string("not-an-object"));
    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_preview_request_json(req, &tk, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'params' must be an object of strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_preview_request_json_valid_no_params(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_preview_request_json(req, &tk, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("mail.test", tk);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_preview_request_json_valid_with_params(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_t* p = json_object();
    json_object_set_new(p, "NAME", json_string("Ada"));
    json_object_set_new(req, "params", p);
    const char* tk = NULL;
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_preview_request_json(req, &tk, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("mail.test", tk);
    TEST_ASSERT_EQUAL_STRING("Ada", mailrelay_template_params_get(&params, "NAME"));
    json_decref(req);
    mailrelay_template_params_free(&params);
}

/* ===========================================================================
 * parse_producer_error
 * =========================================================================== */

static void check_mapping(const char* producer_err,
                          const char* expected_code,
                          const char* expected_message,
                          unsigned int expected_status) {
    char code[64] = {0};
    char message[512] = {0};
    unsigned int status = 0;
    parse_producer_error(producer_err, code, sizeof(code), message, sizeof(message), &status);
    TEST_ASSERT_EQUAL_STRING(expected_code, code);
    TEST_ASSERT_EQUAL_STRING(expected_message, message);
    TEST_ASSERT_EQUAL(expected_status, status);
}

void test_parse_producer_error_null(void) {
    check_mapping(NULL, "MAIL_INTERNAL_ERROR", "Unknown mail relay error",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_parse_producer_error_empty(void) {
    check_mapping("", "MAIL_INTERNAL_ERROR", "Unknown mail relay error",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_parse_producer_error_no_colon(void) {
    check_mapping("some opaque error text", "MAIL_INTERNAL_ERROR", "some opaque error text",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_parse_producer_error_code_truncated(void) {
    /* code longer than code_cap gets truncated to fit. */
    char code[8] = {0};
    char message[512] = {0};
    unsigned int status = 0;
    parse_producer_error("MAIL_TEMPLATE_NOT_FOUND: missing", code, sizeof(code),
                         message, sizeof(message), &status);
    TEST_ASSERT_EQUAL_STRING("MAIL_TE", code);
    TEST_ASSERT_EQUAL_STRING("missing", message);
    /* Truncated code no longer matches a known prefix, so it falls to 500. */
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, status);
}

void test_parse_producer_error_template_not_found(void) {
    check_mapping("MAIL_TEMPLATE_NOT_FOUND: Template not found or inactive",
                  "MAIL_TEMPLATE_NOT_FOUND", "Template not found or inactive",
                  MHD_HTTP_NOT_FOUND);
}

void test_parse_producer_error_param_missing(void) {
    check_mapping("MAIL_PARAM_MISSING: template_key is required",
                  "MAIL_PARAM_MISSING", "template_key is required",
                  MHD_HTTP_BAD_REQUEST);
}

void test_parse_producer_error_recipient_invalid(void) {
    check_mapping("MAIL_RECIPIENT_INVALID: bad address",
                  "MAIL_RECIPIENT_INVALID", "bad address",
                  MHD_HTTP_BAD_REQUEST);
}

void test_parse_producer_error_queue_full(void) {
    check_mapping("MAIL_QUEUE_FULL: queue at capacity",
                  "MAIL_QUEUE_FULL", "queue at capacity",
                  MHD_HTTP_SERVICE_UNAVAILABLE);
}

void test_parse_producer_error_disabled(void) {
    check_mapping("MAIL_DISABLED: mail relay is disabled",
                  "MAIL_DISABLED", "mail relay is disabled",
                  MHD_HTTP_SERVICE_UNAVAILABLE);
}

void test_parse_producer_error_rate_limited(void) {
    check_mapping("MAIL_RATE_LIMITED: too many requests",
                  "MAIL_RATE_LIMITED", "too many requests", 429);
}

void test_parse_producer_error_unknown(void) {
    check_mapping("MAIL_SOMETHING_ELSE: weird",
                  "MAIL_SOMETHING_ELSE", "weird",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_parse_producer_error_leading_space_in_message(void) {
    /* The code strips ALL leading spaces after the colon, so the message is
     * trimmed to the first non-space character. */
    check_mapping("MAIL_PARAM_MISSING:  indented",
                  "MAIL_PARAM_MISSING", "indented",
                  MHD_HTTP_BAD_REQUEST);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_template_params_null_obj);
    RUN_TEST(test_parse_template_params_not_object);
    RUN_TEST(test_parse_template_params_non_string_value);
    RUN_TEST(test_parse_template_params_valid);

    RUN_TEST(test_parse_preview_request_json_null_args);
    RUN_TEST(test_parse_preview_request_json_missing_key);
    RUN_TEST(test_parse_preview_request_json_key_too_long);
    RUN_TEST(test_parse_preview_request_json_bad_params);
    RUN_TEST(test_parse_preview_request_json_valid_no_params);
    RUN_TEST(test_parse_preview_request_json_valid_with_params);

    RUN_TEST(test_parse_producer_error_null);
    RUN_TEST(test_parse_producer_error_empty);
    RUN_TEST(test_parse_producer_error_no_colon);
    RUN_TEST(test_parse_producer_error_code_truncated);
    RUN_TEST(test_parse_producer_error_template_not_found);
    RUN_TEST(test_parse_producer_error_param_missing);
    RUN_TEST(test_parse_producer_error_recipient_invalid);
    RUN_TEST(test_parse_producer_error_queue_full);
    RUN_TEST(test_parse_producer_error_disabled);
    RUN_TEST(test_parse_producer_error_rate_limited);
    RUN_TEST(test_parse_producer_error_unknown);
    RUN_TEST(test_parse_producer_error_leading_space_in_message);

    return UNITY_END();
}
