/**
 * @file oidc_rp_token.h
 * @brief OIDC Relying Party — token-endpoint client.
 *
 * Phase 11 of the OIDC plan (`docs/OIDC-PLAN.md`). This module
 * exchanges an authorization code (received at `/api/auth/oidc/callback`)
 * for OIDC tokens at the IdP's token endpoint. The exchange is an
 * RFC 6749 §4.1.3 Authorization Code Grant with PKCE (RFC 7636) and
 * is server-to-server only — the browser never sees the request or
 * response.
 *
 * The module supports both standard token-endpoint authentication
 * methods:
 *
 *   - `client_secret_basic` (default, recommended): credentials in
 *     the HTTP Basic auth header. URL-encoded per RFC 6749 §2.3.1
 *     before base64.
 *   - `client_secret_post`: credentials passed as form parameters in
 *     the request body. Less common; some IdPs require it.
 *
 * The selection is driven by `OIDCRPProviderConfig::auth_method`,
 * defaulting to `client_secret_basic` when not specified.
 *
 * The returned token bundle exposes only the fields Hydrogen
 * actually consumes downstream (Phase 12 ID-token validation, Phase
 * 14 callback wiring): `id_token`, `access_token`, `expires_in`,
 * `token_type`. The IdP's optional `refresh_token` is intentionally
 * NOT surfaced — Hydrogen does not use Keycloak refresh tokens
 * beyond the initial code exchange (the plan's non-goal #4).
 *
 * Thread safety: every public function is reentrant. There is no
 * shared state.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_TOKEN_H
#define OIDC_RP_TOKEN_H

#include <stdbool.h>
#include <stddef.h>

#include <src/config/config_oidc_rp.h>

/**
 * @brief Result of a successful token-endpoint exchange.
 *
 * All string fields are heap-allocated and owned by the caller.
 * Free with `oidc_rp_token_response_free()`.
 *
 * `expires_in` is the number of seconds the access token is valid;
 * 0 if the IdP did not specify it.
 */
typedef struct OidcRpTokenResponse {
    char *id_token;       // Always required by Hydrogen (PKCE + openid scope)
    char *access_token;   // Required by spec when grant succeeded
    char *token_type;     // Typically "Bearer"
    long  expires_in;     // Seconds; 0 when absent
} OidcRpTokenResponse;

/**
 * @brief Typed error code from a token exchange attempt.
 *
 * Mirrors the OAuth2 / OIDC error vocabulary at the level Hydrogen
 * cares about. The values are stable across releases so callers can
 * map them to user-facing `oidc_error=` codes on the redirect back
 * to Lithium.
 */
typedef enum OidcRpTokenError {
    OIDC_RP_TOKEN_OK = 0,                    // Success
    OIDC_RP_TOKEN_ERR_BAD_INPUT = 1,         // NULL/empty argument
    OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT = 2, // Discovery doc missing token_endpoint
    OIDC_RP_TOKEN_ERR_TRANSPORT = 3,         // libcurl transport failure (status==0)
    OIDC_RP_TOKEN_ERR_INVALID_GRANT = 4,     // Maps RFC 6749 invalid_grant / invalid_request
    OIDC_RP_TOKEN_ERR_INVALID_CLIENT = 5,    // Maps RFC 6749 invalid_client (401)
    OIDC_RP_TOKEN_ERR_SERVER = 6,            // 5xx from IdP
    OIDC_RP_TOKEN_ERR_BAD_RESPONSE = 7,      // 2xx but JSON malformed / missing id_token
    OIDC_RP_TOKEN_ERR_OTHER = 8              // 4xx with unrecognized error code
} OidcRpTokenError;

/**
 * @brief Free an `OidcRpTokenResponse` and its string fields.
 *
 * NULL-safe. Sensitive token strings are scrubbed (zeroed) before
 * the memory is released, so a freed-but-not-yet-overwritten heap
 * block does not leak credentials through allocator reuse.
 */
void oidc_rp_token_response_free(OidcRpTokenResponse *response);

/**
 * @brief Convert a typed error code to a stable, lowercase string.
 *
 * Useful for logging and for mapping into `oidc_error=` redirect
 * parameters (Phase 14). Never returns NULL — unknown values map
 * to `"unknown"`.
 */
const char *oidc_rp_token_error_name(OidcRpTokenError err);

/**
 * @brief Exchange an authorization code for OIDC tokens.
 *
 * Performs RFC 6749 §4.1.3 Authorization Code Grant with PKCE:
 *
 *   POST <token_endpoint>
 *   Content-Type: application/x-www-form-urlencoded
 *   [Authorization: Basic <urlencode(client_id):urlencode(client_secret)>]
 *
 *   grant_type=authorization_code
 *   &code=<code>
 *   &redirect_uri=<redirect_uri>
 *   &code_verifier=<code_verifier>
 *   [&client_id=<client_id>&client_secret=<client_secret>]
 *
 * The Authorization header is sent when
 * `provider->auth_method == OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC`
 * (the default). For `OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST`, the
 * credentials are appended to the body instead.
 *
 * On 2xx the JSON response is parsed; an `id_token` MUST be
 * present (Hydrogen always requests `openid` scope), otherwise
 * `OIDC_RP_TOKEN_ERR_BAD_RESPONSE` is returned. The optional
 * `refresh_token` field is ignored.
 *
 * On 4xx/5xx the response is parsed best-effort to extract
 * `error` per RFC 6749 §5.2 and mapped to a typed
 * `OidcRpTokenError`.
 *
 * Logging policy: this function NEVER logs `code`,
 * `code_verifier`, the access/ID tokens, or the Authorization
 * header. Only the typed error code, HTTP status, and (on bad
 * response) a small JSON envelope excerpt are logged at
 * `LOG_LEVEL_ALERT` via `SR_AUTH`.
 *
 * @param provider       Active provider config. Must be non-NULL with
 *                       a valid `client_id`. `client_secret` is
 *                       required for both auth methods (this is a
 *                       confidential client per the plan).
 * @param token_endpoint Absolute URL of the IdP token endpoint
 *                       (typically pulled from cached discovery).
 *                       Must be non-NULL and non-empty.
 * @param code           Authorization code received at /callback.
 *                       Must be non-NULL and non-empty.
 * @param redirect_uri   The same `redirect_uri` used at /start.
 *                       Must be non-NULL and non-empty.
 * @param code_verifier  PKCE code verifier from the state-store
 *                       record. Must be non-NULL and non-empty.
 * @param out_response   On success, `*out_response` is set to a
 *                       newly-allocated `OidcRpTokenResponse*`. On
 *                       failure, `*out_response` is set to NULL.
 *                       Must be non-NULL.
 * @return `OIDC_RP_TOKEN_OK` on success, or one of the typed error
 *         codes on failure.
 */
OidcRpTokenError oidc_rp_exchange_code(const OIDCRPProviderConfig *provider,
                                       const char *token_endpoint,
                                       const char *code,
                                       const char *redirect_uri,
                                       const char *code_verifier,
                                       OidcRpTokenResponse **out_response);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly.
// ---------------------------------------------------------------------------
void token_scrub_free(char **s);
char *append_form_param(char *buffer, size_t *out_len, size_t *out_cap, const char *key, const char *value);
char *build_token_body(const OIDCRPProviderConfig *provider, const char *code, const char *redirect_uri, const char *code_verifier);
char *build_basic_auth_header(const char *client_id, const char *client_secret);
OidcRpTokenError map_oauth_error_code(const char *error);
OidcRpTokenError parse_error_body(const char *body);
OidcRpTokenResponse *parse_success_body(const char *body);

#endif // OIDC_RP_TOKEN_H
