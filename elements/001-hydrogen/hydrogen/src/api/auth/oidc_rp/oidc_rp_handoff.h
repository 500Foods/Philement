/**
 * @file oidc_rp_handoff.h
 * @brief OIDC RP one-time handoff exchange endpoint
 *
 * Phase 6 stub: returns `503 {"error":"oidc_disabled"}` whenever the
 * `OIDCRelyingPartyConfig.Enabled` flag is `false` (the default).
 * The real implementation (single-use, IP-bindable handoff store
 * exchange returning a Hydrogen JWT) lands in Phase 13.
 *
 * @author Hydrogen Framework
 * @date 2026-05-08
 */

#ifndef OIDC_RP_HANDOFF_H
#define OIDC_RP_HANDOFF_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/oidc/handoff requests.
 *
 * In Phase 6 this is a stub. When `oidc_rp.enabled` is `false`
 * (default), responds with `503 {"error":"oidc_disabled"}`.
 * Non-POST methods are rejected with `405 {"error":"method_not_allowed"}`.
 *
 * @param connection The HTTP connection.
 * @param url The request URL.
 * @param method The HTTP method.
 * @param version The HTTP version.
 * @param upload_data Request body data.
 * @param upload_data_size Size of request body.
 * @param con_cls Connection-specific data.
 * @return HTTP response code.
 */
//@ swagger:path /api/auth/oidc/handoff
//@ swagger:method POST
//@ swagger:operationId postAuthOidcHandoff
//@ swagger:tags "Auth Service"
//@ swagger:summary Exchange a one-time OIDC handoff code for a Hydrogen JWT
//@ swagger:description Atomically consumes a single-use, short-lived handoff code minted by `/api/auth/oidc/callback` and returns the same envelope as `/api/auth/login`. Phase 6 stub: returns 503 oidc_disabled until Phase 13 wires the handoff store.
//@ swagger:request body application/json {"type":"object","required":["handoff"],"properties":{"handoff":{"type":"string","description":"Single-use handoff code received from the SPA query string"}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"token":{"type":"string"},"expires_at":{"type":"string"},"user_id":{"type":"integer"},"roles":{"type":"array","items":{"type":"string"}}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"handoff_invalid"}}}
//@ swagger:response 405 application/json {"type":"object","properties":{"error":{"type":"string","example":"method_not_allowed"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"error":{"type":"string","example":"oidc_disabled"}}}
enum MHD_Result handle_post_auth_oidc_handoff(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif // OIDC_RP_HANDOFF_H
