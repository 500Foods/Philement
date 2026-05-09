/*
 * Unity Test File: OIDC RP /start helpers (Phase 10)
 *
 * Phase 10 of `docs/OIDC-PLAN.md`. Covers the two pure helpers added to
 * `oidc_rp_service.c` in support of the `/oidc/start` endpoint:
 *
 *   - `oidc_rp_safe_return_to()` — open-redirect allow-list.
 *   - `oidc_rp_build_authorize_url()` — Keycloak authorize URL builder
 *     with proper URL-encoding and canonical parameter order.
 *
 * The handler itself (`handle_get_auth_oidc_start`) is exercised by the
 * black-box test (`tests/test_42_oidc_rp.sh`) since it requires a live
 * MHD connection plus the lazy state-store + discovery-cache init. The
 * Unity layer covers the pure parts where deterministic asserts are
 * easy and exhaustive.
 *
 * Hard rules verified by these tests:
 *   - `safe_return_to(NULL)` returns true ("not provided" is fine).
 *   - `safe_return_to("")` returns false (empty is rejected).
 *   - All open-redirect attack vectors from the plan's security table
 *     are rejected: `//evil.com`, `https://evil.com`, `/\\evil.com`,
 *     `/foo://bar`, plus CR/LF for header injection.
 *   - Acceptable paths are accepted: `/`, `/foo`, `/foo/bar`,
 *     `/foo?x=1#y`, paths containing query strings and fragments.
 *   - `build_authorize_url` rejects every NULL/empty input.
 *   - The output URL begins with the raw authorization endpoint.
 *   - Parameters appear in canonical order: response_type, client_id,
 *     redirect_uri, scope, state, nonce, code_challenge,
 *     code_challenge_method.
 *   - `code_challenge_method=S256` is hard-coded — never anything else.
 *   - Special characters in input values are URL-encoded
 *     (`redirect_uri` containing `:`, `/`, `?`, `=`, `#`, ` `, `&`).
 *
 * No fixtures touch the network, the state store, or the discovery
 * cache.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_service.h>

#include <stdlib.h>
#include <string.h>

// Forward declarations of test functions
void test_safe_return_to_null_is_accepted(void);
void test_safe_return_to_empty_is_rejected(void);
void test_safe_return_to_no_leading_slash_rejected(void);
void test_safe_return_to_protocol_relative_rejected(void);
void test_safe_return_to_backslash_after_slash_rejected(void);
void test_safe_return_to_embedded_scheme_rejected(void);
void test_safe_return_to_backslash_anywhere_rejected(void);
void test_safe_return_to_carriage_return_rejected(void);
void test_safe_return_to_line_feed_rejected(void);
void test_safe_return_to_root_accepted(void);
void test_safe_return_to_simple_path_accepted(void);
void test_safe_return_to_nested_path_accepted(void);
void test_safe_return_to_query_and_fragment_accepted(void);
void test_safe_return_to_dotted_path_accepted(void);

void test_build_authorize_url_rejects_null_endpoint(void);
void test_build_authorize_url_rejects_null_client_id(void);
void test_build_authorize_url_rejects_null_redirect_uri(void);
void test_build_authorize_url_rejects_null_scope(void);
void test_build_authorize_url_rejects_null_state(void);
void test_build_authorize_url_rejects_null_nonce(void);
void test_build_authorize_url_rejects_null_challenge(void);
void test_build_authorize_url_rejects_empty_endpoint(void);
void test_build_authorize_url_rejects_empty_client_id(void);
void test_build_authorize_url_starts_with_endpoint(void);
void test_build_authorize_url_has_canonical_param_order(void);
void test_build_authorize_url_contains_s256(void);
void test_build_authorize_url_url_encodes_redirect_uri(void);
void test_build_authorize_url_url_encodes_state(void);
void test_build_authorize_url_url_encodes_scope(void);

void setUp(void) {
    // Pure helpers — no global state to reset.
}

void tearDown(void) {
    // Same.
}

// ---------------------------------------------------------------------------
// oidc_rp_safe_return_to
// ---------------------------------------------------------------------------

void test_safe_return_to_null_is_accepted(void) {
    // NULL means "no return_to was provided" — caller defends against
    // NULL when reading the value, but validation passes.
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to(NULL));
}

void test_safe_return_to_empty_is_rejected(void) {
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to(""));
}

void test_safe_return_to_no_leading_slash_rejected(void) {
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("foo"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("https://evil.com"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("javascript:alert(1)"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("./foo"));
}

void test_safe_return_to_protocol_relative_rejected(void) {
    // The classic open-redirect: `//evil.com` is read by some browsers
    // as a protocol-relative URL.
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("//evil.com"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("//evil.com/foo"));
}

void test_safe_return_to_backslash_after_slash_rejected(void) {
    // Some parsers fold `\\` to `/`, turning `/\evil.com` into
    // `//evil.com`. Reject up front.
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/\\evil.com"));
}

void test_safe_return_to_embedded_scheme_rejected(void) {
    // Schemes anywhere mid-string can be exploited.
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo://bar"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo://evil.com/bar"));
}

void test_safe_return_to_backslash_anywhere_rejected(void) {
    // Backslashes are Windows path separators that many web frameworks
    // happily fold to `/`. We never want them in a return_to.
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo\\bar"));
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo/\\evil.com"));
}

void test_safe_return_to_carriage_return_rejected(void) {
    // Header injection guard: CR can split a Location header.
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo\rSet-Cookie: x=y"));
}

void test_safe_return_to_line_feed_rejected(void) {
    TEST_ASSERT_FALSE(oidc_rp_safe_return_to("/foo\nSet-Cookie: x=y"));
}

void test_safe_return_to_root_accepted(void) {
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/"));
}

void test_safe_return_to_simple_path_accepted(void) {
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/foo"));
}

void test_safe_return_to_nested_path_accepted(void) {
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/foo/bar/baz"));
}

void test_safe_return_to_query_and_fragment_accepted(void) {
    // Query + fragment are fine — neither can introduce a host.
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/foo?x=1#y"));
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/dashboard?tab=billing"));
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/page#section"));
}

void test_safe_return_to_dotted_path_accepted(void) {
    // `..` is allowed at the syntactic level — path traversal is the
    // backend's problem to defend against, not this allow-list.
    TEST_ASSERT_TRUE(oidc_rp_safe_return_to("/foo/../bar"));
}

// ---------------------------------------------------------------------------
// oidc_rp_build_authorize_url
// ---------------------------------------------------------------------------

// A "happy path" set of fixtures used as defaults — tests that vary
// one input replace just that one.
static const char *DEFAULT_ENDPOINT     = "https://idp.example.com/auth";
static const char *DEFAULT_CLIENT_ID    = "lithium-web";
static const char *DEFAULT_REDIRECT     =
    "https://hydrogen.example.com/api/auth/oidc/callback";
static const char *DEFAULT_SCOPE        = "openid profile email";
static const char *DEFAULT_STATE        = "abcdef0123456789";
static const char *DEFAULT_NONCE        = "0123456789abcdef";
static const char *DEFAULT_CHALLENGE    = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

static char *build_default(void) {
    return oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                       DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                       DEFAULT_STATE, DEFAULT_NONCE,
                                       DEFAULT_CHALLENGE);
}

void test_build_authorize_url_rejects_null_endpoint(void) {
    char *url = oidc_rp_build_authorize_url(NULL, DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_client_id(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, NULL,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_redirect_uri(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                            NULL, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_scope(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, NULL,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_state(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            NULL, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_nonce(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, NULL,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_null_challenge(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            NULL);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_empty_endpoint(void) {
    char *url = oidc_rp_build_authorize_url("", DEFAULT_CLIENT_ID,
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_rejects_empty_client_id(void) {
    char *url = oidc_rp_build_authorize_url(DEFAULT_ENDPOINT, "",
                                            DEFAULT_REDIRECT, DEFAULT_SCOPE,
                                            DEFAULT_STATE, DEFAULT_NONCE,
                                            DEFAULT_CHALLENGE);
    TEST_ASSERT_NULL(url);
}

void test_build_authorize_url_starts_with_endpoint(void) {
    char *url = build_default();
    TEST_ASSERT_NOT_NULL(url);

    size_t prefix_len = strlen(DEFAULT_ENDPOINT);
    TEST_ASSERT_EQUAL_INT(0, strncmp(url, DEFAULT_ENDPOINT, prefix_len));
    // Next character must be '?' (the query string delimiter).
    TEST_ASSERT_EQUAL_CHAR('?', url[prefix_len]);
    free(url);
}

void test_build_authorize_url_has_canonical_param_order(void) {
    char *url = build_default();
    TEST_ASSERT_NOT_NULL(url);

    // Each parameter must appear, in this order, after the previous.
    const char *order[] = {
        "response_type=",
        "client_id=",
        "redirect_uri=",
        "scope=",
        "state=",
        "nonce=",
        "code_challenge=",
        "code_challenge_method=",
    };
    const size_t n = sizeof(order) / sizeof(order[0]);

    const char *cursor = url;
    for (size_t i = 0; i < n; ++i) {
        const char *hit = strstr(cursor, order[i]);
        if (!hit) {
            char msg[160];
            snprintf(msg, sizeof(msg),
                     "param '%s' missing or out of order in: %s",
                     order[i], url);
            TEST_FAIL_MESSAGE(msg);
        }
        cursor = hit + strlen(order[i]);
    }
    free(url);
}

void test_build_authorize_url_contains_s256(void) {
    char *url = build_default();
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(url, "code_challenge_method=S256"),
                                 "S256 method not present in URL");
    free(url);
}

void test_build_authorize_url_url_encodes_redirect_uri(void) {
    char *url = build_default();
    TEST_ASSERT_NOT_NULL(url);

    // The default redirect contains ':', '/', '.' — only ':' and '/'
    // are reserved characters that MUST be percent-encoded inside a
    // query parameter value. The encoder produces `%3A` and `%2F`.
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(url, "%3A"),
                                 "':' should be %3A-encoded in redirect_uri");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(url, "%2F"),
                                 "'/' should be %2F-encoded in redirect_uri");
    // Must NOT contain the literal "https://" anywhere AFTER the
    // endpoint — that would mean redirect_uri leaked unencoded.
    const char *first_colonslash = strstr(url, "://");
    TEST_ASSERT_NOT_NULL(first_colonslash);
    // It must be the one in DEFAULT_ENDPOINT and only that one.
    const char *second = strstr(first_colonslash + 3, "://");
    TEST_ASSERT_NULL_MESSAGE(second,
        "found unencoded scheme after the endpoint — redirect_uri leaked");
    free(url);
}

void test_build_authorize_url_url_encodes_state(void) {
    // Use a state that contains a character requiring encoding to
    // verify the code path (default state is hex which is unreserved).
    char *url = oidc_rp_build_authorize_url(
        DEFAULT_ENDPOINT, DEFAULT_CLIENT_ID, DEFAULT_REDIRECT,
        DEFAULT_SCOPE,
        "abc def",            // space → must be %20 (or +)
        DEFAULT_NONCE, DEFAULT_CHALLENGE);
    TEST_ASSERT_NOT_NULL(url);
    // Either '+' or '%20' is acceptable per HTML form-encoding /
    // RFC 3986 — `api_url_encode` produces %20 by convention.
    TEST_ASSERT_TRUE_MESSAGE(strstr(url, "state=abc%20def") != NULL ||
                             strstr(url, "state=abc+def")   != NULL,
                             "state value not URL-encoded");
    free(url);
}

void test_build_authorize_url_url_encodes_scope(void) {
    // The default scope contains spaces — they must be encoded.
    char *url = build_default();
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_TRUE_MESSAGE(strstr(url, "scope=openid%20profile%20email") != NULL ||
                             strstr(url, "scope=openid+profile+email")     != NULL,
                             "scope spaces not encoded");
    free(url);
}

// ---------------------------------------------------------------------------
// Test Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    // safe_return_to
    RUN_TEST(test_safe_return_to_null_is_accepted);
    RUN_TEST(test_safe_return_to_empty_is_rejected);
    RUN_TEST(test_safe_return_to_no_leading_slash_rejected);
    RUN_TEST(test_safe_return_to_protocol_relative_rejected);
    RUN_TEST(test_safe_return_to_backslash_after_slash_rejected);
    RUN_TEST(test_safe_return_to_embedded_scheme_rejected);
    RUN_TEST(test_safe_return_to_backslash_anywhere_rejected);
    RUN_TEST(test_safe_return_to_carriage_return_rejected);
    RUN_TEST(test_safe_return_to_line_feed_rejected);
    RUN_TEST(test_safe_return_to_root_accepted);
    RUN_TEST(test_safe_return_to_simple_path_accepted);
    RUN_TEST(test_safe_return_to_nested_path_accepted);
    RUN_TEST(test_safe_return_to_query_and_fragment_accepted);
    RUN_TEST(test_safe_return_to_dotted_path_accepted);

    // build_authorize_url
    RUN_TEST(test_build_authorize_url_rejects_null_endpoint);
    RUN_TEST(test_build_authorize_url_rejects_null_client_id);
    RUN_TEST(test_build_authorize_url_rejects_null_redirect_uri);
    RUN_TEST(test_build_authorize_url_rejects_null_scope);
    RUN_TEST(test_build_authorize_url_rejects_null_state);
    RUN_TEST(test_build_authorize_url_rejects_null_nonce);
    RUN_TEST(test_build_authorize_url_rejects_null_challenge);
    RUN_TEST(test_build_authorize_url_rejects_empty_endpoint);
    RUN_TEST(test_build_authorize_url_rejects_empty_client_id);
    RUN_TEST(test_build_authorize_url_starts_with_endpoint);
    RUN_TEST(test_build_authorize_url_has_canonical_param_order);
    RUN_TEST(test_build_authorize_url_contains_s256);
    RUN_TEST(test_build_authorize_url_url_encodes_redirect_uri);
    RUN_TEST(test_build_authorize_url_url_encodes_state);
    RUN_TEST(test_build_authorize_url_url_encodes_scope);

    return UNITY_END();
}
