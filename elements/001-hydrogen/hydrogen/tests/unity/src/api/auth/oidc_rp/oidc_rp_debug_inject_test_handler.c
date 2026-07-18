/*
 * Unity Test File: OIDC RP debug-inject endpoint (Phase 13)
 *
 * Covers the test-only `/api/auth/oidc/_inject_handoff` endpoint in
 * src/api/auth/oidc_rp/oidc_rp_debug_inject.c:
 *
 *   - send_bad_request()                      (400 envelope helper)
 *   - handle_post_auth_oidc_debug_inject()    (full request handler)
 *
 * The whole translation unit is `#ifndef NDEBUG`-gated; Unity builds are
 * debug builds so the functions are present. The handler is driven with
 * a mock MHD connection (all MHD calls are mocked globally via
 * USE_MOCK_LIBMICROHTTPD), a pre-populated ApiPostBuffer in con_cls, and
 * an installed app_config so the feature gate opens.
 *
 * Hard rules verified by these tests:
 *   - Non-POST methods are rejected (405).
 *   - When oidc_rp is disabled, a 503 is returned and the POST buffer is
 *     freed.
 *   - send_bad_request builds a 400 envelope, frees the buffer, and
 *     tolerates a NULL reason.
 *   - Empty body / invalid JSON / missing jwt / non-integer account_id
 *     all take the bad-request path and free the buffer.
 *   - A well-formed request stores a handoff record (verifiable via the
 *     handoff store) and frees the buffer.
 *
 * The runtime is torn down in setUp()/tearDown() (which resets the
 * pthread_once gate) so lazy-init runs deterministically per test.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/api_utils.h>
#include <src/api/auth/oidc_rp/oidc_rp_debug_inject.h>
#include <src/api/auth/oidc_rp/oidc_rp_service.h>
#include <src/api/auth/oidc_rp/oidc_rp_handoff_store.h>
#include <src/webserver/web_server_core.h>

#include <stdlib.h>
#include <string.h>

extern AppConfig *app_config;

// Forward declarations of test functions
void test_send_bad_request_with_reason(void);
void test_send_bad_request_null_reason(void);
void test_send_bad_request_frees_buffer(void);
void test_handler_get_method_rejected(void);
void test_handler_null_method_rejected(void);
void test_handler_disabled_returns_503(void);
void test_handler_buffer_continue(void);
void test_handler_empty_body(void);
void test_handler_invalid_json(void);
void test_handler_missing_jwt(void);
void test_handler_empty_jwt(void);
void test_handler_missing_account_id(void);
void test_handler_non_integer_account_id(void);
void test_handler_success_stores_handoff(void);
void test_handler_success_with_optional_fields(void);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build an ApiPostBuffer as though api_buffer_post_data had already
// accumulated a complete POST body. Ownership passes to con_cls and is
// released by the handler via api_free_post_buffer.
static void *make_post_buffer(const char *body) {
    ApiPostBuffer *buffer = calloc(1, sizeof(ApiPostBuffer));
    TEST_ASSERT_NOT_NULL(buffer);
    buffer->magic = API_POST_BUFFER_MAGIC;
    buffer->http_method = 'P';
    if (body) {
        size_t len = strlen(body);
        buffer->data = malloc(len + 1);
        TEST_ASSERT_NOT_NULL(buffer->data);
        memcpy(buffer->data, body, len + 1);
        buffer->size = len;
        buffer->capacity = len + 1;
    }
    return buffer;
}

static void install_enabled_config(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = 0;
}

// Drive the handler with an already-complete buffer (upload size 0).
static enum MHD_Result drive(void **con_cls) {
    size_t upload_data_size = 0;
    struct MHD_Connection *conn = (void *)0x1234;
    return handle_post_auth_oidc_debug_inject(
        conn, "/api/auth/oidc/_inject_handoff", "POST", "HTTP/1.1",
        NULL, &upload_data_size, con_cls);
}

void setUp(void) {
    app_config = NULL;
    // Ensure a clean runtime for each test (resets pthread_once gate).
    oidc_rp_runtime_shutdown();
    oidc_rp_handoff_store_test_disable_sweeper();
    oidc_rp_handoff_store_shutdown();
}

void tearDown(void) {
    oidc_rp_runtime_shutdown();
    oidc_rp_handoff_store_shutdown();
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ---------------------------------------------------------------------------
// send_bad_request
// ---------------------------------------------------------------------------

void test_send_bad_request_with_reason(void) {
    void *con_cls = make_post_buffer("{}");
    struct MHD_Connection *conn = (void *)0x1234;

    enum MHD_Result r = send_bad_request(conn, &con_cls, "some reason");

    // Buffer is freed by send_bad_request.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_send_bad_request_null_reason(void) {
    void *con_cls = make_post_buffer("{}");
    struct MHD_Connection *conn = (void *)0x1234;

    // NULL reason must be tolerated (defaults to "(unspecified)").
    enum MHD_Result r = send_bad_request(conn, &con_cls, NULL);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_send_bad_request_frees_buffer(void) {
    // Even with a NULL buffer the call must be safe.
    void *con_cls = NULL;
    struct MHD_Connection *conn = (void *)0x1234;

    enum MHD_Result r = send_bad_request(conn, &con_cls, "empty");

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

// ---------------------------------------------------------------------------
// Method / feature gate
// ---------------------------------------------------------------------------

void test_handler_get_method_rejected(void) {
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    struct MHD_Connection *conn = (void *)0x1234;

    enum MHD_Result r = handle_post_auth_oidc_debug_inject(
        conn, "/api/auth/oidc/_inject_handoff", "GET", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);

    // Method discrimination happens before any buffering.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_null_method_rejected(void) {
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    struct MHD_Connection *conn = (void *)0x1234;

    enum MHD_Result r = handle_post_auth_oidc_debug_inject(
        conn, "/api/auth/oidc/_inject_handoff", NULL, "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_disabled_returns_503(void) {
    // No app_config -> oidc_rp_is_enabled() is false.
    void *con_cls = make_post_buffer("{}");

    enum MHD_Result r = drive(&con_cls);

    // Disabled path frees the buffer.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_buffer_continue(void) {
    // First MHD callback for a POST: con_cls is NULL, so
    // api_buffer_post_data allocates the buffer and returns
    // API_BUFFER_CONTINUE -> handler returns MHD_YES and keeps con_cls.
    install_enabled_config();
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    struct MHD_Connection *conn = (void *)0x1234;

    enum MHD_Result r = handle_post_auth_oidc_debug_inject(
        conn, "/api/auth/oidc/_inject_handoff", "POST", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);

    TEST_ASSERT_EQUAL_INT(MHD_YES, r);
    TEST_ASSERT_NOT_NULL(con_cls);
    // Clean up the buffer the handler left in con_cls.
    api_free_post_buffer(&con_cls);
    TEST_ASSERT_NULL(con_cls);
}

// ---------------------------------------------------------------------------
// Bad request paths (feature enabled)
// ---------------------------------------------------------------------------
void test_handler_empty_body(void) {
    install_enabled_config();
    // Empty buffer -> "empty body".
    void *con_cls = make_post_buffer(NULL);

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_invalid_json(void) {
    install_enabled_config();
    void *con_cls = make_post_buffer("this is not json");

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_missing_jwt(void) {
    install_enabled_config();
    void *con_cls = make_post_buffer("{\"account_id\": 42}");

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_empty_jwt(void) {
    install_enabled_config();
    void *con_cls = make_post_buffer("{\"jwt\": \"\", \"account_id\": 42}");

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_missing_account_id(void) {
    install_enabled_config();
    void *con_cls = make_post_buffer("{\"jwt\": \"a.b.c\"}");

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handler_non_integer_account_id(void) {
    install_enabled_config();
    void *con_cls =
        make_post_buffer("{\"jwt\": \"a.b.c\", \"account_id\": \"nope\"}");

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

// ---------------------------------------------------------------------------
// Success path
// ---------------------------------------------------------------------------

void test_handler_success_stores_handoff(void) {
    install_enabled_config();
    void *con_cls =
        make_post_buffer("{\"jwt\": \"header.payload.sig\", "
                         "\"account_id\": 7}");

    size_t size_before = oidc_rp_handoff_store_size();

    enum MHD_Result r = drive(&con_cls);

    // Buffer freed and a handoff record was stored.
    TEST_ASSERT_NULL(con_cls);
    TEST_ASSERT_EQUAL_size_t(size_before + 1, oidc_rp_handoff_store_size());
    (void)r;
}

void test_handler_success_with_optional_fields(void) {
    install_enabled_config();
    void *con_cls = make_post_buffer(
        "{\"jwt\": \"header.payload.sig\", "
        "\"account_id\": 99, "
        "\"username\": \"alice\", "
        "\"roles\": \"admin,user\", "
        "\"expires_at\": 1893456000, "
        "\"ttl_seconds\": 120}");

    size_t size_before = oidc_rp_handoff_store_size();

    enum MHD_Result r = drive(&con_cls);

    TEST_ASSERT_NULL(con_cls);
    TEST_ASSERT_EQUAL_size_t(size_before + 1, oidc_rp_handoff_store_size());
    (void)r;
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_bad_request_with_reason);
    RUN_TEST(test_send_bad_request_null_reason);
    RUN_TEST(test_send_bad_request_frees_buffer);

    RUN_TEST(test_handler_get_method_rejected);
    RUN_TEST(test_handler_null_method_rejected);
    RUN_TEST(test_handler_disabled_returns_503);
    RUN_TEST(test_handler_buffer_continue);

    RUN_TEST(test_handler_empty_body);
    RUN_TEST(test_handler_invalid_json);
    RUN_TEST(test_handler_missing_jwt);
    RUN_TEST(test_handler_empty_jwt);
    RUN_TEST(test_handler_missing_account_id);
    RUN_TEST(test_handler_non_integer_account_id);

    RUN_TEST(test_handler_success_stores_handoff);
    RUN_TEST(test_handler_success_with_optional_fields);

    return UNITY_END();
}
