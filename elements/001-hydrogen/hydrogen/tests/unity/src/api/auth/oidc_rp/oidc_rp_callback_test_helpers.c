/*
 * Unity Test File: OIDC RP /callback helpers
 *
 * Covers the pure / low-dependency helpers declared in
 * `oidc_rp_callback.h` and implemented in `oidc_rp_callback.c`:
 *
 *   - `callback_truncated_state()`   — 16-byte state truncator for logs.
 *   - `build_spa_error_url()`        — SPA error redirect URL builder.
 *   - `build_spa_success_url()`      — SPA success (handoff) redirect URL builder.
 *   - `callback_scrub_free()`        — volatile scrub + free of a secret string.
 *   - `redirect_with_error()`        — build error URL then 302 via MHD.
 *
 * These are the deterministic, network-free pieces of the /callback
 * chain. The full `handle_get_auth_oidc_callback` handler is exercised
 * by the blackbox test (`tests/test_42_oidc_rp.sh`); its deep error paths
 * (discovery, token exchange, linker, JWT mint) require a live MHD
 * connection plus global subsystem state and are not unit-testable here.
 *
 * `redirect_with_error` / `build_spa_error_url` depend on MHD only for the
 * final queue; MHD is globally mocked in the Unity build, so the redirect
 * branch is exercised without a real connection.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocking (malloc fault injection) for the allocation-failure
// branches. USE_MOCK_SYSTEM is a global Unity define, but guard the explicit
// define so we never trigger a -Werror redefinition if the build already set it.
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

#include <src/api/auth/oidc_rp/oidc_rp_callback.h>

#include <stdlib.h>
#include <string.h>

// Forward declarations of test functions
void test_truncated_state_null_yields_null_literal(void);
void test_truncated_state_short_value(void);
void test_truncated_state_exactly_eight(void);
void test_truncated_state_truncates_long(void);

void test_build_spa_error_url_defaults_error_code_to_server_error(void);
void test_build_spa_error_url_without_return_to(void);
void test_build_spa_error_url_with_return_to(void);
void test_build_spa_error_url_no_scheme_no_origin(void);
void test_build_spa_error_url_derives_origin_from_redirect(void);
void test_build_spa_error_url_encode_failure_is_null(void);

void test_build_spa_success_url_rejects_null_handoff(void);
void test_build_spa_success_url_rejects_empty_handoff(void);
void test_build_spa_success_url_without_return_to(void);
void test_build_spa_success_url_with_return_to(void);
void test_build_spa_success_url_encode_failure_is_null(void);

void test_callback_scrub_free_null_is_noop(void);
void test_callback_scrub_free_scrubs_and_frees(void);

void test_redirect_with_error_queues_url(void);
void test_redirect_with_error_build_failure_returns_mhd_no(void);

// handle_get_auth_oidc_callback early-branch coverage (feature gate,
// method discrimination, and the no-provider path). The deep success /
// error chain after the provider is resolved requires live MHD + global
// subsystem state and is exercised by the blackbox suite (test_42).
void test_handler_method_not_allowed(void);
void test_handler_disabled_no_config(void);
void test_handler_disabled_feature_off(void);
void test_handler_enabled_no_provider(void);

// A minimal provider fixture used where a provider pointer is needed.
static OIDCRPProviderConfig g_provider = {0};

// app_config is a global owned by the runtime; the handler reads
// oidc_rp.enabled / provider_count from it. We install a throwaway
// config for the feature-gate branches.
extern AppConfig *app_config;
static AppConfig *g_test_config = NULL;

// A non-NULL placeholder connection. MHD is globally mocked, so any
// non-NULL pointer satisfies oidc_rp_send_redirect's NULL guard without
// requiring a real connection. MHD_Connection is opaque, so we use a
// cast from a real address.
static char g_dummy_conn_storage = 0;
static struct MHD_Connection *const g_dummy_conn =
    (struct MHD_Connection *)&g_dummy_conn_storage;

void setUp(void) {
    memset(&g_provider, 0, sizeof(g_provider));
    g_provider.redirect_uri = (char *)"http://localhost:5243/api/auth/oidc/callback";
    app_config = NULL;
    g_test_config = NULL;
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
    if (g_test_config) {
        free(g_test_config);
        g_test_config = NULL;
    }
    app_config = NULL;
    memset(&g_provider, 0, sizeof(g_provider));
}

// Install a config with the OIDC RP feature enabled but no providers
// configured (provider_count == 0).
static void install_enabled_no_provider_config(void) {
    g_test_config = calloc(1, sizeof(AppConfig));
    if (!g_test_config) {
        TEST_FAIL_MESSAGE("failed to allocate test AppConfig");
        return;
    }
    g_test_config->oidc_rp.enabled = true;
    g_test_config->oidc_rp.provider_count = 0;
    app_config = g_test_config;
}

// ---------------------------------------------------------------------------
// callback_truncated_state
// ---------------------------------------------------------------------------

void test_truncated_state_null_yields_null_literal(void) {
    char out[16];
    memset(out, 'Z', sizeof(out));
    callback_truncated_state(NULL, out);
    TEST_ASSERT_EQUAL_STRING("(null)", out);
}

void test_truncated_state_short_value(void) {
    char out[16];
    callback_truncated_state("abc", out);
    TEST_ASSERT_EQUAL_STRING("abc", out);
}

void test_truncated_state_exactly_eight(void) {
    char out[16];
    callback_truncated_state("12345678", out);
    TEST_ASSERT_EQUAL_STRING("12345678", out);
}

void test_truncated_state_truncates_long(void) {
    char out[16];
    callback_truncated_state("abcdefghijklmnop", out);
    // Only the first 8 characters are copied and NUL-terminated.
    TEST_ASSERT_EQUAL_STRING("abcdefgh", out);
    TEST_ASSERT_EQUAL_size_t(8, strlen(out));
}

// ---------------------------------------------------------------------------
// build_spa_error_url
// ---------------------------------------------------------------------------

void test_build_spa_error_url_defaults_error_code_to_server_error(void) {
    // NULL error_code must default to "server_error".
    char *url = build_spa_error_url(
        "http://localhost:5243/api/auth/oidc/callback", NULL, NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "oidc_error=server_error"));
    free(url);
}

void test_build_spa_error_url_without_return_to(void) {
    char *url = build_spa_error_url(
        "http://localhost:5243/api/auth/oidc/callback", "server_error", NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "/login?oidc=1&oidc_error=server_error"));
    // No return_to parameter should be present.
    TEST_ASSERT_NULL(strstr(url, "return_to="));
    free(url);
}

void test_build_spa_error_url_with_return_to(void) {
    char *url = build_spa_error_url(
        "http://localhost:5243/api/auth/oidc/callback", "state_invalid",
        "/dashboard");
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "oidc_error=state_invalid"));
    TEST_ASSERT_NOT_NULL(strstr(url, "return_to="));
    free(url);
}

void test_build_spa_error_url_no_scheme_no_origin(void) {
    // A redirect_uri with no scheme yields an empty origin → URL starts
    // with "/login?...".
    char *url = build_spa_error_url("not-a-url", "server_error", NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_EQUAL_CHAR('/', url[0]);
    TEST_ASSERT_NOT_NULL(strstr(url, "/login?oidc=1&oidc_error=server_error"));
    free(url);
}

void test_build_spa_error_url_derives_origin_from_redirect(void) {
    char *url = build_spa_error_url(
        "https://hydrogen.example.com/api/auth/oidc/callback", "server_error",
        NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "https://hydrogen.example.com/login"));
    free(url);
}

void test_build_spa_error_url_encode_failure_is_null(void) {
    // api_url_encode only fails on allocation failure. Force it via the
    // globally-active USE_MOCK_SYSTEM malloc fault injection.
    mock_system_set_malloc_failure(1);
    char *url = build_spa_error_url(
        "http://localhost:5243/api/auth/oidc/callback", "server_error", NULL);
    mock_system_reset_all();
    TEST_ASSERT_NULL(url);
}

// ---------------------------------------------------------------------------
// build_spa_success_url
// ---------------------------------------------------------------------------

void test_build_spa_success_url_rejects_null_handoff(void) {
    char *url = build_spa_success_url(
        "http://localhost:5243/api/auth/oidc/callback", NULL, NULL);
    TEST_ASSERT_NULL(url);
}

void test_build_spa_success_url_rejects_empty_handoff(void) {
    char *url = build_spa_success_url(
        "http://localhost:5243/api/auth/oidc/callback", "", NULL);
    TEST_ASSERT_NULL(url);
}

void test_build_spa_success_url_without_return_to(void) {
    char *url = build_spa_success_url(
        "http://localhost:5243/api/auth/oidc/callback", "deadbeef", NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "/login?oidc=1&handoff=deadbeef"));
    TEST_ASSERT_NULL(strstr(url, "return_to="));
    free(url);
}

void test_build_spa_success_url_with_return_to(void) {
    char *url = build_spa_success_url(
        "http://localhost:5243/api/auth/oidc/callback", "deadbeef",
        "/dashboard");
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "handoff=deadbeef"));
    TEST_ASSERT_NOT_NULL(strstr(url, "return_to="));
    free(url);
}

void test_build_spa_success_url_encode_failure_is_null(void) {
    mock_system_set_malloc_failure(1);
    char *url = build_spa_success_url(
        "http://localhost:5243/api/auth/oidc/callback", "deadbeef", NULL);
    mock_system_reset_all();
    TEST_ASSERT_NULL(url);
}

// ---------------------------------------------------------------------------
// callback_scrub_free
// ---------------------------------------------------------------------------

void test_callback_scrub_free_null_is_noop(void) {
    // Must not crash / must return cleanly.
    callback_scrub_free(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_callback_scrub_free_scrubs_and_frees(void) {
    char *s = strdup("super-secret-token");
    TEST_ASSERT_NOT_NULL(s);
    callback_scrub_free(s);
    s = NULL;
    // Nothing observable after free; reaching here without ASAN/valgrind
    // complaints is the assertion. Use a fresh allocation to confirm the
    // heap region is reusable.
    char *again = strdup("reused");
    TEST_ASSERT_NOT_NULL(again);
    free(again);
}

// ---------------------------------------------------------------------------
// redirect_with_error
// ---------------------------------------------------------------------------

void test_redirect_with_error_queues_url(void) {
    // MHD is globally mocked; the redirect build + queue path must
    // succeed (MHD_NO only on a build failure).
    enum MHD_Result ret = redirect_with_error(
        g_dummy_conn, &g_provider, "server_error", NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_redirect_with_error_build_failure_returns_mhd_no(void) {
    // Force build_spa_error_url's allocation to fail → redirect_with_error
    // logs and returns MHD_NO.
    mock_system_set_malloc_failure(1);
    enum MHD_Result ret = redirect_with_error(
        g_dummy_conn, &g_provider, "server_error", NULL);
    mock_system_reset_all();
    TEST_ASSERT_EQUAL_INT(MHD_NO, ret);
}

// ---------------------------------------------------------------------------
// handle_get_auth_oidc_callback — early branches (feature gate, method
// discrimination, no-provider). These run before any subsystem init and
// only need app_config + a dummy connection.
// ---------------------------------------------------------------------------

void test_handler_method_not_allowed(void) {
    // Feature enabled, but a non-GET method must be rejected with 405
    // before the provider is resolved.
    install_enabled_no_provider_config();
    enum MHD_Result ret = handle_get_auth_oidc_callback(
        g_dummy_conn, "/api/auth/oidc/callback", "POST", "HTTP/1.1",
        NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handler_disabled_no_config(void) {
    // No config at all → feature gate closed → 503 oidc_disabled.
    app_config = NULL;
    enum MHD_Result ret = handle_get_auth_oidc_callback(
        g_dummy_conn, "/api/auth/oidc/callback", "GET", "HTTP/1.1",
        NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handler_disabled_feature_off(void) {
    // Config present but feature disabled → 503 oidc_disabled.
    g_test_config = calloc(1, sizeof(AppConfig));
    if (!g_test_config) {
        TEST_FAIL_MESSAGE("failed to allocate test AppConfig");
        return;
    }
    g_test_config->oidc_rp.enabled = false;
    g_test_config->oidc_rp.provider_count = 1;
    app_config = g_test_config;
    enum MHD_Result ret = handle_get_auth_oidc_callback(
        g_dummy_conn, "/api/auth/oidc/callback", "GET", "HTTP/1.1",
        NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handler_enabled_no_provider(void) {
    // Enabled but provider_count == 0 → the no-provider branch builds a
    // 503 JSON envelope {"error":"oidc_no_provider"}.
    install_enabled_no_provider_config();
    enum MHD_Result ret = handle_get_auth_oidc_callback(
        g_dummy_conn, "/api/auth/oidc/callback", "GET", "HTTP/1.1",
        NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    // callback_truncated_state
    RUN_TEST(test_truncated_state_null_yields_null_literal);
    RUN_TEST(test_truncated_state_short_value);
    RUN_TEST(test_truncated_state_exactly_eight);
    RUN_TEST(test_truncated_state_truncates_long);

    // build_spa_error_url
    RUN_TEST(test_build_spa_error_url_defaults_error_code_to_server_error);
    RUN_TEST(test_build_spa_error_url_without_return_to);
    RUN_TEST(test_build_spa_error_url_with_return_to);
    RUN_TEST(test_build_spa_error_url_no_scheme_no_origin);
    RUN_TEST(test_build_spa_error_url_derives_origin_from_redirect);
    RUN_TEST(test_build_spa_error_url_encode_failure_is_null);

    // build_spa_success_url
    RUN_TEST(test_build_spa_success_url_rejects_null_handoff);
    RUN_TEST(test_build_spa_success_url_rejects_empty_handoff);
    RUN_TEST(test_build_spa_success_url_without_return_to);
    RUN_TEST(test_build_spa_success_url_with_return_to);
    RUN_TEST(test_build_spa_success_url_encode_failure_is_null);

    // callback_scrub_free
    RUN_TEST(test_callback_scrub_free_null_is_noop);
    RUN_TEST(test_callback_scrub_free_scrubs_and_frees);

    // redirect_with_error
    RUN_TEST(test_redirect_with_error_queues_url);
    RUN_TEST(test_redirect_with_error_build_failure_returns_mhd_no);

    // handle_get_auth_oidc_callback — early branches
    RUN_TEST(test_handler_method_not_allowed);
    RUN_TEST(test_handler_disabled_no_config);
    RUN_TEST(test_handler_disabled_feature_off);
    RUN_TEST(test_handler_enabled_no_provider);

    return UNITY_END();
}
