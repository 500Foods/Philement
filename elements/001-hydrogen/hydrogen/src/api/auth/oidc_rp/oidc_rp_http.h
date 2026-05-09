/**
 * @file oidc_rp_http.h
 * @brief OIDC Relying Party — narrow outbound HTTP GET/POST helpers.
 *
 * Phase 9 of the OIDC plan (`docs/OIDC-PLAN.md`) introduced the GET
 * wrapper for discovery + JWKS. Phase 11 added a companion POST
 * wrapper for the token-endpoint exchange. Both share the same test
 * seam, body cap, and TLS discipline.
 *
 * The wrappers exist so that all outbound HTTP from the OIDC RP code
 * has a single, testable point of contact with libcurl.
 *
 * Design notes:
 *
 *   - Strict TLS verification is the caller's choice via the
 *     `verify_ssl` parameter. Production code MUST pass `true`.
 *   - The body is returned as a heap-allocated NUL-terminated buffer,
 *     even on non-2xx responses (so callers can log the IdP error
 *     payload). Caller owns the buffer.
 *   - Redirect-following is disabled. The two URLs we fetch in
 *     Phase 9 (discovery doc, JWKS) are stable and a redirect would
 *     indicate misconfiguration.
 *   - A test-only injection seam (`oidc_rp_http_test_set_response`)
 *     lets Unity tests substitute a canned response keyed by URL
 *     substring, avoiding the need to mock libcurl symbols. The
 *     seam compiles in release builds too — it has no effect unless
 *     a fixture is registered, which production code never does.
 *
 * Threading: every public function is reentrant. The test fixture
 * registry is guarded by an internal mutex so concurrent unit tests
 * (which Unity does not actually run, but which is cheap insurance)
 * cannot corrupt it.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_HTTP_H
#define OIDC_RP_HTTP_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Result of a synchronous OIDC RP HTTP GET.
 *
 * `body` is heap-allocated on success or on an HTTP failure where
 * the server still sent a payload. On total network failure (no
 * response received) `body` is NULL and `http_status` is 0.
 *
 * `http_status` is 0 when a transport-level failure occurred (e.g.
 * connect timeout, TLS handshake failure). The `error_message`
 * carries the libcurl text for diagnostics; logging policy in the
 * RP code keeps it at DEBUG/ALERT and never logs URL query strings.
 */
typedef struct OidcRpHttpResponse {
    long  http_status;       // 0 on transport failure, else 100..599
    char *body;              // NUL-terminated; may be NULL
    size_t body_size;        // strlen-equivalent; 0 when body is NULL
    char *error_message;     // Owned. NULL on success.
} OidcRpHttpResponse;

/**
 * @brief Free an `OidcRpHttpResponse` and zero out its fields.
 *
 * NULL-safe.
 */
void oidc_rp_http_response_free(OidcRpHttpResponse *response);

/**
 * @brief Synchronous HTTP GET.
 *
 * Returns a heap-allocated `OidcRpHttpResponse*`. Caller owns the
 * struct and must release it with `oidc_rp_http_response_free`.
 *
 * Constraints:
 *   - URL scheme MUST be http or https.
 *   - Redirects are NOT followed.
 *   - Connect timeout is 10 seconds; total timeout 30 seconds.
 *   - Response body capped at 1 MiB. Larger bodies are truncated and
 *     a non-2xx-equivalent failure is reported. A discovery doc or
 *     JWKS far above this size is itself a signal of malformed IdP.
 *
 * @param url        Absolute http(s) URL. Must be non-NULL.
 * @param verify_ssl When true, libcurl verifies peer + host. Production
 *                   MUST pass true. Test/dev may pass false.
 * @param accept     Optional `Accept` header (e.g. `"application/json"`).
 *                   Pass NULL to omit.
 * @return Newly-allocated response; never NULL on caller mistakes
 *         (returns a struct with `error_message` set instead). Returns
 *         NULL only on calloc failure for the struct itself.
 */
OidcRpHttpResponse *oidc_rp_http_get(const char *url,
                                     bool verify_ssl,
                                     const char *accept);

/**
 * @brief Synchronous HTTP POST.
 *
 * Phase 11 companion to `oidc_rp_http_get`. Same constraints (TLS
 * verify mandatory in production, no redirect-following, 1 MiB body
 * cap, 30 s total timeout / 10 s connect timeout) and the same test
 * seam.
 *
 * Used today by the token-endpoint client (`oidc_rp_token.c`) to
 * exchange an authorization code for tokens.
 *
 * The body is sent as-is in the request body. The Content-Type
 * header is set from `content_type`. If `authorization` is non-NULL,
 * an `Authorization: <authorization>` header is added — this is how
 * we send `Authorization: Basic …` for the `client_secret_basic`
 * token-endpoint auth method. Caller is responsible for the entire
 * value (e.g. `"Basic dXNlcjpwYXNz"`); the helper does not prepend
 * `Basic ` itself.
 *
 * Constraints (mirrors GET):
 *   - URL scheme MUST be http or https.
 *   - Redirects are NOT followed (a 302 from a token endpoint would
 *     indicate misconfiguration; following it could leak credentials).
 *   - Connect timeout 10 s, total 30 s.
 *   - Response body capped at 1 MiB.
 *
 * @param url            Absolute http(s) URL. Must be non-NULL.
 * @param verify_ssl     When true, libcurl verifies peer + host.
 *                       Production MUST pass true.
 * @param body           Request body bytes (NUL-terminated). Pass
 *                       NULL or empty for an empty-bodied POST. The
 *                       helper does not free the buffer.
 * @param content_type   Value for the `Content-Type` request header
 *                       (e.g. `"application/x-www-form-urlencoded"`).
 *                       Pass NULL to omit.
 * @param accept         Optional `Accept` header (e.g.
 *                       `"application/json"`). Pass NULL to omit.
 * @param authorization  Optional `Authorization` header value. Caller
 *                       owns formatting (e.g. `"Basic dXNlcjpwYXNz"`).
 *                       Pass NULL to omit.
 * @return Newly-allocated response; never NULL on caller mistakes
 *         (returns a struct with `error_message` set). Returns NULL
 *         only on calloc failure for the struct itself.
 */
OidcRpHttpResponse *oidc_rp_http_post(const char *url,
                                      bool verify_ssl,
                                      const char *body,
                                      const char *content_type,
                                      const char *accept,
                                      const char *authorization);

/**
 * @brief Test-only: register a canned response for any URL whose
 *        absolute form contains `url_substring`.
 *
 * Fixtures are queued FIFO (capacity 8 entries). Each call to
 * `oidc_rp_http_get` / `_post` walks the queue head-to-tail and
 * removes the first fixture whose URL substring matches the request
 * URL. This lets a single test inject multiple consecutive responses
 * (e.g. Phase 12 rotation-recovery, where the validator fetches the
 * JWKS endpoint twice in a row).
 *
 * NULL `body` is allowed and yields a NULL-body response. NULL or
 * empty `url_substring` registers a wildcard fixture that matches
 * any URL.
 *
 * The Phase 9 unit tests use this to feed `oidc_rp_discovery_get`
 * and `oidc_rp_jwks_get` JSON fixtures without needing a running
 * mock Keycloak.
 *
 * @param url_substring  Substring matched against the request URL.
 *                       NULL or empty registers an unconditional
 *                       fixture (matches any URL).
 * @param http_status    Status to report on the response.
 * @param body           NUL-terminated body, or NULL.
 */
void oidc_rp_http_test_set_response(const char *url_substring,
                                    long http_status,
                                    const char *body);

/**
 * @brief Test-only: drop every queued fixture.
 *
 * Unity tearDown should call this so no leftover fixture leaks into
 * the next test. After this call the queue is empty and the next
 * `oidc_rp_http_get` / `_post` will fall through to the real network
 * path (or return a transport failure if the URL is unreachable).
 */
void oidc_rp_http_test_clear_responses(void);

#endif // OIDC_RP_HTTP_H
