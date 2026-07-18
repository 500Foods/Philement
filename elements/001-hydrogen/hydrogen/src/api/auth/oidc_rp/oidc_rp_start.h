/**
 * @file oidc_rp_start.h
 * @brief OIDC RP authorization start endpoint
 *
 * Phase 10 of `docs/OIDC-PLAN.md`. Initiates an Authorization Code +
 * PKCE flow with the configured OIDC provider:
 *
 *   1. Generates `state`, `nonce`, and a PKCE verifier+challenge.
 *   2. Validates the optional `return_to` query parameter against an
 *      open-redirect allow-list.
 *   3. Resolves the discovery doc (cache hit or fresh fetch) to find
 *      the IdP's `authorization_endpoint`.
 *   4. Persists the state record (state, nonce, verifier, database,
 *      return_to, client_ip) with the configured TTL.
 *   5. 302-redirects the browser to the IdP authorization URL with
 *      `Cache-Control: no-store`.
 *
 * When `oidc_rp.enabled` is `false` (the default), the endpoint
 * responds with `503 {"error":"oidc_disabled"}` and no state is
 * generated.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_START_H
#define OIDC_RP_START_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle GET /api/auth/oidc/start requests.
 *
 * Builds an OIDC Authorization Code + PKCE redirect to the configured
 * provider (currently `Providers[0]`). When `oidc_rp.enabled` is
 * `false`, responds 503 `oidc_disabled`. Non-GET methods get 405. An
 * unsafe `return_to` value gets 400 `invalid_return_to`. Internal
 * failures (RNG, alloc, discovery) yield 500 `oidc_internal_error` or
 * 502 `oidc_discovery_failed`.
 *
 * Query parameters:
 *   - `database`  — optional; persisted into the state record so the
 *                   eventual JWT is minted for the right DB. Falls
 *                   back to `OIDC_RP.Database` config or hardcoded
 *                   default at callback time (Phase 14).
 *   - `return_to` — optional; relative path inside Lithium for the
 *                   post-handoff redirect. Allow-listed (must start
 *                   with `/`, no `//`, no scheme, no backslash, no
 *                   CR/LF).
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
//@ swagger:path /api/auth/oidc/start
//@ swagger:method GET
//@ swagger:operationId getAuthOidcStart
//@ swagger:tags "Auth Service"
//@ swagger:summary Begin OIDC sign-in by redirecting to the IdP
//@ swagger:description Initiates an Authorization Code + PKCE flow with the configured OIDC provider. Returns 302 to the IdP authorization endpoint when enabled, 503 oidc_disabled when the feature is gated off.
//@ swagger:response 302 text/plain Redirect to the IdP authorization endpoint
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_return_to"}}}
//@ swagger:response 405 application/json {"type":"object","properties":{"error":{"type":"string","example":"method_not_allowed"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"oidc_internal_error"}}}
//@ swagger:response 502 application/json {"type":"object","properties":{"error":{"type":"string","example":"oidc_discovery_failed"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"error":{"type":"string","example":"oidc_disabled"}}}
enum MHD_Result handle_get_auth_oidc_start(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly.
// ---------------------------------------------------------------------------
enum MHD_Result send_internal_error(struct MHD_Connection *connection, const char *cause);
void start_truncated_state(const char *state, char out[16]);
enum MHD_Result oidc_rp_start_fail(struct MHD_Connection *connection,
                                   const char *cause,
                                   char *state,
                                   char *nonce,
                                   char *code_verifier,
                                   char *code_challenge,
                                   char *client_ip,
                                   char *auth_endpoint_copy);

#endif // OIDC_RP_START_H
