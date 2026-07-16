/**
 * @file oidc_rp_debug_inject.h
 * @brief OIDC RP debug-inject endpoint (NON-RELEASE BUILDS ONLY)
 *
 * Phase 13 of the OIDC plan ships a single-use, IP-bindable handoff
 * exchange at `POST /api/auth/oidc/handoff`. The black-box test
 * (`tests/test_42_oidc_rp.sh`) needs to populate the handoff store
 * without going through the full `/start → IdP → /callback` flow,
 * because Phase 13 lands BEFORE Phase 14 wires `/callback`.
 *
 * The solution is this debug-only sidecar endpoint:
 *
 *   `POST /api/auth/oidc/_inject_handoff`
 *
 * which calls `oidc_rp_handoff_store_put` directly with operator-
 * supplied values, returns the handoff code, and otherwise behaves
 * identically to the rest of the OIDC RP surface (gated by
 * `oidc_rp_is_enabled`).
 *
 * **Compile-time gate.** This entire endpoint — declaration, code,
 * and dispatcher branch — is compiled out when `NDEBUG` is defined.
 * The release build (`hydrogen_release`) sets `-DNDEBUG`; the
 * regular, debug, perf, valgrind, and coverage builds do not. The
 * black-box test (`test_42`) uses the `regular` build. The
 * production binary is the `release` build, which by construction
 * cannot reach this code: the `else if` branch in
 * `src/api/api_service.c` is also `#ifndef NDEBUG`-gated, so the
 * URL would 404 before any handler dispatch.
 *
 * Verifying the gate works in release: `nm hydrogen_release |
 * grep handle_post_auth_oidc_inject_handoff` returns nothing.
 *
 * Wire format:
 *
 *   Request body:
 *     {
 *       "jwt": "<some.jwt.token>",        // required
 *       "account_id": 42,                 // required
 *       "username": "alice",              // optional
 *       "roles": "admin,user",            // optional
 *       "expires_at": 1893456000,         // optional (defaults to 0)
 *       "ttl_seconds": 60                 // optional (defaults to provider TTL)
 *     }
 *
 *   Success (200):
 *     {
 *       "handoff": "<32-byte hex code>",
 *       "client_ip": "192.0.2.7"          // bound IP, for /handoff testing
 *     }
 *
 *   Disabled (503): {"error": "oidc_disabled"}
 *
 *   Bad request (400): {"error": "bad_request"}
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_DEBUG_INJECT_H
#define OIDC_RP_DEBUG_INJECT_H

#ifndef NDEBUG

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/oidc/_inject_handoff requests.
 *
 * Test-only endpoint. Compiled out of release builds via the
 * `#ifndef NDEBUG` gate around this entire header.
 *
 * @param connection The HTTP connection.
 * @param url The request URL.
 * @param method The HTTP method.
 * @param version The HTTP version.
 * @param upload_data Request body data.
 * @param upload_data_size Size of request body.
 * @param con_cls Connection-specific data.
 * @return MHD_Result.
 */
enum MHD_Result handle_post_auth_oidc_debug_inject(
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
enum MHD_Result send_bad_request(struct MHD_Connection *connection, void **con_cls, const char *reason);

#endif // NDEBUG

#endif // OIDC_RP_DEBUG_INJECT_H
