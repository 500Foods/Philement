/*
 * Unity Test File: mailrelay send helper coverage
 * Tests parse_string_array(), mailrelay_send_parse_template_params(), parse_send_request_json()
 * and mailrelay_send_parse_producer_error() from src/api/mailrelay/send/send.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/api/mailrelay/send/send.h>
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_template.h>

/* Forward declarations */
bool parse_string_array(json_t* arr, const char* field_name,
                        const char* const** out_items, int* out_count,
                        char* err, size_t err_cap);
bool mailrelay_send_parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                           char* err, size_t err_cap);
bool parse_send_request_json(json_t* request_json,
                             MailRelaySendTemplateRequest* req,
                             MailRelayTemplateParams* params,
                             const char* const** to_arr,
                             const char* const** cc_arr,
                             const char* const** bcc_arr,
                             char* err, size_t err_cap);
void mailrelay_send_parse_producer_error(const char* producer_err,
                          char* code, size_t code_cap,
                          char* message, size_t message_cap,
                          unsigned int* http_status);

void test_parse_string_array_null(void);
void test_parse_string_array_not_array(void);
void test_parse_string_array_empty(void);
void test_parse_string_array_too_many(void);
void test_parse_string_array_non_string(void);
void test_parse_string_array_valid(void);
void test_parse_template_params_null_obj(void);
void test_parse_template_params_not_object(void);
void test_parse_template_params_non_string_value(void);
void test_parse_template_params_valid(void);
void test_parse_send_request_json_null_args(void);
void test_parse_send_request_json_missing_template_key(void);
void test_parse_send_request_json_template_key_too_long(void);
void test_parse_send_request_json_missing_idempotency_key(void);
void test_parse_send_request_json_idempotency_key_too_long(void);
void test_parse_send_request_json_to_not_array(void);
void test_parse_send_request_json_cc_not_array(void);
void test_parse_send_request_json_bcc_not_array(void);
void test_parse_send_request_json_no_recipients(void);
void test_parse_send_request_json_invalid_recipient(void);
void test_parse_send_request_json_bad_params(void);
void test_parse_send_request_json_priority_not_integer(void);
void test_parse_send_request_json_valid_minimal(void);
void test_parse_send_request_json_valid_full(void);
void test_mailrelay_send_parse_producer_error_null(void);
void test_mailrelay_send_parse_producer_error_empty(void);
void test_mailrelay_send_parse_producer_error_no_colon(void);
void test_mailrelay_send_parse_producer_error_code_truncated(void);
void test_mailrelay_send_parse_producer_error_template_not_found(void);
void test_mailrelay_send_parse_producer_error_param_missing(void);
void test_mailrelay_send_parse_producer_error_recipient_invalid(void);
void test_mailrelay_send_parse_producer_error_queue_full(void);
void test_mailrelay_send_parse_producer_error_disabled(void);
void test_mailrelay_send_parse_producer_error_rate_limited(void);
void test_mailrelay_send_parse_producer_error_unknown(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

/* ===========================================================================
 * parse_string_array
 * =========================================================================== */

void test_parse_string_array_null(void) {
    const char* const* items = (const char* const*)0xDEAD;
    int count = -1;
    TEST_ASSERT_TRUE(parse_string_array(NULL, "to", &items, &count, NULL, 0));
    TEST_ASSERT_NULL(items);
    TEST_ASSERT_EQUAL(0, count);
}

void test_parse_string_array_not_array(void) {
    json_t* obj = json_object();
    const char* const* items = NULL;
    int count = 0;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_string_array(obj, "to", &items, &count, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'to' must be an array of strings", err);
    json_decref(obj);
}

void test_parse_string_array_empty(void) {
    json_t* arr = json_array();
    const char* const* items = (const char* const*)0xDEAD;
    int count = -1;
    TEST_ASSERT_TRUE(parse_string_array(arr, "to", &items, &count, NULL, 0));
    TEST_ASSERT_NULL(items);
    TEST_ASSERT_EQUAL(0, count);
    json_decref(arr);
}

void test_parse_string_array_too_many(void) {
    json_t* arr = json_array();
    for (int i = 0; i <= 256; i++) {
        json_array_append_new(arr, json_string("a@b.com"));
    }
    const char* const* items = NULL;
    int count = 0;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_string_array(arr, "to", &items, &count, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'to' exceeds maximum of 256 items", err);
    json_decref(arr);
}

void test_parse_string_array_non_string(void) {
    json_t* arr = json_array();
    json_array_append_new(arr, json_integer(5));
    const char* const* items = NULL;
    int count = 0;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_string_array(arr, "to", &items, &count, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'to' must contain only strings", err);
    json_decref(arr);
}

void test_parse_string_array_valid(void) {
    json_t* arr = json_array();
    json_array_append_new(arr, json_string("a@b.com"));
    json_array_append_new(arr, json_string("c@d.com"));
    const char* const* items = NULL;
    int count = 0;
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_string_array(arr, "to", &items, &count, err, sizeof(err)));
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("a@b.com", items[0]);
    TEST_ASSERT_EQUAL_STRING("c@d.com", items[1]);
    json_decref(arr);
}

/* ===========================================================================
 * mailrelay_send_parse_template_params
 * =========================================================================== */

void test_parse_template_params_null_obj(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_send_parse_template_params(NULL, &params, NULL, 0));
    mailrelay_template_params_free(&params);
}

void test_parse_template_params_not_object(void) {
    MailRelayTemplateParams params = {0};
    mailrelay_template_params_init(&params);
    json_t* arr = json_array();
    char err[128] = {0};
    TEST_ASSERT_FALSE(mailrelay_send_parse_template_params(arr, &params, err, sizeof(err)));
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
    TEST_ASSERT_FALSE(mailrelay_send_parse_template_params(obj, &params, err, sizeof(err)));
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
    TEST_ASSERT_TRUE(mailrelay_send_parse_template_params(obj, &params, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Ada", mailrelay_template_params_get(&params, "NAME"));
    TEST_ASSERT_EQUAL_STRING("Paris", mailrelay_template_params_get(&params, "CITY"));
    json_decref(obj);
    mailrelay_template_params_free(&params);
}

/* ===========================================================================
 * parse_send_request_json
 * =========================================================================== */

void test_parse_send_request_json_null_args(void) {
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(NULL, NULL, NULL, NULL, NULL, NULL, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Invalid arguments", err);

    MailRelaySendTemplateRequest req = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to = NULL;
    const char* const* cc = NULL;
    const char* const* bcc = NULL;
    json_t* r = json_object();
    TEST_ASSERT_FALSE(parse_send_request_json(r, &req, &params, &to, &cc, &bcc, NULL, 0));
    json_decref(r);
}

void test_parse_send_request_json_missing_template_key(void) {
    json_t* req = json_object();
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: template_key is required", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_template_key_too_long(void) {
    json_t* req = json_object();
    char long_key[400];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    json_object_set_new(req, "template_key", json_string(long_key));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: template_key is too long", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_missing_idempotency_key(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: idempotency_key is required", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_idempotency_key_too_long(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    char long_key[600];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    json_object_set_new(req, "idempotency_key", json_string(long_key));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: idempotency_key is too long", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_to_not_array(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_object_set_new(req, "to", json_string("not-an-array"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'to' must be an array of strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_cc_not_array(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);
    json_object_set_new(req, "cc", json_string("not-an-array"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'cc' must be an array of strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_bcc_not_array(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);
    json_object_set_new(req, "bcc", json_string("not-an-array"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'bcc' must be an array of strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_no_recipients(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("MAIL_RECIPIENT_INVALID: at least one recipient is required", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_invalid_recipient(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    /* A non-string element is rejected by parse_string_array. */
    json_array_append_new(to, json_integer(42));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'to' must contain only strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_bad_params(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);
    json_object_set_new(req, "params", json_string("not-an-object"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'params' must be an object of strings", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_priority_not_integer(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);
    json_object_set_new(req, "priority", json_string("high"));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_FALSE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Field 'priority' must be an integer", err);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_valid_minimal(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_send_request_json(req, &r, &params, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("mail.test", r.template_key);
    TEST_ASSERT_EQUAL_STRING("key-1", r.idempotency_key);
    TEST_ASSERT_EQUAL(1, r.to_count);
    TEST_ASSERT_EQUAL(0, r.priority);
    TEST_ASSERT_NULL(r.from);
    TEST_ASSERT_NULL(r.reply_to);
    json_decref(req);
    mailrelay_template_params_free(&params);
}

void test_parse_send_request_json_valid_full(void) {
    json_t* req = json_object();
    json_object_set_new(req, "template_key", json_string("mail.test"));
    json_object_set_new(req, "idempotency_key", json_string("key-1"));
    json_t* to = json_array();
    json_array_append_new(to, json_string("to@example.com"));
    json_object_set_new(req, "to", to);
    json_t* cc = json_array();
    json_array_append_new(cc, json_string("cc@example.com"));
    json_object_set_new(req, "cc", cc);
    json_t* bcc = json_array();
    json_array_append_new(bcc, json_string("bcc@example.com"));
    json_object_set_new(req, "bcc", bcc);
    json_t* params = json_object();
    json_object_set_new(params, "NAME", json_string("Ada"));
    json_object_set_new(req, "params", params);
    json_object_set_new(req, "priority", json_integer(5));

    MailRelaySendTemplateRequest r = {0};
    MailRelayTemplateParams p = {0};
    const char* const* to_a = NULL;
    const char* const* cc_a = NULL;
    const char* const* bcc_a = NULL;
    char err[128] = {0};
    TEST_ASSERT_TRUE(parse_send_request_json(req, &r, &p, &to_a, &cc_a, &bcc_a, err, sizeof(err)));
    TEST_ASSERT_EQUAL(1, r.to_count);
    TEST_ASSERT_EQUAL(1, r.cc_count);
    TEST_ASSERT_EQUAL(1, r.bcc_count);
    TEST_ASSERT_EQUAL(5, r.priority);
    TEST_ASSERT_EQUAL_STRING("Ada", mailrelay_template_params_get(&p, "NAME"));
    json_decref(req);
    mailrelay_template_params_free(&p);
}

/* ===========================================================================
 * mailrelay_send_parse_producer_error
 * =========================================================================== */

static void check_mapping(const char* producer_err,
                          const char* expected_code,
                          const char* expected_message,
                          unsigned int expected_status) {
    char code[64] = {0};
    char message[512] = {0};
    unsigned int status = 0;
    mailrelay_send_parse_producer_error(producer_err, code, sizeof(code), message, sizeof(message), &status);
    TEST_ASSERT_EQUAL_STRING(expected_code, code);
    TEST_ASSERT_EQUAL_STRING(expected_message, message);
    TEST_ASSERT_EQUAL(expected_status, status);
}

void test_mailrelay_send_parse_producer_error_null(void) {
    check_mapping(NULL, "MAIL_INTERNAL_ERROR", "Unknown mail relay error",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_mailrelay_send_parse_producer_error_empty(void) {
    check_mapping("", "MAIL_INTERNAL_ERROR", "Unknown mail relay error",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_mailrelay_send_parse_producer_error_no_colon(void) {
    check_mapping("some opaque error text", "MAIL_INTERNAL_ERROR", "some opaque error text",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

void test_mailrelay_send_parse_producer_error_code_truncated(void) {
    char code[8] = {0};
    char message[512] = {0};
    unsigned int status = 0;
    mailrelay_send_parse_producer_error("MAIL_TEMPLATE_NOT_FOUND: missing", code, sizeof(code),
                         message, sizeof(message), &status);
    TEST_ASSERT_EQUAL_STRING("MAIL_TE", code);
    TEST_ASSERT_EQUAL_STRING("missing", message);
    /* Truncated code no longer matches a known prefix, so it falls to 500. */
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, status);
}

void test_mailrelay_send_parse_producer_error_template_not_found(void) {
    check_mapping("MAIL_TEMPLATE_NOT_FOUND: Template not found or inactive",
                  "MAIL_TEMPLATE_NOT_FOUND", "Template not found or inactive",
                  MHD_HTTP_NOT_FOUND);
}

void test_mailrelay_send_parse_producer_error_param_missing(void) {
    check_mapping("MAIL_PARAM_MISSING: template_key is required",
                  "MAIL_PARAM_MISSING", "template_key is required",
                  MHD_HTTP_BAD_REQUEST);
}

void test_mailrelay_send_parse_producer_error_recipient_invalid(void) {
    check_mapping("MAIL_RECIPIENT_INVALID: bad address",
                  "MAIL_RECIPIENT_INVALID", "bad address",
                  MHD_HTTP_BAD_REQUEST);
}

void test_mailrelay_send_parse_producer_error_queue_full(void) {
    check_mapping("MAIL_QUEUE_FULL: queue at capacity",
                  "MAIL_QUEUE_FULL", "queue at capacity",
                  MHD_HTTP_SERVICE_UNAVAILABLE);
}

void test_mailrelay_send_parse_producer_error_disabled(void) {
    check_mapping("MAIL_DISABLED: mail relay is disabled",
                  "MAIL_DISABLED", "mail relay is disabled",
                  MHD_HTTP_SERVICE_UNAVAILABLE);
}

void test_mailrelay_send_parse_producer_error_rate_limited(void) {
    check_mapping("MAIL_RATE_LIMITED: too many requests",
                  "MAIL_RATE_LIMITED", "too many requests", 429);
}

void test_mailrelay_send_parse_producer_error_unknown(void) {
    check_mapping("MAIL_SOMETHING_ELSE: weird",
                  "MAIL_SOMETHING_ELSE", "weird",
                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_array_null);
    RUN_TEST(test_parse_string_array_not_array);
    RUN_TEST(test_parse_string_array_empty);
    RUN_TEST(test_parse_string_array_too_many);
    RUN_TEST(test_parse_string_array_non_string);
    RUN_TEST(test_parse_string_array_valid);

    RUN_TEST(test_parse_template_params_null_obj);
    RUN_TEST(test_parse_template_params_not_object);
    RUN_TEST(test_parse_template_params_non_string_value);
    RUN_TEST(test_parse_template_params_valid);

    RUN_TEST(test_parse_send_request_json_null_args);
    RUN_TEST(test_parse_send_request_json_missing_template_key);
    RUN_TEST(test_parse_send_request_json_template_key_too_long);
    RUN_TEST(test_parse_send_request_json_missing_idempotency_key);
    RUN_TEST(test_parse_send_request_json_idempotency_key_too_long);
    RUN_TEST(test_parse_send_request_json_to_not_array);
    RUN_TEST(test_parse_send_request_json_cc_not_array);
    RUN_TEST(test_parse_send_request_json_bcc_not_array);
    RUN_TEST(test_parse_send_request_json_no_recipients);
    RUN_TEST(test_parse_send_request_json_invalid_recipient);
    RUN_TEST(test_parse_send_request_json_bad_params);
    RUN_TEST(test_parse_send_request_json_priority_not_integer);
    RUN_TEST(test_parse_send_request_json_valid_minimal);
    RUN_TEST(test_parse_send_request_json_valid_full);

    RUN_TEST(test_mailrelay_send_parse_producer_error_null);
    RUN_TEST(test_mailrelay_send_parse_producer_error_empty);
    RUN_TEST(test_mailrelay_send_parse_producer_error_no_colon);
    RUN_TEST(test_mailrelay_send_parse_producer_error_code_truncated);
    RUN_TEST(test_mailrelay_send_parse_producer_error_template_not_found);
    RUN_TEST(test_mailrelay_send_parse_producer_error_param_missing);
    RUN_TEST(test_mailrelay_send_parse_producer_error_recipient_invalid);
    RUN_TEST(test_mailrelay_send_parse_producer_error_queue_full);
    RUN_TEST(test_mailrelay_send_parse_producer_error_disabled);
    RUN_TEST(test_mailrelay_send_parse_producer_error_rate_limited);
    RUN_TEST(test_mailrelay_send_parse_producer_error_unknown);

    return UNITY_END();
}
