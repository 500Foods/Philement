/*
 * Unity Test File: OIDC RP /start internal helpers (coverage expansion)
 *
 * Targets the two pure/internal helpers defined in oidc_rp_start.c that
 * are not yet exercised by oidc_rp_start_test_helpers.c (which only
 * covers the shared safe_return_to / build_authorize_url helpers):
 *
 *   - start_truncated_state()  — DEBUG-level state prefix for logs;
 *                                 NULL input -> "(null)", otherwise the
 *                                 first 8 chars (or fewer) copied.
 *   - send_internal_error()    — 500 envelope builder used by the /start
 *                                 handler for allocation / RNG / state
 *                                 failures. Exercises the api_send_json
 *                                 path (MHD mocked) end-to-end.
 *
 * The remaining lines of oidc_rp_start.c live inside
 * handle_get_auth_oidc_start() and require a live MHD connection plus
 * the lazy state-store + discovery-cache init (covered by the black-box
 * test_42_oidc_rp.sh). The json_object() allocation-failure branch of
 * send_internal_error is the irreducible floor (third-party malloc
 * injection).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_start.h>

#include <stdlib.h>
#include <string.h>

// MHD is mocked globally (USE_MOCK_LIBMICROHTTPD) — the connection is an
// opaque pointer that is never dereferenced by the mocked calls.
#include <unity/mocks/mock_libmicrohttpd.h>

static struct MHD_Connection *const FAKE_CONN =
    (struct MHD_Connection *)0x1;

// Forward declarations of test functions
void test_start_truncated_state_null_is_null_marker(void);
void test_start_truncated_state_short_state_copied(void);
void test_start_truncated_state_long_state_truncated(void);
void test_start_truncated_state_exactly_eight_copied(void);
void test_send_internal_error_happy_path_returns_yes(void);
void test_send_internal_error_null_cause_is_accepted(void);

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

// ---------------------------------------------------------------------------
// start_truncated_state
// ---------------------------------------------------------------------------

void test_start_truncated_state_null_is_null_marker(void) {
    char out[16];
    start_truncated_state(NULL, out);
    TEST_ASSERT_EQUAL_STRING("(null)", out);
}

void test_start_truncated_state_short_state_copied(void) {
    char out[16];
    start_truncated_state("abc", out);
    // "abc" is shorter than 8 -> copied verbatim, NUL terminated.
    TEST_ASSERT_EQUAL_STRING("abc", out);
}

void test_start_truncated_state_long_state_truncated(void) {
    char out[16];
    const char *long_state = "abcdefghijklmnop";
    start_truncated_state(long_state, out);
    // Only the first 8 characters are kept.
    TEST_ASSERT_EQUAL_STRING_LEN("abcdefgh", out, 8);
    TEST_ASSERT_EQUAL_CHAR('\0', out[8]);
}

void test_start_truncated_state_exactly_eight_copied(void) {
    char out[16];
    start_truncated_state("12345678", out);
    TEST_ASSERT_EQUAL_STRING("12345678", out);
}

// ---------------------------------------------------------------------------
// send_internal_error
// ---------------------------------------------------------------------------

void test_send_internal_error_happy_path_returns_yes(void) {
    // No brotli header -> api_send_json_response takes the uncompressed
    // path; mock MHD queue returns MHD_YES.
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result ret = send_internal_error(FAKE_CONN, "runtime init failed");
    TEST_ASSERT_EQUAL(MHD_YES, ret);
}

void test_send_internal_error_null_cause_is_accepted(void) {
    // A NULL cause must not crash — the code substitutes "(unknown)".
    mock_mhd_set_queue_response_result(MHD_YES);
    enum MHD_Result ret = send_internal_error(FAKE_CONN, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, ret);
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_truncated_state_null_is_null_marker);
    RUN_TEST(test_start_truncated_state_short_state_copied);
    RUN_TEST(test_start_truncated_state_long_state_truncated);
    RUN_TEST(test_start_truncated_state_exactly_eight_copied);
    RUN_TEST(test_send_internal_error_happy_path_returns_yes);
    RUN_TEST(test_send_internal_error_null_cause_is_accepted);

    return UNITY_END();
}
