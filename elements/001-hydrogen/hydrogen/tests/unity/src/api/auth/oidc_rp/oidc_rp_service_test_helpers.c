/*
 * Unity Test File: OIDC RP service helpers (coverage expansion)
 *
 * Targets the remaining un-covered lines of `oidc_rp_service.c`
 * (beyond what oidc_rp_start_test_helpers.c already exercises for
 * oidc_rp_safe_return_to() and oidc_rp_build_authorize_url()):
 *
 *   - append_param()            — NULL-param guard + buffer-growth path
 *   - oidc_rp_send_redirect()  — NULL/empty guard, MHD-response-failure
 *                                  branch, and the happy 302 path
 *   - oidc_rp_get_active_provider() — disabled / no-provider / ok branch
 *   - oidc_rp_is_enabled()     — NULL config / disabled / enabled
 *
 * The allocation-failure log+return paths inside
 * oidc_rp_send_disabled_response(), oidc_rp_send_method_not_allowed()
 * and the malloc-failure branches of append_param / build_authorize_url
 * sit behind third-party (jansson / real malloc) failure injection and
 * are the irreducible floor for this file.
 *
 * No network, no state store, no discovery cache is touched.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_service.h>

#include <stdlib.h>
#include <string.h>

// MHD + malloc are already mocked globally (USE_MOCK_LIBMICROHTTPD /
// USE_MOCK_SYSTEM). We only need the control API from the MHD mock
// to exercise the redirect response-failure branch.
#include <unity/mocks/mock_libmicrohttpd.h>

// Mock app_config storage so we can drive the enabled/disabled gates.
static AppConfig mock_app_config_storage = {0};

// Forward declarations of test functions
void test_append_param_null_args_returns_false(void);
void test_append_param_grows_buffer(void);
void test_oidc_rp_send_redirect_null_connection_returns_no(void);
void test_oidc_rp_send_redirect_null_location_returns_no(void);
void test_oidc_rp_send_redirect_empty_location_returns_no(void);
void test_oidc_rp_send_redirect_create_response_failure_returns_no(void);
void test_oidc_rp_send_redirect_happy_path_returns_yes(void);
void test_oidc_rp_get_active_provider_null_config_returns_null(void);
void test_oidc_rp_get_active_provider_disabled_returns_null(void);
void test_oidc_rp_get_active_provider_zero_providers_returns_null(void);
void test_oidc_rp_get_active_provider_returns_first_provider(void);
void test_oidc_rp_is_enabled_null_config_returns_false(void);
void test_oidc_rp_is_enabled_disabled_returns_false(void);
void test_oidc_rp_is_enabled_enabled_returns_true(void);

void setUp(void) {
    mock_mhd_reset_all();
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    mock_mhd_reset_all();
    app_config = NULL;
}

// ---------------------------------------------------------------------------
// append_param
// ---------------------------------------------------------------------------

void test_append_param_null_args_returns_false(void) {
    char  buf[64];
    char *ptr = buf;
    char *null_buf = NULL;
    size_t len = 0;
    size_t cap = sizeof(buf);

    // The documented guard contract: NULL buf, NULL *buf, NULL key, or
    // NULL value must short-circuit before any dereference. (prefix, len
    // and cap are required to be valid per the internal contract.)
    TEST_ASSERT_FALSE(append_param(NULL,      &len, &cap, "?", "k", "v"));
    TEST_ASSERT_FALSE(append_param(&null_buf, &len, &cap, "?", "k", "v"));
    TEST_ASSERT_FALSE(append_param(&ptr,      &len, &cap, "?", NULL, "v"));
    TEST_ASSERT_FALSE(append_param(&ptr,      &len, &cap, "?", "k", NULL));
}

void test_append_param_grows_buffer(void) {
    // Start with a tiny buffer and append a value that does not fit,
    // forcing the realloc/growth loop (new_cap doubling up to 64).
    size_t cap = 8;
    size_t len = 0;
    char  *buf = malloc(cap);
    TEST_ASSERT_NOT_NULL(buf);

    const char *value =
        "https://hydrogen.example.com/api/auth/oidc/callback";

    bool ok = append_param(&buf, &len, &cap, "&", "redirect_uri", value);
    TEST_ASSERT_TRUE(ok);

    // The buffer must now contain the encoded key/value pair and the
    // capacity must have been grown past the original 8 bytes. The value
    // is percent-encoded, so len is the encoded length, not strlen(value).
    TEST_ASSERT_TRUE(cap > 8);
    TEST_ASSERT_EQUAL_STRING_LEN("&redirect_uri=", buf, strlen("&redirect_uri="));
    TEST_ASSERT_TRUE(len > strlen("&redirect_uri="));

    free(buf);
}

// ---------------------------------------------------------------------------
// oidc_rp_send_redirect
// ---------------------------------------------------------------------------

void test_oidc_rp_send_redirect_null_connection_returns_no(void) {
    TEST_ASSERT_EQUAL(MHD_NO, oidc_rp_send_redirect(NULL, "/foo"));
}

// MHD_Connection is an opaque type in the mock — use a sentinel pointer
// that is never dereferenced by the mocked MHD calls.
static struct MHD_Connection *const FAKE_CONN =
    (struct MHD_Connection *)0x1;

void test_oidc_rp_send_redirect_null_location_returns_no(void) {
    TEST_ASSERT_EQUAL(MHD_NO, oidc_rp_send_redirect(FAKE_CONN, NULL));
}

void test_oidc_rp_send_redirect_empty_location_returns_no(void) {
    TEST_ASSERT_EQUAL(MHD_NO, oidc_rp_send_redirect(FAKE_CONN, ""));
}

void test_oidc_rp_send_redirect_create_response_failure_returns_no(void) {
    mock_mhd_set_create_response_should_fail(true);
    TEST_ASSERT_EQUAL(MHD_NO, oidc_rp_send_redirect(FAKE_CONN, "/foo"));
}

void test_oidc_rp_send_redirect_happy_path_returns_yes(void) {
    mock_mhd_set_queue_response_result(MHD_YES);
    TEST_ASSERT_EQUAL(MHD_YES, oidc_rp_send_redirect(FAKE_CONN, "/foo/bar"));
}

// ---------------------------------------------------------------------------
// oidc_rp_get_active_provider
// ---------------------------------------------------------------------------

void test_oidc_rp_get_active_provider_null_config_returns_null(void) {
    app_config = NULL;
    TEST_ASSERT_NULL(oidc_rp_get_active_provider());
}

void test_oidc_rp_get_active_provider_disabled_returns_null(void) {
    app_config->oidc_rp.enabled = false;
    app_config->oidc_rp.provider_count = 1;
    TEST_ASSERT_NULL(oidc_rp_get_active_provider());
}

void test_oidc_rp_get_active_provider_zero_providers_returns_null(void) {
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = 0;
    TEST_ASSERT_NULL(oidc_rp_get_active_provider());
}

void test_oidc_rp_get_active_provider_returns_first_provider(void) {
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = 2;
    const OIDCRPProviderConfig *p = oidc_rp_get_active_provider();
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_PTR(&app_config->oidc_rp.providers[0], p);
}

// ---------------------------------------------------------------------------
// oidc_rp_is_enabled
// ---------------------------------------------------------------------------

void test_oidc_rp_is_enabled_null_config_returns_false(void) {
    app_config = NULL;
    TEST_ASSERT_FALSE(oidc_rp_is_enabled());
}

void test_oidc_rp_is_enabled_disabled_returns_false(void) {
    app_config->oidc_rp.enabled = false;
    TEST_ASSERT_FALSE(oidc_rp_is_enabled());
}

void test_oidc_rp_is_enabled_enabled_returns_true(void) {
    app_config->oidc_rp.enabled = true;
    TEST_ASSERT_TRUE(oidc_rp_is_enabled());
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_append_param_null_args_returns_false);
    RUN_TEST(test_append_param_grows_buffer);

    RUN_TEST(test_oidc_rp_send_redirect_null_connection_returns_no);
    RUN_TEST(test_oidc_rp_send_redirect_null_location_returns_no);
    RUN_TEST(test_oidc_rp_send_redirect_empty_location_returns_no);
    RUN_TEST(test_oidc_rp_send_redirect_create_response_failure_returns_no);
    RUN_TEST(test_oidc_rp_send_redirect_happy_path_returns_yes);

    RUN_TEST(test_oidc_rp_get_active_provider_null_config_returns_null);
    RUN_TEST(test_oidc_rp_get_active_provider_disabled_returns_null);
    RUN_TEST(test_oidc_rp_get_active_provider_zero_providers_returns_null);
    RUN_TEST(test_oidc_rp_get_active_provider_returns_first_provider);

    RUN_TEST(test_oidc_rp_is_enabled_null_config_returns_false);
    RUN_TEST(test_oidc_rp_is_enabled_disabled_returns_false);
    RUN_TEST(test_oidc_rp_is_enabled_enabled_returns_true);

    return UNITY_END();
}
