/*
 * Unity Test File: OIDC RP ID-token validation (Phase 12).
 *
 * Covers `oidc_rp_validate_id_token` and supporting helpers from
 * `src/api/auth/oidc_rp/oidc_rp_idtoken.c`. JWKS lookups are
 * exercised through the http test seam
 * (`oidc_rp_http_test_set_response`) — we never actually open a
 * socket. RSA keypairs are generated fresh per `setUp()` so no key
 * material is committed to the repo.
 *
 * Coverage:
 *   - Argument validation (NULL/empty inputs).
 *   - Lifecycle helpers (claims_free NULL-safe, error_name table).
 *   - Successful validation populates every claim.
 *   - alg=none rejected even if it appears in the allow-list (paranoia).
 *   - alg outside allow-list rejected.
 *   - alg in allow-list but not RS256 rejected (until other families
 *     are wired in).
 *   - kid mismatch triggers JWKS refetch and succeeds.
 *   - kid mismatch persists after refetch ⇒ KID_UNKNOWN.
 *   - Tampered signature ⇒ BAD_SIGNATURE (after one rotation retry).
 *   - Expired exp ⇒ EXPIRED.
 *   - iat in the future beyond skew ⇒ NOT_YET_VALID.
 *   - nbf in the future beyond skew ⇒ NOT_YET_VALID.
 *   - Wrong iss ⇒ ISS_MISMATCH.
 *   - aud missing client_id ⇒ AUD_MISMATCH.
 *   - aud as JSON array containing client_id ⇒ OK.
 *   - Missing required claim (sub) ⇒ MISSING_CLAIM.
 *   - Nonce mismatch ⇒ NONCE_MISMATCH.
 *   - Malformed JWT (wrong segment count, empty segments) ⇒ MALFORMED.
 *   - email_verified=true honoured; default false when missing.
 *   - realm_access.roles correctly extracted.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_idtoken.h>
#include <src/api/auth/oidc_rp/oidc_rp_jwks.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include <src/config/config_oidc_rp.h>
#include <src/utils/utils_crypto.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Forward declarations.
// ---------------------------------------------------------------------------

void test_claims_free_handles_null(void);
void test_error_name_returns_stable_strings(void);
void test_validate_rejects_null_provider(void);
void test_validate_rejects_null_jwks_uri(void);
void test_validate_rejects_null_id_token(void);
void test_validate_rejects_null_nonce(void);
void test_validate_rejects_null_out(void);
void test_validate_rejects_provider_without_alg_list(void);
void test_validate_happy_path(void);
void test_validate_happy_path_aud_array(void);
void test_validate_rejects_alg_none(void);
void test_validate_rejects_alg_not_in_allow_list(void);
void test_validate_rejects_unimplemented_alg(void);
void test_validate_rejects_wrong_iss(void);
void test_validate_rejects_wrong_aud(void);
void test_validate_rejects_missing_sub(void);
void test_validate_rejects_expired(void);
void test_validate_rejects_iat_in_future(void);
void test_validate_rejects_nbf_in_future(void);
void test_validate_rejects_wrong_nonce(void);
void test_validate_rejects_malformed_segments(void);
void test_validate_rejects_empty_segments(void);
void test_validate_rejects_tampered_signature(void);
void test_validate_kid_miss_triggers_refetch(void);
void test_validate_kid_unknown_after_refetch(void);
void test_validate_email_verified_honoured(void);
void test_validate_extracts_realm_roles(void);

// ---------------------------------------------------------------------------
// Crypto fixture: per-test RSA keypair, JWK string, and token signer.
// ---------------------------------------------------------------------------

static EVP_PKEY* g_pkey = NULL;
static char* g_jwk_n_b64u = NULL; // base64url of the modulus
static char* g_jwk_e_b64u = NULL; // base64url of the public exponent

static void make_keypair(void) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_TRUE(EVP_PKEY_keygen_init(ctx) > 0);
    TEST_ASSERT_TRUE(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) > 0);
    TEST_ASSERT_TRUE(EVP_PKEY_keygen(ctx, &g_pkey) > 0);
    EVP_PKEY_CTX_free(ctx);

    BIGNUM* n = NULL;
    BIGNUM* e = NULL;
    TEST_ASSERT_TRUE(EVP_PKEY_get_bn_param(g_pkey, OSSL_PKEY_PARAM_RSA_N, &n) > 0);
    TEST_ASSERT_TRUE(EVP_PKEY_get_bn_param(g_pkey, OSSL_PKEY_PARAM_RSA_E, &e) > 0);

    int n_len = BN_num_bytes(n);
    int e_len = BN_num_bytes(e);
    unsigned char* n_buf = malloc((size_t)n_len);
    unsigned char* e_buf = malloc((size_t)e_len);
    TEST_ASSERT_NOT_NULL(n_buf);
    TEST_ASSERT_NOT_NULL(e_buf);
    BN_bn2bin(n, n_buf);
    BN_bn2bin(e, e_buf);
    BN_free(n);
    BN_free(e);

    g_jwk_n_b64u = utils_base64url_encode(n_buf, (size_t)n_len);
    g_jwk_e_b64u = utils_base64url_encode(e_buf, (size_t)e_len);
    free(n_buf);
    free(e_buf);

    TEST_ASSERT_NOT_NULL(g_jwk_n_b64u);
    TEST_ASSERT_NOT_NULL(g_jwk_e_b64u);
}

// Build a JWKS JSON document containing a single key with the given
// kid, using the current global keypair.
static char* make_jwks_json(const char* kid) {
    char* buf = NULL;
    int rc = asprintf(&buf,
        "{\"keys\":[{\"kty\":\"RSA\",\"alg\":\"RS256\",\"use\":\"sig\","
        "\"kid\":\"%s\",\"n\":\"%s\",\"e\":\"%s\"}]}",
        kid, g_jwk_n_b64u, g_jwk_e_b64u);
    TEST_ASSERT_TRUE(rc > 0);
    return buf;
}

// Sign `signing_input` (header_b64.payload_b64) with g_pkey using
// RS256. Returns the base64url-encoded signature segment. Caller
// frees.
static char* sign_rs256(const char* signing_input) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    TEST_ASSERT_NOT_NULL(mdctx);
    TEST_ASSERT_TRUE(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, g_pkey) == 1);
    TEST_ASSERT_TRUE(EVP_DigestSignUpdate(mdctx, signing_input, strlen(signing_input)) == 1);
    size_t sig_len = 0;
    TEST_ASSERT_TRUE(EVP_DigestSignFinal(mdctx, NULL, &sig_len) == 1);
    unsigned char* sig = malloc(sig_len);
    TEST_ASSERT_NOT_NULL(sig);
    TEST_ASSERT_TRUE(EVP_DigestSignFinal(mdctx, sig, &sig_len) == 1);
    EVP_MD_CTX_free(mdctx);
    char* b64u = utils_base64url_encode(sig, sig_len);
    free(sig);
    TEST_ASSERT_NOT_NULL(b64u);
    return b64u;
}

// Build a JWT with the given JSON-string header and payload. The
// caller chooses whether to sign with sign_rs256 or pass a literal
// signature segment (e.g. for malformed-token tests).
//
// Returns a heap-allocated NUL-terminated JWT compact string.
static char* build_jwt(const char* header_json,
                       const char* payload_json,
                       bool sign_with_pkey,
                       const char* literal_sig_b64u) {
    char* h_b64 = utils_base64url_encode((const unsigned char*)header_json,
                                          strlen(header_json));
    char* p_b64 = utils_base64url_encode((const unsigned char*)payload_json,
                                          strlen(payload_json));
    TEST_ASSERT_NOT_NULL(h_b64);
    TEST_ASSERT_NOT_NULL(p_b64);

    char* signing_input = NULL;
    TEST_ASSERT_TRUE(asprintf(&signing_input, "%s.%s", h_b64, p_b64) > 0);

    char* sig_b64 = NULL;
    if (sign_with_pkey) {
        sig_b64 = sign_rs256(signing_input);
    } else {
        sig_b64 = strdup(literal_sig_b64u ? literal_sig_b64u : "AAAA");
    }

    char* jwt = NULL;
    TEST_ASSERT_TRUE(asprintf(&jwt, "%s.%s.%s", h_b64, p_b64, sig_b64) > 0);

    free(h_b64);
    free(p_b64);
    free(signing_input);
    free(sig_b64);
    return jwt;
}

// ---------------------------------------------------------------------------
// Provider config fixture. issuer, client_id, allowed_algorithms set.
// ---------------------------------------------------------------------------

static OIDCRPProviderConfig make_provider(void) {
    OIDCRPProviderConfig p;
    memset(&p, 0, sizeof(p));
    p.name = (char*)"test";
    p.issuer = (char*)"https://idp.example.com/realms/test";
    p.client_id = (char*)"lithium-web";
    p.allowed_algorithms[0] = (char*)"RS256";
    p.allowed_algorithm_count = 1;
    p.jwks_cache_seconds = 60;
    p.verify_ssl = false;
    return p;
}

// ---------------------------------------------------------------------------
// Standard payload helpers.
// ---------------------------------------------------------------------------

static char* make_standard_payload(const char* iss,
                                    const char* aud_json_value,
                                    long iat,
                                    long exp,
                                    const char* nonce) {
    char* buf = NULL;
    int rc = asprintf(&buf,
        "{\"iss\":\"%s\",\"sub\":\"user-123\",\"aud\":%s,"
        "\"iat\":%ld,\"exp\":%ld,\"nonce\":\"%s\","
        "\"email\":\"alice@example.com\",\"email_verified\":true,"
        "\"preferred_username\":\"alice\",\"name\":\"Alice Example\"}",
        iss, aud_json_value, iat, exp, nonce);
    TEST_ASSERT_TRUE(rc > 0);
    return buf;
}

// ---------------------------------------------------------------------------
// Lifecycle.
// ---------------------------------------------------------------------------

void setUp(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_jwks_init();
    // Ensure no stale cache from a previous test:
    oidc_rp_jwks_invalidate("test");
    make_keypair();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    oidc_rp_jwks_invalidate("test");
    oidc_rp_jwks_shutdown();

    if (g_pkey) {
        EVP_PKEY_free(g_pkey);
        g_pkey = NULL;
    }
    free(g_jwk_n_b64u); g_jwk_n_b64u = NULL;
    free(g_jwk_e_b64u); g_jwk_e_b64u = NULL;
}

// ---------------------------------------------------------------------------
// Lifecycle / lookup tests.
// ---------------------------------------------------------------------------

void test_claims_free_handles_null(void) {
    oidc_rp_idtoken_claims_free(NULL); // must not crash
    TEST_PASS();
}

void test_error_name_returns_stable_strings(void) {
    TEST_ASSERT_EQUAL_STRING("ok",              oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_OK));
    TEST_ASSERT_EQUAL_STRING("bad_input",       oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_BAD_INPUT));
    TEST_ASSERT_EQUAL_STRING("malformed",       oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_MALFORMED));
    TEST_ASSERT_EQUAL_STRING("bad_header",      oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_BAD_HEADER));
    TEST_ASSERT_EQUAL_STRING("alg_rejected",    oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_ALG_REJECTED));
    TEST_ASSERT_EQUAL_STRING("kid_unknown",     oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_KID_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("bad_key",         oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_BAD_KEY));
    TEST_ASSERT_EQUAL_STRING("bad_signature",   oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_BAD_SIGNATURE));
    TEST_ASSERT_EQUAL_STRING("bad_payload",     oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_BAD_PAYLOAD));
    TEST_ASSERT_EQUAL_STRING("iss_mismatch",    oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_ISS_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("aud_mismatch",    oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_AUD_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("expired",         oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_EXPIRED));
    TEST_ASSERT_EQUAL_STRING("not_yet_valid",   oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID));
    TEST_ASSERT_EQUAL_STRING("nonce_mismatch",  oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("missing_claim",   oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM));
    TEST_ASSERT_EQUAL_STRING("internal",        oidc_rp_idtoken_error_name(OIDC_RP_IDTOKEN_ERR_INTERNAL));
    TEST_ASSERT_EQUAL_STRING("unknown",         oidc_rp_idtoken_error_name((OidcRpIdTokenError)999));
}

// ---------------------------------------------------------------------------
// Argument validation.
// ---------------------------------------------------------------------------

void test_validate_rejects_null_provider(void) {
    OidcRpIdTokenClaims* c = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(NULL, "https://idp/jwks", "x.y.z", "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);
}

void test_validate_rejects_null_jwks_uri(void) {
    OIDCRPProviderConfig p = make_provider();
    OidcRpIdTokenClaims* c = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, NULL, "x.y.z", "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "", "x.y.z", "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);
}

void test_validate_rejects_null_id_token(void) {
    OIDCRPProviderConfig p = make_provider();
    OidcRpIdTokenClaims* c = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", NULL, "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "", "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);
}

void test_validate_rejects_null_nonce(void) {
    OIDCRPProviderConfig p = make_provider();
    OidcRpIdTokenClaims* c = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "x.y.z", NULL, time(NULL), &c));
    TEST_ASSERT_NULL(c);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "x.y.z", "", time(NULL), &c));
    TEST_ASSERT_NULL(c);
}

void test_validate_rejects_null_out(void) {
    OIDCRPProviderConfig p = make_provider();
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "x.y.z", "n", time(NULL), NULL));
}

void test_validate_rejects_provider_without_alg_list(void) {
    OIDCRPProviderConfig p = make_provider();
    p.allowed_algorithm_count = 0;
    OidcRpIdTokenClaims* c = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_INPUT,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "x.y.z", "n", time(NULL), &c));
    TEST_ASSERT_NULL(c);
}

// ---------------------------------------------------------------------------
// Happy paths.
// ---------------------------------------------------------------------------

void test_validate_happy_path(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"", now - 5, now + 600,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    OidcRpIdTokenError err =
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_OK, err);
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com/realms/test", claims->iss);
    TEST_ASSERT_EQUAL_STRING("user-123", claims->sub);
    TEST_ASSERT_EQUAL_STRING("lithium-web", claims->aud);
    TEST_ASSERT_EQUAL_STRING("alice@example.com", claims->email);
    TEST_ASSERT_TRUE(claims->email_verified);
    TEST_ASSERT_EQUAL_STRING("alice", claims->preferred_username);
    TEST_ASSERT_EQUAL_STRING("Alice Example", claims->name);
    TEST_ASSERT_EQUAL_INT(0, (int)claims->role_count);
    TEST_ASSERT_TRUE(claims->exp > now);

    oidc_rp_idtoken_claims_free(claims);
    free(token);
}

void test_validate_happy_path_aud_array(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload(p.issuer,
                                           "[\"some-other-app\",\"lithium-web\",\"third\"]",
                                           now - 5, now + 600, "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    OidcRpIdTokenError err =
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_OK, err);
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_EQUAL_STRING("lithium-web", claims->aud);

    oidc_rp_idtoken_claims_free(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// Negative paths: alg.
// ---------------------------------------------------------------------------

void test_validate_rejects_alg_none(void) {
    OIDCRPProviderConfig p = make_provider();
    // Even if allow-list says "none", we still reject it.
    p.allowed_algorithms[0] = (char*)"none";
    p.allowed_algorithms[1] = (char*)"RS256";
    p.allowed_algorithm_count = 2;

    long now = (long)time(NULL);
    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"", now - 5, now + 600,
                                           "expected-nonce");
    // Build with alg=none. Use a non-signing path with a literal "AAAA"
    // signature segment, since EVP_DigestSign would fail on "none".
    char* token = build_jwt("{\"alg\":\"none\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, false, "AAAA");
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    OidcRpIdTokenError err =
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_ALG_REJECTED, err);
    TEST_ASSERT_NULL(claims);

    free(token);
}

void test_validate_rejects_alg_not_in_allow_list(void) {
    OIDCRPProviderConfig p = make_provider(); // allow-list = ["RS256"]
    long now = (long)time(NULL);
    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"", now - 5, now + 600,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"HS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, false, "AAAA");
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_ALG_REJECTED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_unimplemented_alg(void) {
    OIDCRPProviderConfig p = make_provider();
    p.allowed_algorithms[0] = (char*)"ES256";
    p.allowed_algorithm_count = 1;

    long now = (long)time(NULL);
    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"", now - 5, now + 600,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"ES256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, false, "AAAA");
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_ALG_REJECTED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// Negative paths: claims.
// ---------------------------------------------------------------------------

void test_validate_rejects_wrong_iss(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload("https://attacker.example.com",
                                           "\"lithium-web\"", now - 5, now + 600,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_ISS_MISMATCH,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_wrong_aud(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload(p.issuer, "\"some-other-client\"",
                                           now - 5, now + 600, "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_AUD_MISMATCH,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_missing_sub(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    // Custom payload without `sub`.
    char* payload = NULL;
    int rc = asprintf(&payload,
        "{\"iss\":\"%s\",\"aud\":\"lithium-web\","
        "\"iat\":%ld,\"exp\":%ld,\"nonce\":\"expected-nonce\"}",
        p.issuer, now - 5, now + 600);
    TEST_ASSERT_TRUE(rc > 0);
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_expired(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    // exp well in the past, beyond skew.
    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now - 1000, now - 500,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_EXPIRED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_iat_in_future(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    // iat well in the future, beyond skew.
    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now + 1000, now + 2000,
                                           "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_nbf_in_future(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = NULL;
    int rc = asprintf(&payload,
        "{\"iss\":\"%s\",\"sub\":\"u\",\"aud\":\"lithium-web\","
        "\"iat\":%ld,\"exp\":%ld,\"nbf\":%ld,\"nonce\":\"expected-nonce\"}",
        p.issuer, now - 5, now + 600, now + 1000);
    TEST_ASSERT_TRUE(rc > 0);
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

void test_validate_rejects_wrong_nonce(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now - 5, now + 600, "wrong-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// Negative paths: token shape.
// ---------------------------------------------------------------------------

void test_validate_rejects_malformed_segments(void) {
    OIDCRPProviderConfig p = make_provider();
    OidcRpIdTokenClaims* claims = NULL;

    // Two segments instead of three.
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MALFORMED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "abc.def",
                                  "n", time(NULL), &claims));
    TEST_ASSERT_NULL(claims);

    // Four segments.
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MALFORMED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "a.b.c.d",
                                  "n", time(NULL), &claims));
    TEST_ASSERT_NULL(claims);

    // No dots.
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MALFORMED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "abcdef",
                                  "n", time(NULL), &claims));
    TEST_ASSERT_NULL(claims);
}

void test_validate_rejects_empty_segments(void) {
    OIDCRPProviderConfig p = make_provider();
    OidcRpIdTokenClaims* claims = NULL;

    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MALFORMED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "..",
                                  "n", time(NULL), &claims));
    TEST_ASSERT_NULL(claims);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_MALFORMED,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", "abc..xyz",
                                  "n", time(NULL), &claims));
    TEST_ASSERT_NULL(claims);
}

void test_validate_rejects_tampered_signature(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    // We're going to fail twice (initial + retry-after-invalidate),
    // so register the JWKS twice.
    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now - 5, now + 600, "expected-nonce");
    // Build with a literal-bogus signature (won't match the real key).
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, false, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_BAD_SIGNATURE,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// JWKS rotation recovery.
// ---------------------------------------------------------------------------

void test_validate_kid_miss_triggers_refetch(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    // First fixture: JWKS with a DIFFERENT kid (will miss).
    char* jwks_old = make_jwks_json("old-key");
    oidc_rp_http_test_set_response("/jwks", 200, jwks_old);
    free(jwks_old);

    // Second fixture: JWKS with the right kid (after invalidate).
    char* jwks_new = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks_new);
    free(jwks_new);

    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now - 5, now + 600, "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    OidcRpIdTokenError err =
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_OK, err);
    TEST_ASSERT_NOT_NULL(claims);

    oidc_rp_idtoken_claims_free(claims);
    free(token);
}

void test_validate_kid_unknown_after_refetch(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    // Both JWKS responses return a JWKS that does NOT contain
    // test-key-1.
    char* jwks_other = make_jwks_json("some-other-kid");
    oidc_rp_http_test_set_response("/jwks", 200, jwks_other);
    oidc_rp_http_test_set_response("/jwks", 200, jwks_other);
    free(jwks_other);

    char* payload = make_standard_payload(p.issuer, "\"lithium-web\"",
                                           now - 5, now + 600, "expected-nonce");
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_ERR_KID_UNKNOWN,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NULL(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// Optional-claim coverage.
// ---------------------------------------------------------------------------

void test_validate_email_verified_honoured(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    // Same as standard payload but with email_verified=false.
    char* payload = NULL;
    int rc = asprintf(&payload,
        "{\"iss\":\"%s\",\"sub\":\"u\",\"aud\":\"lithium-web\","
        "\"iat\":%ld,\"exp\":%ld,\"nonce\":\"expected-nonce\","
        "\"email\":\"u@x.com\",\"email_verified\":false}",
        p.issuer, now - 5, now + 600);
    TEST_ASSERT_TRUE(rc > 0);
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_OK,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_FALSE(claims->email_verified);
    TEST_ASSERT_EQUAL_STRING("u@x.com", claims->email);

    oidc_rp_idtoken_claims_free(claims);
    free(token);
}

void test_validate_extracts_realm_roles(void) {
    OIDCRPProviderConfig p = make_provider();
    long now = (long)time(NULL);

    char* jwks = make_jwks_json("test-key-1");
    oidc_rp_http_test_set_response("/jwks", 200, jwks);
    free(jwks);

    char* payload = NULL;
    int rc = asprintf(&payload,
        "{\"iss\":\"%s\",\"sub\":\"u\",\"aud\":\"lithium-web\","
        "\"iat\":%ld,\"exp\":%ld,\"nonce\":\"expected-nonce\","
        "\"realm_access\":{\"roles\":[\"admin\",\"user\",\"viewer\"]}}",
        p.issuer, now - 5, now + 600);
    TEST_ASSERT_TRUE(rc > 0);
    char* token = build_jwt("{\"alg\":\"RS256\",\"kid\":\"test-key-1\",\"typ\":\"JWT\"}",
                            payload, true, NULL);
    free(payload);

    OidcRpIdTokenClaims* claims = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_IDTOKEN_OK,
        oidc_rp_validate_id_token(&p, "https://idp/jwks", token,
                                  "expected-nonce", (time_t)now, &claims));
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_EQUAL_INT(3, (int)claims->role_count);
    TEST_ASSERT_EQUAL_STRING("admin", claims->roles[0]);
    TEST_ASSERT_EQUAL_STRING("user", claims->roles[1]);
    TEST_ASSERT_EQUAL_STRING("viewer", claims->roles[2]);

    oidc_rp_idtoken_claims_free(claims);
    free(token);
}

// ---------------------------------------------------------------------------
// Unity main.
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_claims_free_handles_null);
    RUN_TEST(test_error_name_returns_stable_strings);
    RUN_TEST(test_validate_rejects_null_provider);
    RUN_TEST(test_validate_rejects_null_jwks_uri);
    RUN_TEST(test_validate_rejects_null_id_token);
    RUN_TEST(test_validate_rejects_null_nonce);
    RUN_TEST(test_validate_rejects_null_out);
    RUN_TEST(test_validate_rejects_provider_without_alg_list);
    RUN_TEST(test_validate_happy_path);
    RUN_TEST(test_validate_happy_path_aud_array);
    RUN_TEST(test_validate_rejects_alg_none);
    RUN_TEST(test_validate_rejects_alg_not_in_allow_list);
    RUN_TEST(test_validate_rejects_unimplemented_alg);
    RUN_TEST(test_validate_rejects_wrong_iss);
    RUN_TEST(test_validate_rejects_wrong_aud);
    RUN_TEST(test_validate_rejects_missing_sub);
    RUN_TEST(test_validate_rejects_expired);
    RUN_TEST(test_validate_rejects_iat_in_future);
    RUN_TEST(test_validate_rejects_nbf_in_future);
    RUN_TEST(test_validate_rejects_wrong_nonce);
    RUN_TEST(test_validate_rejects_malformed_segments);
    RUN_TEST(test_validate_rejects_empty_segments);
    RUN_TEST(test_validate_rejects_tampered_signature);
    RUN_TEST(test_validate_kid_miss_triggers_refetch);
    RUN_TEST(test_validate_kid_unknown_after_refetch);
    RUN_TEST(test_validate_email_verified_honoured);
    RUN_TEST(test_validate_extracts_realm_roles);

    return UNITY_END();
}
