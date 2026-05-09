/*
 * Unity Test File: OIDC RP JWKS cache + parser
 *
 * Phase 9 of `docs/OIDC-PLAN.md`. Covers oidc_rp_jwks_init/shutdown,
 * oidc_rp_jwks_parse (exposed for tests), oidc_rp_jwks_find,
 * oidc_rp_jwks_invalidate, oidc_rp_jwks_size, oidc_rp_jwks_key_count,
 * and oidc_rp_jwks_keys_free from
 * src/api/auth/oidc_rp/oidc_rp_jwks.c.
 *
 * Network access is short-circuited via the http test seam.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_jwks.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

#include <stdlib.h>
#include <string.h>

void test_init_then_shutdown_is_clean(void);
void test_double_init_is_idempotent(void);
void test_shutdown_without_init_is_safe(void);

void test_parse_single_key(void);
void test_parse_multiple_keys(void);
void test_parse_rejects_missing_keys_array(void);
void test_parse_rejects_empty_keys_array(void);
void test_parse_rejects_invalid_json(void);
void test_parse_rejects_non_object_top_level(void);
void test_parse_skips_non_object_entries(void);
void test_parse_skips_entries_missing_kid(void);
void test_parse_keeps_entries_missing_kty_with_warning(void);
void test_parse_handles_null_args(void);

void test_find_returns_null_when_uninitialized(void);
void test_find_fetches_and_returns_match(void);
void test_find_returns_null_for_unknown_kid(void);
void test_find_uses_cache_on_repeat_call(void);
void test_find_returns_null_on_http_failure(void);
void test_find_returns_null_on_parse_failure(void);
void test_find_after_invalidate_refetches(void);
void test_find_with_invalid_args_returns_null(void);
void test_invalidate_unknown_provider_is_safe(void);
void test_invalidate_with_null_is_safe(void);
void test_size_and_key_count_track_inserts(void);
void test_keys_free_handles_null(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_jwks_shutdown();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_jwks_shutdown();
}

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

// Minimal but well-formed JWKS with one RS256 RSA key. The `n` and
// `e` values are placeholder strings — Phase 9 doesn't run the
// crypto, just stores the JSON for Phase 12 to hand to OpenSSL.
static const char *JWKS_ONE_KEY =
    "{"
    "\"keys\":["
    "{"
    "\"kid\":\"key-1\","
    "\"kty\":\"RSA\","
    "\"alg\":\"RS256\","
    "\"use\":\"sig\","
    "\"n\":\"placeholder-n\","
    "\"e\":\"AQAB\""
    "}"
    "]"
    "}";

static const char *JWKS_TWO_KEYS =
    "{"
    "\"keys\":["
    "{\"kid\":\"key-1\",\"kty\":\"RSA\",\"alg\":\"RS256\","
    "\"use\":\"sig\",\"n\":\"n1\",\"e\":\"AQAB\"},"
    "{\"kid\":\"key-2\",\"kty\":\"RSA\",\"alg\":\"RS256\","
    "\"use\":\"sig\",\"n\":\"n2\",\"e\":\"AQAB\"}"
    "]"
    "}";

#define JWKS_URI "https://idp.example.com/realms/foo/jwks"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void test_init_then_shutdown_is_clean(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_size());
    oidc_rp_jwks_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_size());
}

void test_double_init_is_idempotent(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_jwks_shutdown();
}

void test_shutdown_without_init_is_safe(void) {
    oidc_rp_jwks_shutdown();
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

void test_parse_single_key(void) {
    size_t count = 0;
    OidcRpJwk *keys = oidc_rp_jwks_parse(JWKS_ONE_KEY, &count);
    TEST_ASSERT_NOT_NULL(keys);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING("key-1", keys[0].kid);
    TEST_ASSERT_EQUAL_STRING("RSA", keys[0].kty);
    TEST_ASSERT_EQUAL_STRING("RS256", keys[0].alg);
    TEST_ASSERT_EQUAL_STRING("sig", keys[0].use);
    TEST_ASSERT_NOT_NULL(keys[0].json_text);
    TEST_ASSERT_TRUE(strstr(keys[0].json_text, "\"kid\":\"key-1\"") != NULL);
    oidc_rp_jwks_keys_free(keys, count);
}

void test_parse_multiple_keys(void) {
    size_t count = 0;
    OidcRpJwk *keys = oidc_rp_jwks_parse(JWKS_TWO_KEYS, &count);
    TEST_ASSERT_NOT_NULL(keys);
    TEST_ASSERT_EQUAL_size_t(2, count);
    TEST_ASSERT_EQUAL_STRING("key-1", keys[0].kid);
    TEST_ASSERT_EQUAL_STRING("key-2", keys[1].kid);
    oidc_rp_jwks_keys_free(keys, count);
}

void test_parse_rejects_missing_keys_array(void) {
    size_t count = 99;
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("{\"foo\":\"bar\"}", &count));
    TEST_ASSERT_EQUAL_size_t(0, count);
}

void test_parse_rejects_empty_keys_array(void) {
    size_t count = 99;
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("{\"keys\":[]}", &count));
    TEST_ASSERT_EQUAL_size_t(0, count);
}

void test_parse_rejects_invalid_json(void) {
    size_t count = 99;
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("not-json", &count));
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("", &count));
    TEST_ASSERT_NULL(oidc_rp_jwks_parse(NULL, &count));
}

void test_parse_rejects_non_object_top_level(void) {
    size_t count = 99;
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("[]", &count));
    TEST_ASSERT_NULL(oidc_rp_jwks_parse("\"foo\"", &count));
}

void test_parse_skips_non_object_entries(void) {
    const char *mixed =
        "{\"keys\":["
        "\"not-an-object\","
        "{\"kid\":\"good\",\"kty\":\"RSA\",\"alg\":\"RS256\","
        "\"use\":\"sig\",\"n\":\"x\",\"e\":\"AQAB\"}"
        "]}";
    size_t count = 0;
    OidcRpJwk *keys = oidc_rp_jwks_parse(mixed, &count);
    TEST_ASSERT_NOT_NULL(keys);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING("good", keys[0].kid);
    oidc_rp_jwks_keys_free(keys, count);
}

void test_parse_skips_entries_missing_kid(void) {
    const char *mixed =
        "{\"keys\":["
        "{\"kty\":\"RSA\",\"alg\":\"RS256\",\"n\":\"x\",\"e\":\"AQAB\"},"
        "{\"kid\":\"good\",\"kty\":\"RSA\",\"alg\":\"RS256\","
        "\"use\":\"sig\",\"n\":\"x\",\"e\":\"AQAB\"}"
        "]}";
    size_t count = 0;
    OidcRpJwk *keys = oidc_rp_jwks_parse(mixed, &count);
    TEST_ASSERT_NOT_NULL(keys);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING("good", keys[0].kid);
    oidc_rp_jwks_keys_free(keys, count);
}

void test_parse_keeps_entries_missing_kty_with_warning(void) {
    // `kty` missing — we keep the entry but `kty` stays NULL. Phase 12
    // is responsible for rejecting unusable keys.
    const char *bad_kty =
        "{\"keys\":["
        "{\"kid\":\"weird\",\"alg\":\"RS256\","
        "\"use\":\"sig\",\"n\":\"x\",\"e\":\"AQAB\"}"
        "]}";
    size_t count = 0;
    OidcRpJwk *keys = oidc_rp_jwks_parse(bad_kty, &count);
    TEST_ASSERT_NOT_NULL(keys);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING("weird", keys[0].kid);
    TEST_ASSERT_NULL(keys[0].kty);
    oidc_rp_jwks_keys_free(keys, count);
}

void test_parse_handles_null_args(void) {
    size_t count = 0;
    TEST_ASSERT_NULL(oidc_rp_jwks_parse(NULL, &count));
    TEST_ASSERT_NULL(oidc_rp_jwks_parse(JWKS_ONE_KEY, NULL));
}

// ---------------------------------------------------------------------------
// Cache get / find
// ---------------------------------------------------------------------------

void test_find_returns_null_when_uninitialized(void) {
    TEST_ASSERT_NULL(oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1"));
}

void test_find_fetches_and_returns_match(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_http_test_set_response("/jwks", 200, JWKS_ONE_KEY);

    const OidcRpJwk *k = oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1");
    TEST_ASSERT_NOT_NULL(k);
    TEST_ASSERT_EQUAL_STRING("key-1", k->kid);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_size());
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_key_count("p"));
}

void test_find_returns_null_for_unknown_kid(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_http_test_set_response(NULL, 200, JWKS_ONE_KEY);

    // Fetch happens (kid is not in cache because cache is empty), but
    // the kid is not in the fetched set. Behaviour: cache is populated,
    // result is NULL, key_count == 1.
    const OidcRpJwk *k =
        oidc_rp_jwks_find("p", JWKS_URI, false, 60, "missing-kid");
    TEST_ASSERT_NULL(k);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_size());
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_key_count("p"));
}

void test_find_uses_cache_on_repeat_call(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_http_test_set_response(NULL, 200, JWKS_TWO_KEYS);

    const OidcRpJwk *first =
        oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1");
    TEST_ASSERT_NOT_NULL(first);

    // No fixture queued — second find must hit the cache. Asks for
    // a different kid that is in the same set; must succeed without
    // a fetch.
    const OidcRpJwk *second =
        oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-2");
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("key-2", second->kid);
}

void test_find_returns_null_on_http_failure(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_http_test_set_response(NULL, 503, "{\"error\":\"oops\"}");

    const OidcRpJwk *k =
        oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1");
    TEST_ASSERT_NULL(k);
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_size());
}

void test_find_returns_null_on_parse_failure(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_http_test_set_response(NULL, 200, "not-json");

    const OidcRpJwk *k =
        oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1");
    TEST_ASSERT_NULL(k);
}

void test_find_after_invalidate_refetches(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());

    oidc_rp_http_test_set_response(NULL, 200, JWKS_ONE_KEY);
    TEST_ASSERT_NOT_NULL(oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-1"));
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_key_count("p"));

    oidc_rp_jwks_invalidate("p");
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_size());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_key_count("p"));

    // Refetch must trigger a new HTTP call. Switch to a 2-key set.
    oidc_rp_http_test_set_response(NULL, 200, JWKS_TWO_KEYS);
    TEST_ASSERT_NOT_NULL(oidc_rp_jwks_find("p", JWKS_URI, false, 60, "key-2"));
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_jwks_key_count("p"));
}

void test_find_with_invalid_args_returns_null(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());

    TEST_ASSERT_NULL(oidc_rp_jwks_find(NULL, JWKS_URI, false, 60, "k"));
    TEST_ASSERT_NULL(oidc_rp_jwks_find("",   JWKS_URI, false, 60, "k"));
    TEST_ASSERT_NULL(oidc_rp_jwks_find("p",  NULL,     false, 60, "k"));
    TEST_ASSERT_NULL(oidc_rp_jwks_find("p",  "",       false, 60, "k"));
    TEST_ASSERT_NULL(oidc_rp_jwks_find("p",  JWKS_URI, false, 60, NULL));
    TEST_ASSERT_NULL(oidc_rp_jwks_find("p",  JWKS_URI, false, 60, ""));
}

void test_invalidate_unknown_provider_is_safe(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_jwks_invalidate("nope");
    TEST_PASS();
}

void test_invalidate_with_null_is_safe(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    oidc_rp_jwks_invalidate(NULL);
    TEST_PASS();
}

void test_size_and_key_count_track_inserts(void) {
    TEST_ASSERT_TRUE(oidc_rp_jwks_init());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_size());
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_jwks_key_count("any"));

    oidc_rp_http_test_set_response(NULL, 200, JWKS_ONE_KEY);
    TEST_ASSERT_NOT_NULL(oidc_rp_jwks_find("p1", JWKS_URI, false, 60, "key-1"));
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_size());

    oidc_rp_http_test_set_response(NULL, 200, JWKS_TWO_KEYS);
    TEST_ASSERT_NOT_NULL(oidc_rp_jwks_find("p2", JWKS_URI, false, 60, "key-1"));
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_jwks_size());
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_jwks_key_count("p1"));
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_jwks_key_count("p2"));
}

void test_keys_free_handles_null(void) {
    oidc_rp_jwks_keys_free(NULL, 0);
    oidc_rp_jwks_keys_free(NULL, 99);
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

    RUN_TEST(test_parse_single_key);
    RUN_TEST(test_parse_multiple_keys);
    RUN_TEST(test_parse_rejects_missing_keys_array);
    RUN_TEST(test_parse_rejects_empty_keys_array);
    RUN_TEST(test_parse_rejects_invalid_json);
    RUN_TEST(test_parse_rejects_non_object_top_level);
    RUN_TEST(test_parse_skips_non_object_entries);
    RUN_TEST(test_parse_skips_entries_missing_kid);
    RUN_TEST(test_parse_keeps_entries_missing_kty_with_warning);
    RUN_TEST(test_parse_handles_null_args);

    RUN_TEST(test_find_returns_null_when_uninitialized);
    RUN_TEST(test_find_fetches_and_returns_match);
    RUN_TEST(test_find_returns_null_for_unknown_kid);
    RUN_TEST(test_find_uses_cache_on_repeat_call);
    RUN_TEST(test_find_returns_null_on_http_failure);
    RUN_TEST(test_find_returns_null_on_parse_failure);
    RUN_TEST(test_find_after_invalidate_refetches);
    RUN_TEST(test_find_with_invalid_args_returns_null);
    RUN_TEST(test_invalidate_unknown_provider_is_safe);
    RUN_TEST(test_invalidate_with_null_is_safe);
    RUN_TEST(test_size_and_key_count_track_inserts);
    RUN_TEST(test_keys_free_handles_null);

    return UNITY_END();
}
