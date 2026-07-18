/*
 * Unity Test File: OIDC RP /start handler + failure helper (coverage)
 *
 * Drives handle_get_auth_oidc_start() into the branches that the black-
 * box suite never hits, and exercises the extracted oidc_rp_start_fail()
 * helper directly so its scrub/free/envelope logic is covered in
 * isolation:
 *
 *   - no-provider gate  (enabled, zero providers)        -> oidc_no_provider
 *   - invalid return_to (unsafe value)                   -> invalid_return_to
 *   - discovery failure  (provider name empty => fast
 *     NULL from oidc_rp_discovery_get)                   -> oidc_discovery_failed
 *   - oidc_rp_start_fail() exercised with all buffers
 *     present and with several NULLs (partial-alloc path)
 *
 * The runtime is lazily initialized exactly like the production path and
 * torn down in tearDown() so the sweeper threads do not keep the process
 * alive. This mirrors oidc_rp_debug_inject_test_handler.c.
 *
 * The success path (302 + authorize URL) and the remaining allocation-
 * failure floors are covered by the black-box test_42_oidc_rp.sh and by
 * third-party malloc injection (irreducible floor).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_start.h>
#include <src/api/auth/oidc_rp/oidc_rp_service.h>

#include <stdlib.h>
#include <string.h>

// MHD is mocked globally (USE_MOCK_LIBMICROHTTPD).
#include <unity/mocks/mock_libmicrohttpd.h>

extern AppConfig *app_config;

static struct MHD_Connection *const FAKE_CONN =
    (struct MHD_Connection *)0x1234;

// Forward declarations of test functions
void test_start_no_provider_returns_no_provider(void);
void test_start_invalid_return_to_rejected(void);
void test_start_discovery_failure_returns_bad_gateway(void);
void test_start_fail_all_buffers_present(void);
void test_start_fail_partial_allocation(void);
void test_start_fail_null_cause_accepted(void);

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

static void install_enabled_config(size_t provider_count) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->oidc_rp.enabled = true;
    app_config->oidc_rp.provider_count = provider_count;
}

// Drive the GET /start handler with an already-complete (empty) request.
static enum MHD_Result drive_start(void) {
    size_t upload_data_size = 0;
    return handle_get_auth_oidc_start(
        FAKE_CONN, "/api/auth/oidc/start", "GET", "HTTP/1.1",
        NULL, &upload_data_size, &(void *){NULL});
}

void setUp(void) {
    mock_mhd_reset_all();
    app_config = NULL;
    oidc_rp_runtime_shutdown();
}

void tearDown(void) {
    oidc_rp_runtime_shutdown();
    mock_mhd_reset_all();
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ---------------------------------------------------------------------------
// Handler branches
// ---------------------------------------------------------------------------

void test_start_no_provider_returns_no_provider(void) {
    // enabled but no providers configured — gate returns before lazy init.
    install_enabled_config(0);
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result r = drive_start();
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_start_invalid_return_to_rejected(void) {
    // enabled + one provider, but an unsafe return_to in the query string.
    install_enabled_config(1);
    // A non-empty provider name so lazy init + discovery can run; the
    // unsafe return_to makes the handler short-circuit before discovery.
    app_config->oidc_rp.providers[0].name = strdup("test-provider");
    mock_mhd_add_lookup("return_to", "//evil.com");
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result r = drive_start();
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_start_discovery_failure_returns_bad_gateway(void) {
    // enabled + one provider with an EMPTY name: oidc_rp_discovery_get
    // returns NULL immediately (no network) -> oidc_discovery_failed.
    install_enabled_config(1);
    app_config->oidc_rp.providers[0].name = strdup("");
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result r = drive_start();
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

// ---------------------------------------------------------------------------
// oidc_rp_start_fail (extracted helper)
// ---------------------------------------------------------------------------

void test_start_fail_all_buffers_present(void) {
    // All sensitive + bookkeeping buffers present; they must all be freed
    // and the internal-error envelope emitted.
    mock_mhd_set_queue_response_result(MHD_YES);
    char *state = strdup("state-value");
    char *nonce = strdup("nonce-value");
    char *verifier = strdup("verifier-value");
    char *challenge = strdup("challenge-value");
    char *ip = strdup("203.0.113.7");
    char *endpoint = strdup("https://idp/auth");
    enum MHD_Result r = oidc_rp_start_fail(FAKE_CONN, "state store put failed",
                                           state, nonce, verifier, challenge,
                                           ip, endpoint);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_start_fail_partial_allocation(void) {
    // Earlier allocation failed: only some buffers exist; NULLs are
    // skipped gracefully and no dereference occurs.
    mock_mhd_set_queue_response_result(MHD_YES);
    char *state = strdup("state-value");
    char *challenge = strdup("challenge-value");
    enum MHD_Result r = oidc_rp_start_fail(FAKE_CONN,
                                           "PKCE/state generation failed",
                                           state, NULL, NULL, challenge,
                                           NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_start_fail_null_cause_accepted(void) {
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result r = oidc_rp_start_fail(FAKE_CONN, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_no_provider_returns_no_provider);
    RUN_TEST(test_start_invalid_return_to_rejected);
    RUN_TEST(test_start_discovery_failure_returns_bad_gateway);
    RUN_TEST(test_start_fail_all_buffers_present);
    RUN_TEST(test_start_fail_partial_allocation);
    RUN_TEST(test_start_fail_null_cause_accepted);

    return UNITY_END();
}
