/*
 * Unity Test File: OIDC RP PKCE / nonce / state helpers
 *
 * Phase 8 of `docs/OIDC-PLAN.md`. Covers
 *   - oidc_rp_make_code_verifier()
 *   - oidc_rp_make_code_challenge()
 *   - oidc_rp_make_random_hex()
 * from src/api/auth/oidc_rp/oidc_rp_pkce.c.
 *
 * Hard rules verified by these tests:
 *   - The RFC 7636 Appendix B known-answer vector
 *     (verifier "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk" maps
 *      to challenge "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM")
 *     reproduces byte-for-byte. This is the regression anchor for
 *     anyone touching the encoding pipeline.
 *   - Generated verifiers are exactly OIDC_RP_PKCE_VERIFIER_LENGTH
 *     characters and only contain RFC 7636 §4.1 unreserved chars
 *     (subset: A-Z a-z 0-9 - _).
 *   - Generated challenges are exactly OIDC_RP_PKCE_CHALLENGE_LENGTH
 *     characters and only contain base64url chars.
 *   - Random hex values are the requested width, lowercase hex only,
 *     and reject zero-length / over-cap requests.
 *   - Repeated invocations produce distinct values (no PRNG stuck
 *     state). 1,000 verifiers and 1,000 32-byte hex strings must
 *     all be unique.
 *   - NULL / empty input to the challenge function returns NULL.
 *
 * No fixtures touch the network or filesystem. Unity tests do not
 * link ASAN; leak coverage comes from test_11_leaks_like_a_sieve at
 * the integration level (Phases 14+ exercise these helpers under
 * ASAN once endpoints actually call them).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_pkce.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations of test functions
void test_make_code_verifier_returns_correct_length(void);
void test_make_code_verifier_uses_only_unreserved_chars(void);
void test_make_code_verifier_is_unique_across_invocations(void);

void test_make_code_challenge_known_answer_rfc7636(void);
void test_make_code_challenge_returns_correct_length(void);
void test_make_code_challenge_rejects_null(void);
void test_make_code_challenge_rejects_empty(void);
void test_make_code_challenge_is_deterministic(void);
void test_make_code_challenge_uses_only_base64url_chars(void);

void test_make_random_hex_returns_correct_length(void);
void test_make_random_hex_uses_only_lowercase_hex(void);
void test_make_random_hex_rejects_zero_byte_count(void);
void test_make_random_hex_rejects_over_cap(void);
void test_make_random_hex_is_unique_across_invocations(void);
void test_make_random_hex_handles_single_byte(void);

void setUp(void) {
    // Pure helpers — no global state to reset.
}

void tearDown(void) {
    // Same.
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Subset of RFC 7636 §4.1 unreserved chars that base64url emits:
// A-Z a-z 0-9 - _. (Full unreserved set also includes '.' and '~',
// which our base64url table never produces — that is fine, it is a
// strict subset.)
static int is_base64url_char(char c) {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '-'
        || c == '_';
}

static int is_lowercase_hex_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

// ---------------------------------------------------------------------------
// oidc_rp_make_code_verifier
// ---------------------------------------------------------------------------

void test_make_code_verifier_returns_correct_length(void) {
    char* v = oidc_rp_make_code_verifier();
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_size_t(OIDC_RP_PKCE_VERIFIER_LENGTH, strlen(v));
    free(v);
}

void test_make_code_verifier_uses_only_unreserved_chars(void) {
    char* v = oidc_rp_make_code_verifier();
    TEST_ASSERT_NOT_NULL(v);
    for (size_t i = 0; v[i] != '\0'; ++i) {
        if (!is_base64url_char(v[i])) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "verifier byte %zu = 0x%02x is not RFC 7636 unreserved",
                     i, (unsigned char)v[i]);
            TEST_FAIL_MESSAGE(msg);
        }
    }
    free(v);
}

void test_make_code_verifier_is_unique_across_invocations(void) {
    // Sanity check that the PRNG isn't stuck. 1,000 draws is well
    // below any practical birthday-collision threshold for 32 bytes
    // of entropy (2^128 expected collisions), so any duplicate here
    // is a real bug, not a flake.
    enum { N = 1000 };
    char** out = calloc(N, sizeof(char*));
    TEST_ASSERT_NOT_NULL(out);

    for (int i = 0; i < N; ++i) {
        out[i] = oidc_rp_make_code_verifier();
        TEST_ASSERT_NOT_NULL(out[i]);
    }

    // O(N^2) compare is fine for N=1000 in a unit test.
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            if (strcmp(out[i], out[j]) == 0) {
                TEST_FAIL_MESSAGE("two verifiers collided");
            }
        }
    }

    for (int i = 0; i < N; ++i) free(out[i]);
    free(out);
}

// ---------------------------------------------------------------------------
// oidc_rp_make_code_challenge
// ---------------------------------------------------------------------------

void test_make_code_challenge_known_answer_rfc7636(void) {
    // RFC 7636 Appendix B.
    const char* verifier  = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const char* expected  = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

    char* challenge = oidc_rp_make_code_challenge(verifier);
    TEST_ASSERT_NOT_NULL(challenge);
    TEST_ASSERT_EQUAL_STRING(expected, challenge);
    free(challenge);
}

void test_make_code_challenge_returns_correct_length(void) {
    char* v = oidc_rp_make_code_verifier();
    TEST_ASSERT_NOT_NULL(v);
    char* c = oidc_rp_make_code_challenge(v);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_size_t(OIDC_RP_PKCE_CHALLENGE_LENGTH, strlen(c));
    free(v);
    free(c);
}

void test_make_code_challenge_rejects_null(void) {
    TEST_ASSERT_NULL(oidc_rp_make_code_challenge(NULL));
}

void test_make_code_challenge_rejects_empty(void) {
    TEST_ASSERT_NULL(oidc_rp_make_code_challenge(""));
}

void test_make_code_challenge_is_deterministic(void) {
    // Same input twice must yield the same challenge — the function
    // is a pure SHA-256 + base64url, no salt.
    const char* verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    char* a = oidc_rp_make_code_challenge(verifier);
    char* b = oidc_rp_make_code_challenge(verifier);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_STRING(a, b);
    free(a);
    free(b);
}

void test_make_code_challenge_uses_only_base64url_chars(void) {
    char* v = oidc_rp_make_code_verifier();
    TEST_ASSERT_NOT_NULL(v);
    char* c = oidc_rp_make_code_challenge(v);
    TEST_ASSERT_NOT_NULL(c);
    for (size_t i = 0; c[i] != '\0'; ++i) {
        if (!is_base64url_char(c[i])) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "challenge byte %zu = 0x%02x is not base64url",
                     i, (unsigned char)c[i]);
            TEST_FAIL_MESSAGE(msg);
        }
    }
    free(v);
    free(c);
}

// ---------------------------------------------------------------------------
// oidc_rp_make_random_hex
// ---------------------------------------------------------------------------

void test_make_random_hex_returns_correct_length(void) {
    // The plan fixes 32 bytes (= 64 hex chars) for state and nonce;
    // also exercise a couple of other widths.
    static const size_t widths[] = { 1, 8, 16, 32, 64 };
    for (size_t i = 0; i < sizeof(widths) / sizeof(widths[0]); ++i) {
        char* h = oidc_rp_make_random_hex(widths[i]);
        TEST_ASSERT_NOT_NULL(h);
        TEST_ASSERT_EQUAL_size_t(widths[i] * 2, strlen(h));
        free(h);
    }
}

void test_make_random_hex_uses_only_lowercase_hex(void) {
    char* h = oidc_rp_make_random_hex(32);
    TEST_ASSERT_NOT_NULL(h);
    for (size_t i = 0; h[i] != '\0'; ++i) {
        if (!is_lowercase_hex_char(h[i])) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "hex byte %zu = 0x%02x is not lowercase hex",
                     i, (unsigned char)h[i]);
            TEST_FAIL_MESSAGE(msg);
        }
    }
    free(h);
}

void test_make_random_hex_rejects_zero_byte_count(void) {
    TEST_ASSERT_NULL(oidc_rp_make_random_hex(0));
}

void test_make_random_hex_rejects_over_cap(void) {
    // The cap is an internal sanity bound; we don't hard-code its
    // value into the test contract, but we know 4096 is well over
    // any reasonable cap.
    TEST_ASSERT_NULL(oidc_rp_make_random_hex(4096));
}

void test_make_random_hex_is_unique_across_invocations(void) {
    enum { N = 1000 };
    char** out = calloc(N, sizeof(char*));
    TEST_ASSERT_NOT_NULL(out);

    for (int i = 0; i < N; ++i) {
        out[i] = oidc_rp_make_random_hex(32);
        TEST_ASSERT_NOT_NULL(out[i]);
    }

    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            if (strcmp(out[i], out[j]) == 0) {
                TEST_FAIL_MESSAGE("two random hex values collided");
            }
        }
    }

    for (int i = 0; i < N; ++i) free(out[i]);
    free(out);
}

void test_make_random_hex_handles_single_byte(void) {
    // Edge case: one byte → two hex chars.
    char* h = oidc_rp_make_random_hex(1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_size_t(2, strlen(h));
    TEST_ASSERT_TRUE(is_lowercase_hex_char(h[0]));
    TEST_ASSERT_TRUE(is_lowercase_hex_char(h[1]));
    free(h);
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_make_code_verifier_returns_correct_length);
    RUN_TEST(test_make_code_verifier_uses_only_unreserved_chars);
    RUN_TEST(test_make_code_verifier_is_unique_across_invocations);

    RUN_TEST(test_make_code_challenge_known_answer_rfc7636);
    RUN_TEST(test_make_code_challenge_returns_correct_length);
    RUN_TEST(test_make_code_challenge_rejects_null);
    RUN_TEST(test_make_code_challenge_rejects_empty);
    RUN_TEST(test_make_code_challenge_is_deterministic);
    RUN_TEST(test_make_code_challenge_uses_only_base64url_chars);

    RUN_TEST(test_make_random_hex_returns_correct_length);
    RUN_TEST(test_make_random_hex_uses_only_lowercase_hex);
    RUN_TEST(test_make_random_hex_rejects_zero_byte_count);
    RUN_TEST(test_make_random_hex_rejects_over_cap);
    RUN_TEST(test_make_random_hex_is_unique_across_invocations);
    RUN_TEST(test_make_random_hex_handles_single_byte);

    return UNITY_END();
}
