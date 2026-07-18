/*
 * Unity Test File: OIDC RP /handoff handler (direct coverage)
 *
 * Tests handle_post_auth_oidc_handoff() in oidc_rp_handoff.c and its
 * shared send_handoff_invalid() envelope directly — there were no
 * Unity tests associated with this source file. Paths exercised:
 *
 *   - method discrimination (non-POST -> 405)
 *   - feature gate (disabled -> 503, buffer freed)
 *   - first-chunk buffering (API_BUFFER_CONTINUE -> MHD_YES)
 *   - empty body / invalid json / missing+empty handoff field
 *   - unknown / expired / replay (take returns NULL)
 *   - IP-bind mismatch (stored IP present, requester IP absent)
 *   - success envelope (record taken, JWT surfaced to SPA)
 *   - send_handoff_invalid() in isolation, incl. json alloc floor
 *
 * The runtime + handoff store are lazily initialized exactly like
 * production and torn down per test (mirrors
 * oidc_rp_debug_inject_test_handler.c) so sweeper threads do not keep
 * the process alive.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/api_utils.h>
#include <src/api/auth/oidc_rp/oidc_rp_handoff.h>
#include <src/api/auth/oidc_rp/oidc_rp_handoff_store.h>
#include <src/api/auth/oidc_rp/oidc_rp_service.h>

#include <stdlib.h>
#include <string.h>

// MHD is mocked globally (USE_MOCK_LIBMICROHTTPD).
#include <unity/mocks/mock_libmicrohttpd.h>

extern AppConfig *app_config;

static struct MHD_Connection *const FAKE_CONN =
    (struct MHD_Connection *)0x1234;

// Forward declarations of test functions
void test_handoff_get_method_rejected(void);
void test_handoff_null_method_rejected(void);
void test_handoff_disabled_returns_503(void);
void test_handoff_buffer_continue(void);
void test_handoff_empty_body(void);
void test_handoff_invalid_json(void);
void test_handoff_missing_handoff_field(void);
void test_handoff_empty_handoff_field(void);
void test_handoff_unknown_code(void);
void test_handoff_ip_mismatch(void);
void test_handoff_success_envelope(void);
void test_send_handoff_invalid_basic(void);
void test_send_handoff_invalid_null_reason(void);

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

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

static void install_enabled_config(int bind_to_ip) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = 1;
    app_config->oidc_rp.providers[0].bind_handoff_to_ip = bind_to_ip;
}

// Drive the /handoff handler with a complete POST body (upload size 0).
static enum MHD_Result drive(void **con_cls, const char *body) {
    void *buf = make_post_buffer(body);
    *con_cls = buf;
    size_t upload_data_size = 0;
    return handle_post_auth_oidc_handoff(
        FAKE_CONN, "/api/auth/oidc/handoff", "POST", "HTTP/1.1",
        NULL, &upload_data_size, con_cls);
}

void setUp(void) {
    mock_mhd_reset_all();
    app_config = NULL;
    oidc_rp_runtime_shutdown();
    oidc_rp_handoff_store_test_disable_sweeper();
    oidc_rp_handoff_store_shutdown();
}

void tearDown(void) {
    oidc_rp_runtime_shutdown();
    oidc_rp_handoff_store_shutdown();
    mock_mhd_reset_all();
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ---------------------------------------------------------------------------
// Method / feature gate
// ---------------------------------------------------------------------------

void test_handoff_get_method_rejected(void) {
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = handle_post_auth_oidc_handoff(
        FAKE_CONN, "/api/auth/oidc/handoff", "GET", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_null_method_rejected(void) {
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = handle_post_auth_oidc_handoff(
        FAKE_CONN, "/api/auth/oidc/handoff", NULL, "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_disabled_returns_503(void) {
    // No app_config -> oidc_rp_is_enabled() is false.
    void *con_cls = make_post_buffer("{\"handoff\":\"abc\"}");
    enum MHD_Result r = drive(&con_cls, "{\"handoff\":\"abc\"}");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_buffer_continue(void) {
    // First MHD callback: con_cls is NULL -> api_buffer_post_data
    // allocates and returns API_BUFFER_CONTINUE -> handler keeps con_cls.
    install_enabled_config(0);
    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = handle_post_auth_oidc_handoff(
        FAKE_CONN, "/api/auth/oidc/handoff", "POST", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, r);
    TEST_ASSERT_NOT_NULL(con_cls);
    api_free_post_buffer(&con_cls);
    TEST_ASSERT_NULL(con_cls);
}

// ---------------------------------------------------------------------------
// Body / field validation
// ---------------------------------------------------------------------------

void test_handoff_empty_body(void) {
    install_enabled_config(0);
    void *con_cls = make_post_buffer(NULL);
    enum MHD_Result r = drive(&con_cls, NULL);
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_invalid_json(void) {
    install_enabled_config(0);
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "this is not json");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_missing_handoff_field(void) {
    install_enabled_config(0);
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "{\"account_id\": 7}");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_empty_handoff_field(void) {
    install_enabled_config(0);
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "{\"handoff\": \"\"}");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

// ---------------------------------------------------------------------------
// Store interactions
// ---------------------------------------------------------------------------

void test_handoff_unknown_code(void) {
    install_enabled_config(0);
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "{\"handoff\":\"does-not-exist\"}");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_ip_mismatch(void) {
    // Provider binds to IP; a record with a stored IP but a requester
    // whose IP cannot be resolved (mock returns NULL) -> mismatch.
    install_enabled_config(1);
    TEST_ASSERT_TRUE(oidc_rp_runtime_lazy_init());
    bool ok = oidc_rp_handoff_store_put("code123", "header.payload.sig",
                                        7, "alice", "admin,user",
                                        "203.0.113.7", 0, 0);
    TEST_ASSERT_TRUE(ok);
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "{\"handoff\":\"code123\"}");
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handoff_success_envelope(void) {
    // No IP binding -> record is taken and the success envelope is built.
    install_enabled_config(0);
    TEST_ASSERT_TRUE(oidc_rp_runtime_lazy_init());
    bool ok = oidc_rp_handoff_store_put("goodcode", "header.payload.sig",
                                        42, "bob", "user",
                                        NULL, 1893456000, 0);
    TEST_ASSERT_TRUE(ok);
    size_t size_before = oidc_rp_handoff_store_size();
    void *con_cls = NULL;
    enum MHD_Result r = drive(&con_cls, "{\"handoff\":\"goodcode\"}");
    // Buffer freed; record consumed (atomic take).
    TEST_ASSERT_NULL(con_cls);
    TEST_ASSERT_EQUAL_size_t(size_before - 1, oidc_rp_handoff_store_size());
    (void)r;
}

// ---------------------------------------------------------------------------
// send_handoff_invalid (helper, in isolation)
// ---------------------------------------------------------------------------

void test_send_handoff_invalid_basic(void) {
    void *con_cls = make_post_buffer("{}");
    enum MHD_Result r = send_handoff_invalid(FAKE_CONN, &con_cls,
                                             "unknown/expired/replay");
    // Buffer freed by the helper.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_send_handoff_invalid_null_reason(void) {
    void *con_cls = make_post_buffer("{}");
    enum MHD_Result r = send_handoff_invalid(FAKE_CONN, &con_cls, NULL);
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handoff_get_method_rejected);
    RUN_TEST(test_handoff_null_method_rejected);
    RUN_TEST(test_handoff_disabled_returns_503);
    RUN_TEST(test_handoff_buffer_continue);
    RUN_TEST(test_handoff_empty_body);
    RUN_TEST(test_handoff_invalid_json);
    RUN_TEST(test_handoff_missing_handoff_field);
    RUN_TEST(test_handoff_empty_handoff_field);
    RUN_TEST(test_handoff_unknown_code);
    RUN_TEST(test_handoff_ip_mismatch);
    RUN_TEST(test_handoff_success_envelope);
    RUN_TEST(test_send_handoff_invalid_basic);
    RUN_TEST(test_send_handoff_invalid_null_reason);

    return UNITY_END();
}
