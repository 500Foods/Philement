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
#include <stdbool.h>
#include <src/config/config_oidc_rp.h>

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

/**
 * @brief Validate a `return_to` query parameter against the open-redirect
 *        allow-list.
 *
 * The `return_to` value is a relative path inside Lithium that the
 * post-handoff redirect will deep-link the user back to. To prevent
 * open-redirect attacks (where an attacker tricks a user into clicking a
 * Hydrogen URL that sends them to `evil.com` after sign-in), the value
 * must satisfy ALL of:
 *
 *   - non-NULL pointer (NULL is treated as "not provided" — see below)
 *   - non-empty
 *   - starts with a single `/`
 *   - second character is NOT `/` (rejects `//evil.com` protocol-relative)
 *   - second character is NOT `\` (rejects `/\\evil.com` and friends)
 *   - does NOT contain `://` (rejects schemes hidden mid-string)
 *   - does NOT contain backslashes (Windows path separators that some
 *     parsers treat as `/`)
 *   - does NOT contain CR or LF (header-injection guard)
 *
 * NULL input is considered "no return_to provided" and returns `true`
 * (the caller must check for NULL before using the value, but that is a
 * different concern from validation). An empty string is rejected.
 *
 * @param return_to The candidate path. May be NULL.
 * @return `true` if `return_to` is NULL or a safe relative path; `false`
 *         if the value is present but unsafe.
 */
bool oidc_rp_safe_return_to(const char *return_to);

/**
 * @brief Build the full Keycloak authorization URL for a `/start` redirect.
 *
 * Constructs `${authorization_endpoint}?response_type=code&client_id=…
 * &redirect_uri=…&scope=…&state=…&nonce=…&code_challenge=…
 * &code_challenge_method=S256` with every value properly URL-encoded.
 *
 * Query parameters are appended in canonical order (matching the order
 * specified above) to make the result trivially testable.
 *
 * The function does not validate that the inputs are well-formed — that
 * is the caller's responsibility. It only handles encoding.
 *
 * @param authorization_endpoint The IdP authorization URL (from the
 *        cached discovery doc). Must be non-NULL.
 * @param client_id              The provider's `client_id`. Required.
 * @param redirect_uri           The Hydrogen callback URL. Required.
 * @param scope                  Space-separated scope list (e.g.
 *        `"openid profile email"`). Required.
 * @param state                  Opaque state token. Required.
 * @param nonce                  Opaque nonce. Required.
 * @param code_challenge         PKCE code challenge (S256). Required.
 * @return Heap-allocated NUL-terminated URL, or NULL on bad inputs or
 *         allocation failure. Caller must `free()`.
 */
char *oidc_rp_build_authorize_url(const char *authorization_endpoint,
                                  const char *client_id,
                                  const char *redirect_uri,
                                  const char *scope,
                                  const char *state,
                                  const char *nonce,
                                  const char *code_challenge);

/**
 * @brief Issue a 302 redirect with `Cache-Control: no-store`.
 *
 * Sends an empty body, the `Location` header set to `location`, and the
 * `Cache-Control: no-store` header so intermediaries and the browser do
 * not retain the URL (which embeds a `state` token visible only to this
 * one transaction).
 *
 * Returns `MHD_YES` on success, `MHD_NO` on response-allocation failure.
 *
 * @param connection The MHD connection to respond on.
 * @param location   The absolute URL to redirect to. Must be non-NULL
 *                   and non-empty.
 */
enum MHD_Result oidc_rp_send_redirect(struct MHD_Connection *connection,
                                      const char *location);

/**
 * @brief Lazily initialize the OIDC RP runtime (state store + discovery
 *        cache) on first real use.
 *
 * Phase 14 of `docs/OIDC-PLAN.md` will move this to a proper startup
 * hook in Hydrogen's launch sequence. Until then, Phase 10 calls this
 * the first time `/oidc/start` is hit with `enabled=true`. The function
 * is idempotent and thread-safe: subsequent calls are no-ops.
 *
 * Uses the first provider's `state_ttl_seconds` for the state store.
 *
 * @return `true` if the runtime is ready to serve requests after this
 *         call; `false` on init failure (in which case the caller must
 *         return a 500 envelope).
 */
bool oidc_rp_runtime_lazy_init(void);

/**
 * @brief Resolve the active provider config.
 *
 * Phase 10 always uses `Providers[0]`. Future phases will dispatch by
 * provider name from the request. Returns NULL when OIDC RP is disabled
 * or no providers are configured.
 *
 * @return Const pointer to the provider, or NULL.
 */
const OIDCRPProviderConfig *oidc_rp_get_active_provider(void);

/**
 * @brief Tear down the OIDC RP runtime, if it was lazily initialized.
 *
 * Phase 14: paired counterpart to `oidc_rp_runtime_lazy_init`. Stops
 * the state-store and handoff-store sweeper threads (joining them
 * via condvar wakeup), tears down the discovery + JWKS caches, and
 * resets the `pthread_once` gate so a subsequent re-init is
 * possible. Idempotent and safe to call when no init ever ran.
 *
 * Without this, the lazily-spawned sweeper threads keep the process
 * alive past the normal Hydrogen shutdown sequence, causing the
 * test harness to time out on `stop_hydrogen`.
 *
 * Phase 21+ may move this to a proper Hydrogen subsystem-shutdown
 * hook; until then, Hydrogen's main shutdown path calls this
 * directly.
 */
void oidc_rp_runtime_shutdown(void);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly.
// ---------------------------------------------------------------------------
bool append_param(char **buf, size_t *len, size_t *cap, const char *prefix, const char *key, const char *value);
void oidc_rp_runtime_init_impl(void);

#endif // OIDC_RP_SERVICE_H
