/**
 * @file oidc_rp_idtoken.c
 * @brief OIDC Relying Party — ID token validation (Phase 12).
 *
 * Implements `oidc_rp_validate_id_token` and supporting helpers.
 * See header for contract details.
 */

#include <src/hydrogen.h>

#include "oidc_rp_idtoken.h"
#include "oidc_rp_jwks.h"

#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include <openssl/evp.h>

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Sensitive-buffer scrubbing helper (matches Phase 7/8/10/11 discipline).
// ---------------------------------------------------------------------------
static void scrub_and_free(char* p) {
    if (!p) return;
    volatile char* v = (volatile char*)p;
    while (*v) *v++ = 0;
    free(p);
}

static void scrub_bytes_and_free(unsigned char* p, size_t n) {
    if (!p) return;
    volatile unsigned char* v = (volatile unsigned char*)p;
    for (size_t i = 0; i < n; ++i) v[i] = 0;
    free(p);
}

// ---------------------------------------------------------------------------
// Public lifecycle helpers.
// ---------------------------------------------------------------------------

void oidc_rp_idtoken_claims_free(OidcRpIdTokenClaims* claims) {
    if (!claims) return;
    free(claims->iss);
    free(claims->sub);
    free(claims->aud);
    scrub_and_free(claims->email);            // PII
    free(claims->preferred_username);
    free(claims->name);
    for (size_t i = 0; i < claims->role_count && i < OIDC_RP_IDTOKEN_MAX_ROLES; ++i) {
        free(claims->roles[i]);
    }
    free(claims);
}

const char* oidc_rp_idtoken_error_name(OidcRpIdTokenError err) {
    switch (err) {
        case OIDC_RP_IDTOKEN_OK:                  return "ok";
        case OIDC_RP_IDTOKEN_ERR_BAD_INPUT:       return "bad_input";
        case OIDC_RP_IDTOKEN_ERR_MALFORMED:       return "malformed";
        case OIDC_RP_IDTOKEN_ERR_BAD_HEADER:      return "bad_header";
        case OIDC_RP_IDTOKEN_ERR_ALG_REJECTED:    return "alg_rejected";
        case OIDC_RP_IDTOKEN_ERR_KID_UNKNOWN:     return "kid_unknown";
        case OIDC_RP_IDTOKEN_ERR_BAD_KEY:         return "bad_key";
        case OIDC_RP_IDTOKEN_ERR_BAD_SIGNATURE:   return "bad_signature";
        case OIDC_RP_IDTOKEN_ERR_BAD_PAYLOAD:     return "bad_payload";
        case OIDC_RP_IDTOKEN_ERR_ISS_MISMATCH:    return "iss_mismatch";
        case OIDC_RP_IDTOKEN_ERR_AUD_MISMATCH:    return "aud_mismatch";
        case OIDC_RP_IDTOKEN_ERR_EXPIRED:         return "expired";
        case OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID:   return "not_yet_valid";
        case OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH:  return "nonce_mismatch";
        case OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM:   return "missing_claim";
        case OIDC_RP_IDTOKEN_ERR_INTERNAL:        return "internal";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// JWS splitter and segment decoders.
// ---------------------------------------------------------------------------

// Split `id_token` into three substrings on '.' boundaries. Returns
// false if the token has the wrong number of segments. The output
// pointers reference into a heap-allocated buffer that the caller
// must free via `free(*out_buf)`.
static bool split_jws(const char* id_token,
                      char** out_buf,
                      char** header_b64,
                      char** payload_b64,
                      char** sig_b64,
                      size_t* signing_input_len) {
    *out_buf = NULL;
    *header_b64 = *payload_b64 = *sig_b64 = NULL;
    *signing_input_len = 0;

    size_t len = strlen(id_token);
    char* buf = malloc(len + 1);
    if (!buf) return false;
    memcpy(buf, id_token, len + 1);

    char* dot1 = strchr(buf, '.');
    if (!dot1) { free(buf); return false; }
    char* dot2 = strchr(dot1 + 1, '.');
    if (!dot2) { free(buf); return false; }
    if (strchr(dot2 + 1, '.') != NULL) {
        // More than three segments — JWE is not supported here.
        free(buf);
        return false;
    }

    *signing_input_len = (size_t)(dot2 - buf); // length of header.payload (excluding sig)
    *dot1 = '\0';
    *dot2 = '\0';

    *header_b64  = buf;
    *payload_b64 = dot1 + 1;
    *sig_b64     = dot2 + 1;
    *out_buf     = buf;

    if (**header_b64 == '\0' || **payload_b64 == '\0' || **sig_b64 == '\0') {
        free(buf);
        *out_buf = *header_b64 = *payload_b64 = *sig_b64 = NULL;
        *signing_input_len = 0;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Header parsing.
// ---------------------------------------------------------------------------

// Decode and parse the JOSE header, returning the `alg` and `kid`.
// Both values are heap-allocated copies (caller frees).
static OidcRpIdTokenError parse_header(const char* header_b64,
                                       char** out_alg,
                                       char** out_kid) {
    *out_alg = *out_kid = NULL;

    size_t hlen = 0;
    unsigned char* hraw = utils_base64url_decode(header_b64, &hlen);
    if (!hraw || hlen == 0) {
        free(hraw);
        return OIDC_RP_IDTOKEN_ERR_MALFORMED;
    }

    // jansson needs a NUL-terminated string.
    char* hstr = malloc(hlen + 1);
    if (!hstr) { free(hraw); return OIDC_RP_IDTOKEN_ERR_INTERNAL; }
    memcpy(hstr, hraw, hlen);
    hstr[hlen] = '\0';
    free(hraw);

    json_error_t jerr;
    json_t* root = json_loads(hstr, 0, &jerr);
    free(hstr);
    if (!root || !json_is_object(root)) {
        if (root) json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_BAD_HEADER;
    }

    const json_t* j_alg = json_object_get(root, "alg");
    const json_t* j_kid = json_object_get(root, "kid");

    if (!json_is_string(j_alg)) {
        json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_BAD_HEADER;
    }

    *out_alg = strdup(json_string_value(j_alg));
    if (!*out_alg) {
        json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_INTERNAL;
    }

    // `kid` is technically optional in JOSE, but for our use-case (a
    // multi-key JWKS where the IdP rotates keys) it is required to
    // pick the right verification key. Treat absent kid as a header
    // error.
    if (!json_is_string(j_kid)) {
        free(*out_alg);
        *out_alg = NULL;
        json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_BAD_HEADER;
    }
    *out_kid = strdup(json_string_value(j_kid));
    if (!*out_kid) {
        free(*out_alg);
        *out_alg = NULL;
        json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_INTERNAL;
    }

    json_decref(root);
    return OIDC_RP_IDTOKEN_OK;
}

// Returns true if `alg` matches one of the provider's allowed_algorithms
// AND is not "none" (case-sensitive — "none" lower-case is the only
// form RFC 7515 defines, and we reject it explicitly).
static bool alg_is_allowed(const OIDCRPProviderConfig* provider, const char* alg) {
    if (strcmp(alg, "none") == 0) return false;
    for (size_t i = 0; i < provider->allowed_algorithm_count; ++i) {
        if (provider->allowed_algorithms[i] &&
            strcmp(provider->allowed_algorithms[i], alg) == 0) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Signature verification (with one rotation-recovery retry).
// ---------------------------------------------------------------------------

static OidcRpIdTokenError verify_signature(const OIDCRPProviderConfig* provider,
                                           const char* jwks_uri,
                                           const char* kid,
                                           const unsigned char* signing_input,
                                           size_t signing_input_len,
                                           const unsigned char* signature,
                                           size_t sig_len) {
    int ttl = provider->jwks_cache_seconds;
    bool retried = false;

    // Loop bound: at most two iterations. The bool `retried` is the
    // bound — every `continue` path sets it to true exactly once,
    // and the second iteration's failure paths return rather than
    // continuing. Verify by inspection: there are exactly four
    // `return` statements and exactly two `continue` statements,
    // each `continue` gated on `!retried`.
    //
    // Note: a NULL return from `oidc_rp_jwks_find` after refetch
    // collapses both "kid not in JWKS" and "JWKS refetch failed"
    // into KID_UNKNOWN. Phase 14 may want to map a transient HTTP
    // failure on refetch to a different user-facing code; for now,
    // the retry-and-give-up shape matches the plan's contract.
    for (;;) {
        const OidcRpJwk* jwk = oidc_rp_jwks_find(provider->name, jwks_uri,
                                                  provider->verify_ssl, ttl, kid);
        if (!jwk) {
            // First miss: invalidate and retry once. This handles
            // signing-key rotation while the IdP rotates atomically:
            // a token signed by key K2 may arrive before our cache
            // sees K2 in the JWKS endpoint.
            if (!retried) {
                retried = true;
                oidc_rp_jwks_invalidate(provider->name);
                continue;
            }
            return OIDC_RP_IDTOKEN_ERR_KID_UNKNOWN;
        }

        // Copy the JWK JSON locally — the cache pointer can be
        // invalidated by another caller between now and the OpenSSL
        // call (per Phase 9's "valid until next mutating call"
        // contract).
        char* jwk_json_copy = jwk->json_text ? strdup(jwk->json_text) : NULL;
        if (!jwk_json_copy) {
            return OIDC_RP_IDTOKEN_ERR_INTERNAL;
        }

        EVP_PKEY* pkey = utils_jwk_rsa_to_pkey(jwk_json_copy);
        scrub_and_free(jwk_json_copy);
        if (!pkey) {
            return OIDC_RP_IDTOKEN_ERR_BAD_KEY;
        }

        bool ok = utils_rs256_verify(pkey, signing_input, signing_input_len,
                                     signature, sig_len);
        EVP_PKEY_free(pkey);

        if (ok) return OIDC_RP_IDTOKEN_OK;

        // Verification failed against the currently-cached key.
        // Per the plan: invalidate JWKS and retry exactly once
        // (rotation-recovery). On the second failure, declare bad
        // signature.
        if (!retried) {
            retried = true;
            oidc_rp_jwks_invalidate(provider->name);
            continue;
        }
        return OIDC_RP_IDTOKEN_ERR_BAD_SIGNATURE;
    }
}

// ---------------------------------------------------------------------------
// Payload parsing & claim checks.
// ---------------------------------------------------------------------------

// Helper: copy the value of a string field if present and non-empty.
// On allocation failure, returns false (caller should treat as
// INTERNAL error).
static bool copy_string_field(json_t* root, const char* key, char** out) {
    *out = NULL;
    const json_t* j = json_object_get(root, key);
    if (!json_is_string(j)) return true; // absent is fine for optional fields
    const char* s = json_string_value(j);
    if (!s || !*s) return true;
    *out = strdup(s);
    return *out != NULL;
}

// Check that aud claim contains the expected client_id. aud may be a
// single string or a JSON array per OIDC Core 1.0 §2. On match,
// returns the matched string (heap-allocated copy, caller frees) via
// out_match.
static bool aud_contains(json_t* root, const char* expected, char** out_match) {
    *out_match = NULL;
    const json_t* aud = json_object_get(root, "aud");
    if (!aud) return false;

    if (json_is_string(aud)) {
        const char* s = json_string_value(aud);
        if (s && strcmp(s, expected) == 0) {
            *out_match = strdup(s);
            return *out_match != NULL;
        }
        return false;
    }

    if (json_is_array(aud)) {
        size_t n = json_array_size(aud);
        for (size_t i = 0; i < n; ++i) {
            const json_t* item = json_array_get(aud, i);
            if (!json_is_string(item)) continue;
            const char* s = json_string_value(item);
            if (s && strcmp(s, expected) == 0) {
                *out_match = strdup(s);
                return *out_match != NULL;
            }
        }
    }
    return false;
}

// Read realm_access.roles into the claims struct. Tolerant: missing
// or wrong-typed claim => zero roles (OIDC Core does not require
// realm_access; it's a Keycloak extension).
static void copy_realm_roles(json_t* root, OidcRpIdTokenClaims* claims) {
    const json_t* realm_access = json_object_get(root, "realm_access");
    if (!json_is_object(realm_access)) return;
    const json_t* roles = json_object_get(realm_access, "roles");
    if (!json_is_array(roles)) return;

    size_t n = json_array_size(roles);
    if (n > OIDC_RP_IDTOKEN_MAX_ROLES) n = OIDC_RP_IDTOKEN_MAX_ROLES;
    for (size_t i = 0; i < n; ++i) {
        const json_t* item = json_array_get(roles, i);
        if (!json_is_string(item)) continue;
        const char* s = json_string_value(item);
        if (!s || !*s) continue;
        char* dup = strdup(s);
        if (!dup) continue; // soft-fail individual role copy
        claims->roles[claims->role_count++] = dup;
    }
}

static OidcRpIdTokenError parse_payload_and_check(const char* payload_b64,
                                                  const OIDCRPProviderConfig* provider,
                                                  const char* expected_nonce,
                                                  time_t now,
                                                  OidcRpIdTokenClaims** out_claims) {
    *out_claims = NULL;

    size_t plen = 0;
    unsigned char* praw = utils_base64url_decode(payload_b64, &plen);
    if (!praw || plen == 0) {
        free(praw);
        return OIDC_RP_IDTOKEN_ERR_MALFORMED;
    }

    char* pstr = malloc(plen + 1);
    if (!pstr) { free(praw); return OIDC_RP_IDTOKEN_ERR_INTERNAL; }
    memcpy(pstr, praw, plen);
    pstr[plen] = '\0';
    free(praw);

    json_error_t jerr;
    json_t* root = json_loads(pstr, 0, &jerr);
    free(pstr);
    if (!root || !json_is_object(root)) {
        if (root) json_decref(root);
        return OIDC_RP_IDTOKEN_ERR_BAD_PAYLOAD;
    }

    OidcRpIdTokenError result = OIDC_RP_IDTOKEN_ERR_INTERNAL;

    // ---- iss ----
    const json_t* j_iss = json_object_get(root, "iss");
    if (!json_is_string(j_iss)) {
        result = OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM;
        goto done;
    }
    if (strcmp(json_string_value(j_iss), provider->issuer) != 0) {
        log_this(SR_AUTH,
                 "OIDC_RP: id_token iss mismatch (got=%s expected=%s)",
                 LOG_LEVEL_ALERT, 2,
                 json_string_value(j_iss), provider->issuer);
        result = OIDC_RP_IDTOKEN_ERR_ISS_MISMATCH;
        goto done;
    }

    // ---- aud ----
    char* aud_match = NULL;
    if (!aud_contains(root, provider->client_id, &aud_match)) {
        log_this(SR_AUTH,
                 "OIDC_RP: id_token aud does not contain client_id=%s",
                 LOG_LEVEL_ALERT, 1, provider->client_id);
        result = OIDC_RP_IDTOKEN_ERR_AUD_MISMATCH;
        goto done;
    }

    // ---- sub ----
    const json_t* j_sub = json_object_get(root, "sub");
    if (!json_is_string(j_sub) || !*json_string_value(j_sub)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM;
        goto done;
    }

    // ---- exp ----
    const json_t* j_exp = json_object_get(root, "exp");
    if (!json_is_integer(j_exp) && !json_is_real(j_exp)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM;
        goto done;
    }
    long exp = (long)(json_is_integer(j_exp) ? json_integer_value(j_exp)
                                              : (long long)json_real_value(j_exp));

    // ---- iat (required by OIDC Core 1.0) ----
    const json_t* j_iat = json_object_get(root, "iat");
    if (!json_is_integer(j_iat) && !json_is_real(j_iat)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM;
        goto done;
    }
    long iat = (long)(json_is_integer(j_iat) ? json_integer_value(j_iat)
                                              : (long long)json_real_value(j_iat));

    // ---- nbf (optional) ----
    long nbf = 0;
    const json_t* j_nbf = json_object_get(root, "nbf");
    if (json_is_integer(j_nbf)) {
        nbf = (long)json_integer_value(j_nbf);
    } else if (json_is_real(j_nbf)) {
        nbf = (long)(long long)json_real_value(j_nbf);
    }

    long skew = OIDC_RP_IDTOKEN_DEFAULT_SKEW_SECONDS;

    if (exp <= ((long)now - skew)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_EXPIRED;
        goto done;
    }
    if (iat > ((long)now + skew)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID;
        goto done;
    }
    if (nbf != 0 && nbf > ((long)now + skew)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID;
        goto done;
    }

    // ---- nonce ----
    const json_t* j_nonce = json_object_get(root, "nonce");
    if (!json_is_string(j_nonce)) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH;
        goto done;
    }
    if (strcmp(json_string_value(j_nonce), expected_nonce) != 0) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH;
        goto done;
    }

    // ---- All checks passed; assemble the claims struct ----
    OidcRpIdTokenClaims* claims = calloc(1, sizeof(*claims));
    if (!claims) {
        free(aud_match);
        result = OIDC_RP_IDTOKEN_ERR_INTERNAL;
        goto done;
    }

    claims->aud = aud_match; // transferred ownership
    claims->iss = strdup(json_string_value(j_iss));
    claims->sub = strdup(json_string_value(j_sub));
    claims->exp = exp;
    claims->iat = iat;
    claims->nbf = nbf;

    if (!claims->iss || !claims->sub) {
        oidc_rp_idtoken_claims_free(claims);
        result = OIDC_RP_IDTOKEN_ERR_INTERNAL;
        goto done;
    }

    bool ok = true;
    ok &= copy_string_field(root, "email", &claims->email);
    ok &= copy_string_field(root, "preferred_username", &claims->preferred_username);
    ok &= copy_string_field(root, "name", &claims->name);
    if (!ok) {
        oidc_rp_idtoken_claims_free(claims);
        result = OIDC_RP_IDTOKEN_ERR_INTERNAL;
        goto done;
    }

    const json_t* j_email_verified = json_object_get(root, "email_verified");
    claims->email_verified = json_is_true(j_email_verified);

    copy_realm_roles(root, claims);

    *out_claims = claims;
    result = OIDC_RP_IDTOKEN_OK;

done:
    json_decref(root);
    return result;
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

OidcRpIdTokenError oidc_rp_validate_id_token(const OIDCRPProviderConfig* provider,
                                             const char* jwks_uri,
                                             const char* id_token,
                                             const char* expected_nonce,
                                             time_t now,
                                             OidcRpIdTokenClaims** out_claims) {
    if (out_claims) *out_claims = NULL;

    if (!provider || !provider->issuer || !*provider->issuer ||
        !provider->client_id || !*provider->client_id ||
        provider->allowed_algorithm_count == 0 ||
        !provider->name || !*provider->name ||
        !jwks_uri || !*jwks_uri ||
        !id_token || !*id_token ||
        !expected_nonce || !*expected_nonce ||
        !out_claims) {
        return OIDC_RP_IDTOKEN_ERR_BAD_INPUT;
    }

    // Step 1: split.
    char* buf = NULL;
    char* header_b64 = NULL;
    char* payload_b64 = NULL;
    char* sig_b64 = NULL;
    size_t signing_input_len = 0;
    if (!split_jws(id_token, &buf, &header_b64, &payload_b64, &sig_b64, &signing_input_len)) {
        return OIDC_RP_IDTOKEN_ERR_MALFORMED;
    }

    OidcRpIdTokenError result = OIDC_RP_IDTOKEN_ERR_INTERNAL;
    char* alg = NULL;
    char* kid = NULL;
    unsigned char* sig_bytes = NULL;
    size_t sig_bytes_len = 0;
    unsigned char* signing_input = NULL;
    OidcRpIdTokenClaims* claims = NULL;

    // Step 2: header + alg gate.
    result = parse_header(header_b64, &alg, &kid);
    if (result != OIDC_RP_IDTOKEN_OK) goto cleanup;

    if (!alg_is_allowed(provider, alg)) {
        log_this(SR_AUTH,
                 "OIDC_RP: id_token rejected (alg=%s not in allow-list)",
                 LOG_LEVEL_ALERT, 1, alg);
        result = OIDC_RP_IDTOKEN_ERR_ALG_REJECTED;
        goto cleanup;
    }

    // Currently only RS256 is implemented in the verifier. The
    // allow-list could in principle contain other RS-family entries,
    // but until utils_rs256_verify gains a family parameter, treat
    // anything other than RS256 as not-yet-supported.
    //
    // Reachability note: with the Phase 5 default
    // `AllowedAlgorithms = ["RS256"]` (and the empty-array fallback
    // to the same), this branch cannot trigger in production — the
    // earlier `alg_is_allowed` check at line above filters out any
    // non-RS256 alg first. The branch only fires if an operator
    // opts into a non-RS256 allow-list entry (e.g. ES256), which
    // the Unity test `test_validate_rejects_unimplemented_alg`
    // exercises explicitly. Keeping this defence makes a config
    // mistake fail closed rather than reach an unimplemented
    // verifier path.
    if (strcmp(alg, "RS256") != 0) {
        log_this(SR_AUTH,
                 "OIDC_RP: id_token rejected (alg=%s not yet implemented in verifier)",
                 LOG_LEVEL_ALERT, 1, alg);
        result = OIDC_RP_IDTOKEN_ERR_ALG_REJECTED;
        goto cleanup;
    }

    // Step 3 + 4: signing-input and signature decode + verify.
    sig_bytes = utils_base64url_decode(sig_b64, &sig_bytes_len);
    if (!sig_bytes || sig_bytes_len == 0) {
        result = OIDC_RP_IDTOKEN_ERR_MALFORMED;
        goto cleanup;
    }

    // signing_input is "header_b64.payload_b64" — the bytes split_jws
    // already concatenated in `buf` before we NUL-terminated dot1.
    // Reconstruct on the heap to avoid mutating buf.
    signing_input = malloc(signing_input_len);
    if (!signing_input) {
        result = OIDC_RP_IDTOKEN_ERR_INTERNAL;
        goto cleanup;
    }
    {
        size_t hlen = strlen(header_b64);
        size_t plen = strlen(payload_b64);
        // signing_input_len == hlen + 1 + plen
        memcpy(signing_input, header_b64, hlen);
        signing_input[hlen] = '.';
        memcpy(signing_input + hlen + 1, payload_b64, plen);
    }

    result = verify_signature(provider, jwks_uri, kid,
                              signing_input, signing_input_len,
                              sig_bytes, sig_bytes_len);
    if (result != OIDC_RP_IDTOKEN_OK) goto cleanup;

    // Step 5 + 6: payload claims.
    result = parse_payload_and_check(payload_b64, provider, expected_nonce,
                                     now, &claims);
    if (result != OIDC_RP_IDTOKEN_OK) goto cleanup;

    *out_claims = claims;
    claims = NULL; // ownership transferred

cleanup:
    free(alg);
    free(kid);
    if (sig_bytes) scrub_bytes_and_free(sig_bytes, sig_bytes_len);
    free(signing_input);
    free(buf);
    if (claims) oidc_rp_idtoken_claims_free(claims);
    return result;
}
