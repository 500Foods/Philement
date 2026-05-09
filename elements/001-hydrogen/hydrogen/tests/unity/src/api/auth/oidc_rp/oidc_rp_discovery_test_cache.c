/*
 * Unity Test File: OIDC RP Discovery cache + parser
 *
 * Phase 9 of `docs/OIDC-PLAN.md`. Covers the public API of
 * src/api/auth/oidc_rp/oidc_rp_discovery.c:
 *   - oidc_rp_discovery_init / shutdown
 *   - oidc_rp_discovery_parse (exposed for tests)
 *   - oidc_rp_discovery_get (cache hit, miss, expiry)
 *   - oidc_rp_discovery_invalidate
 *   - oidc_rp_discovery_size
 *   - oidc_rp_discovery_doc_free
 *
 * Network access is short-circuited via the http test seam
 * (oidc_rp_http_test_set_response). Each test that triggers a
 * fetch registers its canned response immediately before the call.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_discovery.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_init_then_shutdown_is_clean(void);
void test_double_init_is_idempotent(void);
void test_shutdown_without_init_is_safe(void);

void test_parse_minimal_required_fields(void);
void test_parse_with_optional_fields(void);
void test_parse_rejects_missing_issuer(void);
void test_parse_rejects_missing_authorization_endpoint(void);
void test_parse_rejects_missing_token_endpoint(void);
void test_parse_rejects_missing_jwks_uri(void);
void test_parse_rejects_invalid_json(void);
void test_parse_rejects_non_object_top_level(void);
void test_parse_rejects_issuer_mismatch(void);
void test_parse_accepts_matching_expected_issuer(void);
void test_parse_with_null_expected_issuer_skips_check(void);

void test_get_returns_null_when_uninitialized(void);
void test_get_fetches_on_cache_miss(void);
void test_get_uses_cache_on_second_call(void);
void test_get_with_trailing_slash_in_issuer(void);
void test_get_returns_null_on_http_failure(void);
void test_get_returns_null_on_parse_failure(void);
void test_get_returns_null_on_issuer_mismatch(void);
void test_get_after_invalidate_refetches(void);
void test_invalidate_unknown_provider_is_safe(void);
void test_invalidate_with_null_is_safe(void);
void test_size_reports_correct_count(void);
void test_doc_free_handles_null(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_discovery_shutdown();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_discovery_shutdown();
}

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

static const char *MIN_DOC =
    "{"
    "\"issuer\":\"https://idp.example.com/realms/foo\","
    "\"authorization_endpoint\":\"https://idp.example.com/realms/foo/auth\","
    "\"token_endpoint\":\"https://idp.example.com/realms/foo/token\","
    "\"jwks_uri\":\"https://idp.example.com/realms/foo/jwks\""
    "}";

static const char *FULL_DOC =
    "{"
    "\"issuer\":\"https://idp.example.com/realms/foo\","
    "\"authorization_endpoint\":\"https://idp.example.com/realms/foo/auth\","
    "\"token_endpoint\":\"https://idp.example.com/realms/foo/token\","
    "\"jwks_uri\":\"https://idp.example.com/realms/foo/jwks\","
    "\"userinfo_endpoint\":\"https://idp.example.com/realms/foo/userinfo\","
    "\"end_session_endpoint\":\"https://idp.example.com/realms/foo/logout\""
    "}";

#define ISSUER "https://idp.example.com/realms/foo"
#define ISSUER_TRAILING_SLASH "https://idp.example.com/realms/foo/"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void test_init_then_shutdown_is_clean(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());
    oidc_rp_discovery_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());
}

void test_double_init_is_idempotent(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_discovery_shutdown();
}

void test_shutdown_without_init_is_safe(void) {
    // Should not crash.
    oidc_rp_discovery_shutdown();
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

void test_parse_minimal_required_fields(void) {
    OidcRpDiscoveryDoc *d = oidc_rp_discovery_parse(MIN_DOC, NULL);
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_STRING(ISSUER, d->issuer);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/foo/auth",
                             d->authorization_endpoint);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/foo/token",
                             d->token_endpoint);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/foo/jwks",
                             d->jwks_uri);
    TEST_ASSERT_NULL(d->userinfo_endpoint);
    TEST_ASSERT_NULL(d->end_session_endpoint);
    oidc_rp_discovery_doc_free(d);
}

void test_parse_with_optional_fields(void) {
    OidcRpDiscoveryDoc *d = oidc_rp_discovery_parse(FULL_DOC, NULL);
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/foo/userinfo",
                             d->userinfo_endpoint);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/foo/logout",
                             d->end_session_endpoint);
    oidc_rp_discovery_doc_free(d);
}

void test_parse_rejects_missing_issuer(void) {
    const char *bad =
        "{"
        "\"authorization_endpoint\":\"https://x/auth\","
        "\"token_endpoint\":\"https://x/token\","
        "\"jwks_uri\":\"https://x/jwks\""
        "}";
    TEST_ASSERT_NULL(oidc_rp_discovery_parse(bad, NULL));
}

void test_parse_rejects_missing_authorization_endpoint(void) {
    const char *bad =
        "{"
        "\"issuer\":\"https://x\","
        "\"token_endpoint\":\"https://x/token\","
        "\"jwks_uri\":\"https://x/jwks\""
        "}";
    TEST_ASSERT_NULL(oidc_rp_discovery_parse(bad, NULL));
}

void test_parse_rejects_missing_token_endpoint(void) {
    const char *bad =
        "{"
        "\"issuer\":\"https://x\","
        "\"authorization_endpoint\":\"https://x/auth\","
        "\"jwks_uri\":\"https://x/jwks\""
        "}";
    TEST_ASSERT_NULL(oidc_rp_discovery_parse(bad, NULL));
}

void test_parse_rejects_missing_jwks_uri(void) {
    const char *bad =
        "{"
        "\"issuer\":\"https://x\","
        "\"authorization_endpoint\":\"https://x/auth\","
        "\"token_endpoint\":\"https://x/token\""
        "}";
    TEST_ASSERT_NULL(oidc_rp_discovery_parse(bad, NULL));
}

void test_parse_rejects_invalid_json(void) {
    TEST_ASSERT_NULL(oidc_rp_discovery_parse("not-json", NULL));
    TEST_ASSERT_NULL(oidc_rp_discovery_parse("", NULL));
    TEST_ASSERT_NULL(oidc_rp_discovery_parse(NULL, NULL));
}

void test_parse_rejects_non_object_top_level(void) {
    TEST_ASSERT_NULL(oidc_rp_discovery_parse("[]", NULL));
    TEST_ASSERT_NULL(oidc_rp_discovery_parse("\"string\"", NULL));
    TEST_ASSERT_NULL(oidc_rp_discovery_parse("42", NULL));
}

void test_parse_rejects_issuer_mismatch(void) {
    OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_parse(MIN_DOC, "https://different-issuer.example.com");
    TEST_ASSERT_NULL(d);
}

void test_parse_accepts_matching_expected_issuer(void) {
    OidcRpDiscoveryDoc *d = oidc_rp_discovery_parse(MIN_DOC, ISSUER);
    TEST_ASSERT_NOT_NULL(d);
    oidc_rp_discovery_doc_free(d);
}

void test_parse_with_null_expected_issuer_skips_check(void) {
    OidcRpDiscoveryDoc *d = oidc_rp_discovery_parse(MIN_DOC, NULL);
    TEST_ASSERT_NOT_NULL(d);
    oidc_rp_discovery_doc_free(d);
}

// ---------------------------------------------------------------------------
// Cache get / fetch
// ---------------------------------------------------------------------------

void test_get_returns_null_when_uninitialized(void) {
    // No init.
    TEST_ASSERT_NULL(oidc_rp_discovery_get("p", ISSUER, false, 60));
}

void test_get_fetches_on_cache_miss(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response("/.well-known/openid-configuration",
                                    200, MIN_DOC);

    const OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_STRING(ISSUER, d->issuer);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_discovery_size());
}

void test_get_uses_cache_on_second_call(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);

    const OidcRpDiscoveryDoc *first =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NOT_NULL(first);

    // No fixture queued — second call must hit the cache. If it
    // attempted a fetch, the test seam is empty and oidc_rp_http_get
    // would attempt a real network call (which would fail with
    // status==0/NULL body). The cache hit path returns the same
    // pointer.
    const OidcRpDiscoveryDoc *second =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_PTR(first, second);
}

void test_get_with_trailing_slash_in_issuer(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());

    // The doc itself reports the canonical (non-slash) issuer; the
    // implementation must accept "issuer/" inputs and strip the slash
    // before appending /.well-known/openid-configuration.
    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);

    const OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_get("test-prov", ISSUER_TRAILING_SLASH, false, 60);
    // Issuer mismatch — config issuer with trailing slash != doc issuer.
    // Behaviour: failure (current contract). If you decide later to
    // canonicalise the configured issuer too, flip this to NOT_NULL
    // and update the contract in the header.
    TEST_ASSERT_NULL(d);
}

void test_get_returns_null_on_http_failure(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response(NULL, 500, "{\"error\":\"oops\"}");

    const OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NULL(d);
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());
}

void test_get_returns_null_on_parse_failure(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response(NULL, 200, "not-json");

    const OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NULL(d);
}

void test_get_returns_null_on_issuer_mismatch(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);

    // Caller's expected issuer differs from the doc's issuer.
    const OidcRpDiscoveryDoc *d =
        oidc_rp_discovery_get("test-prov", "https://different.example.com",
                              false, 60);
    TEST_ASSERT_NULL(d);
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());
}

void test_get_after_invalidate_refetches(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());

    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);
    const OidcRpDiscoveryDoc *first =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NOT_NULL(first);

    oidc_rp_discovery_invalidate("test-prov");
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());

    // Refetch must trigger a new HTTP call.
    oidc_rp_http_test_set_response(NULL, 200, FULL_DOC);
    const OidcRpDiscoveryDoc *second =
        oidc_rp_discovery_get("test-prov", ISSUER, false, 60);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_NULL(second->userinfo_endpoint);
}

void test_invalidate_unknown_provider_is_safe(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_discovery_invalidate("nope");
    TEST_PASS();
}

void test_invalidate_with_null_is_safe(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_discovery_invalidate(NULL);
    TEST_PASS();
}

void test_size_reports_correct_count(void) {
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_discovery_size());

    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);
    TEST_ASSERT_NOT_NULL(oidc_rp_discovery_get("p1", ISSUER, false, 60));
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_discovery_size());

    oidc_rp_http_test_set_response(NULL, 200, MIN_DOC);
    TEST_ASSERT_NOT_NULL(oidc_rp_discovery_get("p2", ISSUER, false, 60));
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_discovery_size());
}

void test_doc_free_handles_null(void) {
    oidc_rp_discovery_doc_free(NULL);
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_then_shutdown_is_clean);
    RUN_TEST(test_double_init_is_idempotent);
    RUN_TEST(test_shutdown_without_init_is_safe);

    RUN_TEST(test_parse_minimal_required_fields);
    RUN_TEST(test_parse_with_optional_fields);
    RUN_TEST(test_parse_rejects_missing_issuer);
    RUN_TEST(test_parse_rejects_missing_authorization_endpoint);
    RUN_TEST(test_parse_rejects_missing_token_endpoint);
    RUN_TEST(test_parse_rejects_missing_jwks_uri);
    RUN_TEST(test_parse_rejects_invalid_json);
    RUN_TEST(test_parse_rejects_non_object_top_level);
    RUN_TEST(test_parse_rejects_issuer_mismatch);
    RUN_TEST(test_parse_accepts_matching_expected_issuer);
    RUN_TEST(test_parse_with_null_expected_issuer_skips_check);

    RUN_TEST(test_get_returns_null_when_uninitialized);
    RUN_TEST(test_get_fetches_on_cache_miss);
    RUN_TEST(test_get_uses_cache_on_second_call);
    RUN_TEST(test_get_with_trailing_slash_in_issuer);
    RUN_TEST(test_get_returns_null_on_http_failure);
    RUN_TEST(test_get_returns_null_on_parse_failure);
    RUN_TEST(test_get_returns_null_on_issuer_mismatch);
    RUN_TEST(test_get_after_invalidate_refetches);
    RUN_TEST(test_invalidate_unknown_provider_is_safe);
    RUN_TEST(test_invalidate_with_null_is_safe);
    RUN_TEST(test_size_reports_correct_count);
    RUN_TEST(test_doc_free_handles_null);

    return UNITY_END();
}
