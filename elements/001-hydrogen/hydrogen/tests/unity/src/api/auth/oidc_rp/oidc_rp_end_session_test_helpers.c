/*
 * Unity Test File: OIDC RP end-session helpers
 *
 * Covers the URL-building helpers in src/api/auth/oidc_rp/oidc_rp_end_session.c:
 *   - oidc_rp_end_session_extract_origin()
 *   - oidc_rp_end_session_build_idp_logout_url()
 *
 * These were previously file-static and therefore uncovered by both the
 * blackbox and Unity suites. They are now exposed as public, prefixed
 * APIs so they can be exercised directly in isolation.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_end_session.h>
#include <src/config/config.h>

#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

#include <stdlib.h>
#include <string.h>

void test_extract_origin_null(void);
void test_extract_origin_empty(void);
void test_extract_origin_no_scheme(void);
void test_extract_origin_with_path(void);
void test_extract_origin_no_path(void);
void test_extract_origin_with_query_and_fragment(void);
void test_build_logout_url_null_endpoint(void);
void test_build_logout_url_null_id_token(void);
void test_build_logout_url_full(void);
void test_build_logout_url_without_client_id_null(void);
void test_build_logout_url_without_client_id_empty(void);
void test_build_logout_url_with_null_redirect(void);
void test_handle_end_session_disabled(void);
void test_handle_end_session_missing_auth(void);
void test_handle_end_session_invalid_auth_format(void);
void test_handle_end_session_empty_token(void);

extern AppConfig *app_config;

void setUp(void) {
    // Start each test with no config so the feature gate is closed
    // unless a specific test installs one.
    app_config = NULL;
}

void tearDown(void) {
    // Free any config a test may have installed.
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ---------------------------------------------------------------------------
// oidc_rp_end_session_extract_origin
// ---------------------------------------------------------------------------

void test_extract_origin_null(void) {
    char *origin = oidc_rp_end_session_extract_origin(NULL);
    TEST_ASSERT_NULL(origin);
}

void test_extract_origin_empty(void) {
    char *origin = oidc_rp_end_session_extract_origin("");
    TEST_ASSERT_NULL(origin);
}

void test_extract_origin_no_scheme(void) {
    // No "://" separator -> cannot determine origin.
    char *origin = oidc_rp_end_session_extract_origin("nocolonhere");
    TEST_ASSERT_NULL(origin);
}

void test_extract_origin_with_path(void) {
    char *origin = oidc_rp_end_session_extract_origin(
        "https://lithium.philement.com/api/auth/oidc/callback");
    TEST_ASSERT_NOT_NULL(origin);
    TEST_ASSERT_EQUAL_STRING("https://lithium.philement.com", origin);
    free(origin);
}

void test_extract_origin_no_path(void) {
    // Bare origin with no path component.
    char *origin = oidc_rp_end_session_extract_origin("https://example.com");
    TEST_ASSERT_NOT_NULL(origin);
    TEST_ASSERT_EQUAL_STRING("https://example.com", origin);
    free(origin);
}

void test_extract_origin_with_query_and_fragment(void) {
    char *origin = oidc_rp_end_session_extract_origin(
        "http://idp.example.com/realms/foo/end-session?foo=bar#frag");
    TEST_ASSERT_NOT_NULL(origin);
    TEST_ASSERT_EQUAL_STRING("http://idp.example.com", origin);
    free(origin);
}

// ---------------------------------------------------------------------------
// oidc_rp_end_session_build_idp_logout_url
// ---------------------------------------------------------------------------

void test_build_logout_url_null_endpoint(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        NULL, "tok", "https://example.com", "client");
    TEST_ASSERT_NULL(url);
}

void test_build_logout_url_null_id_token(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        "https://idp.example.com/end", NULL, "https://example.com", "client");
    TEST_ASSERT_NULL(url);
}

void test_build_logout_url_full(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        "https://idp.example.com/realms/foo/protocol/openid-connect/logout",
        "idtokendata",
        "https://app.example.com",
        "my-client");
    TEST_ASSERT_NOT_NULL(url);
    // Endpoint prefix must be intact.
    TEST_ASSERT_TRUE(strncmp(
        url,
        "https://idp.example.com/realms/foo/protocol/openid-connect/logout?",
        strlen("https://idp.example.com/realms/foo/protocol/openid-connect/logout?")) == 0);
    TEST_ASSERT_NOT_NULL(strstr(url, "id_token_hint=idtokendata"));
    TEST_ASSERT_NOT_NULL(strstr(url, "post_logout_redirect_uri="));
    TEST_ASSERT_NOT_NULL(strstr(url, "client_id=my-client"));
    free(url);
}

void test_build_logout_url_without_client_id_null(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        "https://idp.example.com/logout",
        "idtokendata",
        "https://app.example.com",
        NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "id_token_hint=idtokendata"));
    TEST_ASSERT_NOT_NULL(strstr(url, "post_logout_redirect_uri="));
    // No client_id parameter when client_id is NULL.
    TEST_ASSERT_NULL(strstr(url, "client_id="));
    free(url);
}

void test_build_logout_url_without_client_id_empty(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        "https://idp.example.com/logout",
        "idtokendata",
        "https://app.example.com",
        "");
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "id_token_hint=idtokendata"));
    TEST_ASSERT_NOT_NULL(strstr(url, "post_logout_redirect_uri="));
    // Empty client_id (after encoding) suppresses the client_id parameter.
    TEST_ASSERT_NULL(strstr(url, "client_id="));
    free(url);
}

void test_build_logout_url_with_null_redirect(void) {
    char *url = oidc_rp_end_session_build_idp_logout_url(
        "https://idp.example.com/logout",
        "idtokendata",
        NULL,
        "my-client");
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "id_token_hint=idtokendata"));
    // NULL redirect_uri is encoded as the empty string, so the parameter
    // is still present (just empty).
    TEST_ASSERT_NOT_NULL(strstr(url, "post_logout_redirect_uri="));
    TEST_ASSERT_NOT_NULL(strstr(url, "client_id=my-client"));
    free(url);
}

void test_handle_end_session_disabled(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_data_size = 0;

    // Phase 1: allocate + buffer the (empty) POST body.
    enum MHD_Result r1 = handle_post_auth_oidc_end_session(
        conn, "/api/auth/oidc/end-session", "POST", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, r1);
    TEST_ASSERT_NOT_NULL(con_cls);

    // Phase 2: finalize buffering -> feature gate fires (disabled).
    enum MHD_Result r2 = handle_post_auth_oidc_end_session(
        conn, "/api/auth/oidc/end-session", "POST", "HTTP/1.1",
        NULL, &upload_data_size, &con_cls);
    // The disabled response frees the POST buffer, so con_cls is cleared.
    TEST_ASSERT_NULL(con_cls);
    (void)r2;
}

// ---------------------------------------------------------------------------
// handle_post_auth_oidc_end_session — enabled-mode auth/JWT error branches
//
// With a minimal `app_config` where OIDC RP is enabled and no providers
// are configured, the feature gate opens and lazy runtime init runs. We
// then drive the Authorization-header and JWT-validation failure paths by
// controlling the (mocked) MHD_lookup_connection_value result. Each branch
// frees the POST buffer, so we re-run the two-phase buffering per case.
// ---------------------------------------------------------------------------

// Bring up a minimal enabled config and a buffered POST request pair.
// Returns the handler's result from the final (COMPLETE) call; `con_cls`
// is left pointing at whatever the handler set.
static enum MHD_Result drive_end_session(const char *auth_header,
                                         void **con_cls,
                                         size_t *upload_data_size) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    mock_mhd_reset_all();
    mock_mhd_set_lookup_result(auth_header);

    enum MHD_Result r1 = handle_post_auth_oidc_end_session(
        conn, "/api/auth/oidc/end-session", "POST", "HTTP/1.1",
        NULL, upload_data_size, con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, r1);

    return handle_post_auth_oidc_end_session(
        conn, "/api/auth/oidc/end_session", "POST", "HTTP/1.1",
        NULL, upload_data_size, con_cls);
}

static void install_enabled_config(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = 0;
}

void test_handle_end_session_missing_auth(void) {
    install_enabled_config();

    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = drive_end_session(NULL, &con_cls, &upload_data_size);

    // Missing Authorization header -> 401, POST buffer freed.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handle_end_session_invalid_auth_format(void) {
    install_enabled_config();

    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = drive_end_session("NotBearer xyz", &con_cls, &upload_data_size);

    // Malformed Authorization header -> 401, POST buffer freed.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

void test_handle_end_session_empty_token(void) {
    install_enabled_config();

    void *con_cls = NULL;
    size_t upload_data_size = 0;
    enum MHD_Result r = drive_end_session("Bearer ", &con_cls, &upload_data_size);

    // "Bearer " with empty token -> 401, POST buffer freed.
    TEST_ASSERT_NULL(con_cls);
    (void)r;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_origin_null);
    RUN_TEST(test_extract_origin_empty);
    RUN_TEST(test_extract_origin_no_scheme);
    RUN_TEST(test_extract_origin_with_path);
    RUN_TEST(test_extract_origin_no_path);
    RUN_TEST(test_extract_origin_with_query_and_fragment);
    RUN_TEST(test_build_logout_url_null_endpoint);
    RUN_TEST(test_build_logout_url_null_id_token);
    RUN_TEST(test_build_logout_url_full);
    RUN_TEST(test_build_logout_url_without_client_id_null);
    RUN_TEST(test_build_logout_url_without_client_id_empty);
    RUN_TEST(test_build_logout_url_with_null_redirect);
    RUN_TEST(test_handle_end_session_disabled);
    RUN_TEST(test_handle_end_session_missing_auth);
    RUN_TEST(test_handle_end_session_invalid_auth_format);
    RUN_TEST(test_handle_end_session_empty_token);
    return UNITY_END();
}
