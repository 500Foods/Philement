/**
 * @file oidc_rp_callback.h
 * @brief OIDC RP authorization callback endpoint.
 *
 * Phase 14 of the OIDC plan (`docs/H/plans/OIDC-PLAN.md`). The browser
 * arrives here after the user authenticated at the IdP. The handler
 * runs the entire server-side chain that turns the IdP authorization
 * code into a Hydrogen-issued JWT and a one-time handoff record:
 *
 *   1. Method + feature gate (Phase 6 contracts preserved).
 *   2. Pull `code` and `state` query params.
 *   3. Atomically take the matching state record (PKCE verifier,
 *      stored nonce, stored client_ip, etc.) (Phase 7).
 *   4. POST to the IdP token endpoint to exchange the code (Phase 11).
 *   5. Validate the returned id_token's signature, issuer, audience,
 *      nonce, and time claims (Phase 12).
 *   6. Resolve the OIDC identity to a Hydrogen account. Phase 14 uses
 *      the throwaway `oidc_rp_link_stub_resolve` (account_id = 1);
 *      Phase 21 swaps in the real four-strategy linker.
 *   7. `verify_api_key()` + `generate_jwt()` + `store_jwt()` —
 *      identical to the password login flow.
 *   8. Generate a fresh handoff code, `oidc_rp_handoff_store_put`.
 *   9. 302 redirect the browser back to the SPA with
 *      `?oidc=1&handoff=<code>` (and `&return_to=...` when present).
 *
 * Failures along the chain redirect to the SPA with a typed
 * `?oidc_error=<code>` query string (never a JSON body — the user
 * agent here is the browser, not an SPA fetch). The error vocabulary
 * is stable across releases:
 *
 *     state_invalid    state lookup failed (unknown/expired/replay)
 *     idp_error        IdP returned ?error= on the redirect
 *     token_<x>        token exchange failed (per OidcRpTokenError)
 *     id_token_<x>     id_token validation failed (per OidcRpIdTokenError)
 *     no_account       linker returned no account
 *     no_api_key       OIDC_RP.SystemApiKey unset or rejected
 *     server_error     internal allocation/JWT-mint failure
 *
 * Disabled-feature responses still use the canonical Phase 6
 * `503 {"error":"oidc_disabled"}` envelope; non-GET methods get
 * the Phase 6 `405 {"error":"method_not_allowed"}` envelope.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_CALLBACK_H
#define OIDC_RP_CALLBACK_H

#include <src/hydrogen.h>
#include <microhttpd.h>
#include <src/config/config_oidc_rp.h>  // OIDCRPProviderConfig

/**
 * @brief Handle GET /api/auth/oidc/callback requests.
 *
 * See the file-level comment for the full step-by-step flow. The
 * disabled-feature and method-mismatch contracts from Phase 6 are
 * preserved unchanged.
 *
 * @param connection The HTTP connection.
 * @param url The request URL.
 * @param method The HTTP method.
 * @param version The HTTP version.
 * @param upload_data Request body data (unused for this GET endpoint).
 * @param upload_data_size Size of request body.
 * @param con_cls Connection-specific data.
 * @return HTTP response code.
 */
//@ swagger:path /api/auth/oidc/callback
//@ swagger:method GET
//@ swagger:operationId getAuthOidcCallback
//@ swagger:tags "Auth Service"
//@ swagger:summary OIDC authorization-code callback from the IdP
//@ swagger:description Receives `code` and `state` from the IdP, exchanges them, mints a Hydrogen JWT, and 302-redirects back to the SPA with a one-time handoff code. Failures redirect with a typed `?oidc_error=` parameter.
//@ swagger:response 302 text/plain Redirect to the SPA with handoff or oidc_error query string
//@ swagger:response 405 application/json {"type":"object","properties":{"error":{"type":"string","example":"method_not_allowed"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"error":{"type":"string","example":"oidc_disabled"}}}
enum MHD_Result handle_get_auth_oidc_callback(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed as
// non-static (with these declarations) purely so Unity tests can call
// them directly. Do not rely on these outside the OIDC RP module.
// ---------------------------------------------------------------------------
void callback_truncated_state(const char *state, char out[16]);
char *build_spa_error_url(const char *redirect_uri, const char *error_code, const char *return_to);
char *build_spa_success_url(const char *redirect_uri, const char *handoff_code, const char *return_to);
enum MHD_Result redirect_with_error(struct MHD_Connection *connection, const OIDCRPProviderConfig *provider, const char *error_code, const char *return_to);
void callback_scrub_free(char *s);

#endif // OIDC_RP_CALLBACK_H
