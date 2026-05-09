/**
 * @file oidc_rp_service.h
 * @brief OIDC Relying Party shared service helpers
 *
 * Internal helpers shared by the three OIDC RP endpoint handlers
 * (`/api/auth/oidc/start`, `/api/auth/oidc/callback`,
 * `/api/auth/oidc/handoff`). At Phase 6 these are stub endpoints
 * that always return `503 {"error":"oidc_disabled"}` because the
 * feature is gated by `OIDCRelyingPartyConfig.Enabled`, which
 * defaults to `false`.
 *
 * Real OIDC behaviour (state store, PKCE, discovery, JWKS, ID-token
 * validation, account linking, JWT minting) lands in Phases 7–22.
 *
 * @author Hydrogen Framework
 * @date 2026-05-08
 */

#ifndef OIDC_RP_SERVICE_H
#define OIDC_RP_SERVICE_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Reports whether OIDC Relying Party endpoints should serve real
 *        traffic.
 *
 * Returns `true` only when both:
 *   - `app_config` is non-NULL, and
 *   - `app_config->oidc_rp.enabled` is `true`.
 *
 * The OIDC RP endpoints call this as their first action and short-
 * circuit with a 503 envelope when it returns `false`.
 *
 * @return `true` when OIDC RP is enabled, `false` otherwise.
 */
bool oidc_rp_is_enabled(void);

/**
 * @brief Sends the canonical "OIDC RP disabled" response.
 *
 * Builds `{"error":"oidc_disabled"}` and queues it with HTTP status
 * 503 Service Unavailable. Logs an `SR_AUTH` debug entry that
 * records the path and method that hit the disabled feature.
 *
 * @param connection The MHD connection to respond on.
 * @param method     The HTTP method of the incoming request (used
 *                   for the log message only).
 * @param endpoint   Short label of the endpoint hit (e.g. `"start"`,
 *                   `"callback"`, `"handoff"`); used for logging.
 * @return The result of `api_send_json_response()`.
 */
enum MHD_Result oidc_rp_send_disabled_response(struct MHD_Connection *connection,
                                               const char *method,
                                               const char *endpoint);

/**
 * @brief Sends a 405 Method Not Allowed envelope for the OIDC RP
 *        endpoints.
 *
 * Builds `{"error":"method_not_allowed"}` and queues it with HTTP
 * status 405. Used by the stub handlers when, for instance, a `POST`
 * lands on `/api/auth/oidc/start` (which is `GET`-only) or a `GET`
 * lands on `/api/auth/oidc/handoff` (which is `POST`-only).
 *
 * @param connection      The MHD connection to respond on.
 * @param method          The HTTP method that was rejected (used in
 *                        the log message only).
 * @param endpoint        Short label of the endpoint hit (used in
 *                        the log message only).
 * @param expected_method The method the endpoint actually accepts
 *                        (e.g. `"GET"` or `"POST"`).
 * @return The result of `api_send_json_response()`.
 */
enum MHD_Result oidc_rp_send_method_not_allowed(struct MHD_Connection *connection,
                                                const char *method,
                                                const char *endpoint,
                                                const char *expected_method);

#endif // OIDC_RP_SERVICE_H
