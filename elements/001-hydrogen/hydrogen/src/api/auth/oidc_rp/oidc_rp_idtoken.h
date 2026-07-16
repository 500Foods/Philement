/**
 * @file oidc_rp_idtoken.h
 * @brief OIDC Relying Party — ID token validation.
 *
 * Phase 12 of the OIDC plan (`docs/OIDC-PLAN.md`). This module
 * validates an OpenID Connect `id_token` produced by the IdP (typically
 * Keycloak), as received from `oidc_rp_exchange_code` (Phase 11).
 *
 * Validation steps, in order (per OIDC Core 1.0 §3.1.3.7):
 *   1. Split the JWT into header/payload/signature segments.
 *   2. Parse the JOSE header; require `alg` to appear in the
 *      provider's `allowed_algorithms` list. **Reject `alg=none`.**
 *   3. Find the JWK for the header's `kid` in the cached JWKS
 *      (Phase 9). On miss, invalidate the cache and refetch once;
 *      give up after the second miss (handles signing-key rotation).
 *   4. Verify the RS256 signature against the JWK's RSA public key.
 *   5. Parse the JSON payload and check the standard claims:
 *      - `iss` exact-equal to `provider->issuer`,
 *      - `aud` contains `provider->client_id` (string or array),
 *      - `exp > now - skew`,
 *      - `iat <= now + skew`,
 *      - `nbf <= now + skew` (when present),
 *      - `nonce == expected_nonce`,
 *      - `sub` non-empty (required by OIDC Core).
 *   6. Populate an `OidcRpIdTokenClaims` struct with the fields
 *      Phase 14 (callback wiring) and Phase 22 (role mapping)
 *      consume.
 *
 * Currently only RS256 is supported. The `alg` allow-list is the
 * only gate that determines which algorithms a caller will accept
 * — adding ES256 / PS256 in a future revision means extending
 * `utils_rs256_verify`'s family and updating this validator's
 * dispatch.
 *
 * Logging policy: the validator NEVER logs the raw token, the
 * signature segment, the nonce, or the email. It logs the typed
 * error code, the `kid` (truncated for safety), and the `iss`/`aud`
 * mismatch values when those are the failure cause.
 *
 * Thread safety: every public function is reentrant and uses no
 * shared state of its own. The JWKS cache it consults (Phase 9) is
 * already thread-safe.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_IDTOKEN_H
#define OIDC_RP_IDTOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <jansson.h>

#include <src/config/config_oidc_rp.h>

/**
 * @brief Typed error code for ID-token validation.
 *
 * Stable across releases. Phase 14 will map each value to a typed
 * `?oidc_error=<name>` redirect parameter; the user-facing error
 * surface stays small and predictable.
 */
typedef enum OidcRpIdTokenError {
    OIDC_RP_IDTOKEN_OK = 0,                 // Validation passed
    OIDC_RP_IDTOKEN_ERR_BAD_INPUT = 1,      // NULL/empty argument
    OIDC_RP_IDTOKEN_ERR_MALFORMED = 2,      // Wrong number of segments / non-base64url
    OIDC_RP_IDTOKEN_ERR_BAD_HEADER = 3,     // Header JSON unparseable / missing alg/kid
    OIDC_RP_IDTOKEN_ERR_ALG_REJECTED = 4,   // alg not in allow-list (incl. "none")
    OIDC_RP_IDTOKEN_ERR_KID_UNKNOWN = 5,    // No JWK with matching kid even after refetch
    OIDC_RP_IDTOKEN_ERR_BAD_KEY = 6,        // JWK present but unparseable into EVP_PKEY
    OIDC_RP_IDTOKEN_ERR_BAD_SIGNATURE = 7,  // Cryptographic verify failed
    OIDC_RP_IDTOKEN_ERR_BAD_PAYLOAD = 8,    // Payload JSON unparseable
    OIDC_RP_IDTOKEN_ERR_ISS_MISMATCH = 9,   // iss != provider->issuer
    OIDC_RP_IDTOKEN_ERR_AUD_MISMATCH = 10,  // aud does not contain client_id
    OIDC_RP_IDTOKEN_ERR_EXPIRED = 11,       // exp <= now - skew, or missing
    OIDC_RP_IDTOKEN_ERR_NOT_YET_VALID = 12, // iat > now + skew, or nbf > now + skew
    OIDC_RP_IDTOKEN_ERR_NONCE_MISMATCH = 13,// nonce claim != expected
    OIDC_RP_IDTOKEN_ERR_MISSING_CLAIM = 14, // Required claim (sub, iss, aud, exp) absent
    OIDC_RP_IDTOKEN_ERR_INTERNAL = 15       // OpenSSL / allocation failure
} OidcRpIdTokenError;

/**
 * @brief Maximum number of role strings retained from a single
 * `realm_access.roles` array. Plenty of headroom for typical
 * Keycloak deployments.
 */
#define OIDC_RP_IDTOKEN_MAX_ROLES 32

/**
 * @brief Validated ID-token claims.
 *
 * All string fields are heap-allocated copies owned by the struct;
 * free via `oidc_rp_idtoken_claims_free`. NULL is normal for any
 * optional field that the IdP did not include.
 *
 * `roles` is the parsed `realm_access.roles` array from the ID
 * token, retained verbatim. Phase 22's role-mapping logic decides
 * whether to honour, prefix, or ignore this list. `roles[i]` strings
 * are owned; the array is statically sized at `OIDC_RP_IDTOKEN_MAX_ROLES`
 * with `role_count` indicating how many slots are populated.
 */
typedef struct OidcRpIdTokenClaims {
    char* iss;                                       // Required
    char* sub;                                       // Required
    char* aud;                                       // Required (may be a JSON array; we
                                                     //   keep the matched client_id verbatim)
    char* email;                                     // Optional
    bool  email_verified;
    char* preferred_username;                        // Optional
    char* name;                                      // Optional
    char* roles[OIDC_RP_IDTOKEN_MAX_ROLES];          // realm_access.roles (optional)
    size_t role_count;
    long  exp;                                       // Unix seconds
    long  iat;                                       // Unix seconds
    long  nbf;                                       // 0 if not present
} OidcRpIdTokenClaims;

/**
 * @brief Free an `OidcRpIdTokenClaims` and its string fields.
 *
 * NULL-safe. Email and other PII fields are scrubbed before free,
 * consistent with the Phase 7 / 8 / 10 / 11 hardening discipline.
 */
void oidc_rp_idtoken_claims_free(OidcRpIdTokenClaims* claims);

/**
 * @brief Convert a typed error code to a stable, lowercase string.
 *
 * Useful for logging and for mapping into `oidc_error=` redirect
 * parameters. Never returns NULL — unknown values map to
 * `"unknown"`.
 */
const char* oidc_rp_idtoken_error_name(OidcRpIdTokenError err);

/**
 * @brief Validate an OIDC `id_token` and extract its claims.
 *
 * Performs the full validation chain described in the file header.
 * On success, populates `*out_claims` with a newly-allocated
 * `OidcRpIdTokenClaims*` and returns `OIDC_RP_IDTOKEN_OK`. On any
 * failure path, `*out_claims` is set to NULL and an appropriate
 * typed error is returned.
 *
 * The validator delegates JWK lookup to `oidc_rp_jwks_find`. On a
 * `kid` miss against an already-cached set, it calls
 * `oidc_rp_jwks_invalidate` and retries the lookup once before
 * giving up.
 *
 * Clock skew is fixed at `OIDC_RP_IDTOKEN_DEFAULT_SKEW_SECONDS`
 * (60 s). A future revision may make this provider-configurable.
 *
 * @param provider        Active provider config. Must be non-NULL with
 *                        valid `issuer`, `client_id`, and at least one
 *                        entry in `allowed_algorithms`. The provider's
 *                        cached discovery doc must already contain a
 *                        `jwks_uri` (Phase 14 will guarantee this; the
 *                        validator itself takes the URI as a separate
 *                        argument so unit tests can inject it).
 * @param jwks_uri        Absolute URL of the JWKS endpoint. Must be
 *                        non-NULL and non-empty.
 * @param id_token        The compact JWS string (header.payload.sig).
 *                        Must be non-NULL.
 * @param expected_nonce  Nonce value stored at `/oidc/start` time
 *                        (Phase 7 state record). Must be non-NULL.
 * @param now             Caller-provided "now" timestamp. Tests inject a
 *                        fixed value; production passes `time(NULL)`.
 * @param out_claims      On success, `*out_claims` is set to a
 *                        newly-allocated `OidcRpIdTokenClaims*`.
 *                        On failure, `*out_claims` is set to NULL.
 *                        Must be non-NULL.
 * @return One of `OidcRpIdTokenError`. `OIDC_RP_IDTOKEN_OK` on
 *         successful validation.
 */
OidcRpIdTokenError oidc_rp_validate_id_token(const OIDCRPProviderConfig* provider,
                                             const char* jwks_uri,
                                             const char* id_token,
                                             const char* expected_nonce,
                                             time_t now,
                                             OidcRpIdTokenClaims** out_claims);

/**
 * @brief Default clock-skew tolerance (seconds).
 *
 * Applied symmetrically to `iat`, `exp`, and `nbf`. The plan calls
 * for ±60 s; this is the constant.
 */
#define OIDC_RP_IDTOKEN_DEFAULT_SKEW_SECONDS 60

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly.
// ---------------------------------------------------------------------------
void scrub_and_free(char* p);
void scrub_bytes_and_free(unsigned char* p, size_t n);
bool split_jws(const char* id_token, char** out_buf, char** header_b64, char** payload_b64, char** sig_b64, size_t* signing_input_len);
OidcRpIdTokenError parse_header(const char* header_b64, char** out_alg, char** out_kid);
bool alg_is_allowed(const OIDCRPProviderConfig* provider, const char* alg);
OidcRpIdTokenError verify_signature(const OIDCRPProviderConfig* provider, const char* jwks_uri, const char* kid, const unsigned char* signing_input, size_t signing_input_len, const unsigned char* signature, size_t sig_len);
bool copy_string_field(json_t* root, const char* key, char** out);
bool aud_contains(json_t* root, const char* expected, char** out_match);
void copy_realm_roles(json_t* root, OidcRpIdTokenClaims* claims);
OidcRpIdTokenError parse_payload_and_check(const char* payload_b64, const OIDCRPProviderConfig* provider, const char* expected_nonce, time_t now, OidcRpIdTokenClaims** out_claims);

#endif // OIDC_RP_IDTOKEN_H
