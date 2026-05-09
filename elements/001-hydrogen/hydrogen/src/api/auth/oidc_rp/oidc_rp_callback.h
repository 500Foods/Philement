/**
 * @file oidc_rp_callback.h
 * @brief OIDC RP authorization callback endpoint
 *
 * Phase 6 stub: returns `503 {"error":"oidc_disabled"}` whenever the
 * `OIDCRelyingPartyConfig.Enabled` flag is `false` (the default).
 * The real implementation (state lookup, code exchange, ID-token
 * validation, account linking, JWT minting, handoff redirect) lands
 * across Phases 7–22, finalised in Phase 14.
 *
 * @author Hydrogen Framework
 * @date 2026-05-08
 */

#ifndef OIDC_RP_CALLBACK_H
#define OIDC_RP_CALLBACK_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle GET /api/auth/oidc/callback requests.
 *
 * In Phase 6 this is a stub. When `oidc_rp.enabled` is `false`
 * (default), responds with `503 {"error":"oidc_disabled"}`.
 * Non-GET methods are rejected with `405 {"error":"method_not_allowed"}`.
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
//@ swagger:description Receives `code` and `state` from the IdP, exchanges them, mints a Hydrogen JWT, and redirects back to the SPA with a one-time handoff. Phase 6 stub: returns 503 oidc_disabled until Phase 14 wires the full chain.
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

#endif // OIDC_RP_CALLBACK_H
