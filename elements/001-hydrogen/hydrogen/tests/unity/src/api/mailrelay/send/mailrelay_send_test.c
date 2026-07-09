/*
 * Unity Test File: Mail Relay Send API Endpoint
 *
 * Tests POST /api/mailrelay/send handler covering method validation, JWT and
 * role checks, request parsing, and producer wiring. Uses mock MHD and JWT
 * helpers plus a mock repository executor so the test runs without a live
 * database.
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Third-party includes
#include <jansson.h>
#include <stdlib.h>
#include <string.h>

// Mocks
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_system.h>

// Module under test and dependencies
#include <src/api/api_utils.h>
#include <src/api/mailrelay/send/send.h>
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_repository.h>

// Forward declarations for test functions
void test_send_wrong_method(void);
void test_send_missing_auth(void);
void test_send_missing_role(void);
void test_send_missing_template_key(void);
void test_send_missing_idempotency_key(void);
void test_send_missing_recipients(void);
void test_send_invalid_recipient(void);
void test_send_success(void);

// Test fixtures and helpers
static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;
static mailrelay_repo_execute_fn g_original_executor = NULL;
static json_t* g_template_result = NULL;
static json_t* g_idempotency_result = NULL;

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

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)params_json;
    TEST_ASSERT_NOT_NULL(callback);

    MailRelayRepoResult result = {0};
    result.status = MAILRELAY_REPO_OK;
    result.affected_rows = 1;

    if (query_ref == MAILRELAY_QREF_TEMPLATE_GET_BY_KEY) {
        result.data = g_template_result;
        callback(&result, user_data);
        return true;
    }

    if (query_ref == MAILRELAY_QREF_QUEUE_GET_BY_IDEMPOTENCY) {
        result.data = g_idempotency_result;
        callback(&result, user_data);
        return true;
    }

    TEST_FAIL_MESSAGE("unexpected query_ref in mock executor");
    return false;
}

static void setup_valid_jwt(const char* roles) {
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->database = strdup("testdb");
        result.claims->roles = roles ? strdup(roles) : NULL;
        result.claims->email = strdup("user@example.com");
        result.claims->jti = strdup("request-123");
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);

    if (result.claims) {
        free(result.claims->database);
        free(result.claims->roles);
        free(result.claims->email);
        free(result.claims->jti);
        free(result.claims->username);
        free(result.claims);
    }
}

// Simulate the MHD two-call pattern: first call buffers POST data, second
// call processes the complete request.
static enum MHD_Result call_send_handler(struct MHD_Connection* conn,
                                         const char* method,
                                         const char* body) {
    size_t body_len = body ? strlen(body) : 0;
    void* con_cls = NULL;

    // First call: buffer data.
    size_t upload_size = body_len;
    enum MHD_Result r1 = handle_mailrelay_send_request(conn, "/api/mailrelay/send",
                                                         method, body, &upload_size, &con_cls);
    if (r1 != MHD_YES) {
        api_free_post_buffer(&con_cls);
        return r1;
    }

    // Second call: process the complete buffer.
    size_t final_size = 0;
    const char* empty = "";
    enum MHD_Result r2 = handle_mailrelay_send_request(conn, "/api/mailrelay/send",
                                                         method, empty, &final_size, &con_cls);

    api_free_post_buffer(&con_cls);
    return r2;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();

    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.MaxInMemory = 10;
    g_test_config.mail_relay.DefaultFrom = strdup("sender@example.com");
    g_test_config.server.server_name = strdup("test-server");
    app_config = &g_test_config;

    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);

    set_template_result("Hello %NAME%", "Text: %NAME%", NULL);
    set_idempotency_not_found();

    TEST_ASSERT_TRUE(mailrelay_init());

    mock_mhd_set_queue_response_result(MHD_YES);
    mock_mhd_set_lookup_result("Bearer valid.token.here");
}

void tearDown(void) {
    mailrelay_shutdown();
    mailrelay_repo_set_executor(g_original_executor);

    free(g_test_config.mail_relay.DefaultFrom);
    free(g_test_config.server.server_name);
    memset(&g_test_config, 0, sizeof(g_test_config));

    app_config = g_saved_app_config;

    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();

    if (g_template_result) {
        json_decref(g_template_result);
        g_template_result = NULL;
    }
    if (g_idempotency_result) {
        json_decref(g_idempotency_result);
        g_idempotency_result = NULL;
    }
}

void test_send_wrong_method(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    const char* body = "{}";
    size_t body_len = strlen(body);
    void* con_cls = NULL;

    enum MHD_Result result = handle_mailrelay_send_request(mock_connection,
                                                             "/api/mailrelay/send",
                                                             "GET", body, &body_len, &con_cls);
    api_free_post_buffer(&con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_missing_auth(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    mock_mhd_set_lookup_result(NULL);

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"template_key\":\"mail.test\",\"idempotency_key\":\"key-1\",\"to\":[\"to@example.com\"]}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_missing_role(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("admin");

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"template_key\":\"mail.test\",\"idempotency_key\":\"key-1\",\"to\":[\"to@example.com\"]}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_missing_template_key(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("mail_send");

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"idempotency_key\":\"key-1\",\"to\":[\"to@example.com\"]}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_missing_idempotency_key(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("mail_send");

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"template_key\":\"mail.test\",\"to\":[\"to@example.com\"]}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_missing_recipients(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("mail_send");

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"template_key\":\"mail.test\",\"idempotency_key\":\"key-1\"}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_invalid_recipient(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("mail_send");

    enum MHD_Result result = call_send_handler(mock_connection, "POST",
                                               "{\"template_key\":\"mail.test\",\"idempotency_key\":\"key-1\",\"to\":[\"not-an-email\"]}");

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_send_success(void) {
    struct MHD_Connection* mock_connection = (struct MHD_Connection*)0x123;
    setup_valid_jwt("mail_send");

    const char* body = "{\"template_key\":\"mail.test\",\"idempotency_key\":\"key-1\",\"to\":[\"to@example.com\"],\"params\":{\"NAME\":\"World\"}}";
    enum MHD_Result result = call_send_handler(mock_connection, "POST", body);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_wrong_method);
    RUN_TEST(test_send_missing_auth);
    RUN_TEST(test_send_missing_role);
    RUN_TEST(test_send_missing_template_key);
    RUN_TEST(test_send_missing_idempotency_key);
    RUN_TEST(test_send_missing_recipients);
    RUN_TEST(test_send_invalid_recipient);
    RUN_TEST(test_send_success);

    return UNITY_END();
}
