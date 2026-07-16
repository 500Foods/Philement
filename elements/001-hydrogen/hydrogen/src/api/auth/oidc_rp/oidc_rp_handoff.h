/**
 * @file oidc_rp_handoff.h
 * @brief OIDC RP one-time handoff exchange endpoint
 *
 * Phase 13: real implementation. Atomically consumes a single-use,
 * short-lived handoff code minted by `/api/auth/oidc/callback` and
 * returns the same envelope shape as `/api/auth/login`. The
 * `OIDCRelyingPartyConfig.Enabled` flag still gates the endpoint —
 * disabled deployments get `503 {"error":"oidc_disabled"}`.
 *
 * Wire diagram:
 *
 *   1. Client POSTs `{"handoff": "<32-byte hex>"}`.
 *   2. Handler validates JSON shape, looks up the code in the
 *      handoff store (atomic take), optionally verifies the
 *      requester IP matches the IP that completed `/callback`.
 *   3. On success: 200 with `{token, expires_at, user_id, username,
 *      roles}` — same shape as `POST /api/auth/login` so the SPA
 *      can treat both auth paths uniformly.
 *   4. On any failure (missing/expired code, IP mismatch, malformed
 *      body): 401 with `{"error":"handoff_invalid"}`. Single
 *      envelope so a network observer cannot distinguish replay
 *      from genuine failure.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_HANDOFF_H
#define OIDC_RP_HANDOFF_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/oidc/handoff requests.
 *
 * Disabled deployments (`oidc_rp.enabled = false`, the default)
 * respond with `503 {"error":"oidc_disabled"}`. Non-POST methods are
 * rejected with `405 {"error":"method_not_allowed"}`. The endpoint
 * is in `endpoint_expects_json` so the JSON validation middleware
 * runs first; bodies that fail to parse never reach this handler.
 *
 * On enabled deployments:
 *   - 401 `{"error":"handoff_invalid"}` for missing/expired/replayed
 *     handoff codes, or IP mismatch when `BindHandoffToIp` is set.
 *   - 200 envelope mirrors `/auth/login`'s success response.
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
//@ swagger:description Atomically consumes a single-use, short-lived handoff code minted by `/api/auth/oidc/callback` and returns the same envelope as `/api/auth/login`. Replays return 401 with the same `handoff_invalid` envelope as missing/expired codes.
//@ swagger:request body application/json {"type":"object","required":["handoff"],"properties":{"handoff":{"type":"string","description":"Single-use handoff code received from the SPA query string"}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"token":{"type":"string"},"expires_at":{"type":"integer"},"user_id":{"type":"integer"},"username":{"type":"string"},"roles":{"type":"string"}}}
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

// ---------------------------------------------------------------------------
// Internal helper — NOT part of the stable public API. Exposed non-static
// so Unity tests can call it directly.
// ---------------------------------------------------------------------------
enum MHD_Result send_handoff_invalid(struct MHD_Connection *connection, void **con_cls, const char *reason_for_log);

#endif // OIDC_RP_HANDOFF_H
