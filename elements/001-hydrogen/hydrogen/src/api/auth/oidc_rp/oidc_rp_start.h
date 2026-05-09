/**
 * @file oidc_rp_start.h
 * @brief OIDC RP authorization start endpoint
 *
 * Phase 6 stub: returns `503 {"error":"oidc_disabled"}` whenever the
 * `OIDCRelyingPartyConfig.Enabled` flag is `false` (the default).
 * The real implementation (Authorization Code + PKCE redirect to
 * Keycloak) lands in Phase 10.
 *
 * @author Hydrogen Framework
 * @date 2026-05-08
 */

#ifndef OIDC_RP_START_H
#define OIDC_RP_START_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle GET /api/auth/oidc/start requests.
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
//@ swagger:path /api/auth/oidc/start
//@ swagger:method GET
//@ swagger:operationId getAuthOidcStart
//@ swagger:tags "Auth Service"
//@ swagger:summary Begin OIDC sign-in by redirecting to the IdP
//@ swagger:description Initiates an Authorization Code + PKCE flow with the configured OIDC provider. Phase 6 stub: returns 503 oidc_disabled until Phase 10 wires the redirect builder.
//@ swagger:response 302 text/plain Redirect to the IdP authorization endpoint
//@ swagger:response 405 application/json {"type":"object","properties":{"error":{"type":"string","example":"method_not_allowed"}}}
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

#endif // OIDC_RP_START_H
