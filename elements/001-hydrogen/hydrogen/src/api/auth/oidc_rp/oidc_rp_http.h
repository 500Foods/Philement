/**
 * @file oidc_rp_http.h
 * @brief OIDC Relying Party — narrow outbound HTTP GET/POST helpers.
 *
 * Phase 9 of the OIDC plan (`docs/OIDC-PLAN.md`) introduced the GET
 * wrapper for discovery + JWKS. Phase 11 added a companion POST
 * wrapper for the token-endpoint exchange. Both share the same test
 * seam, body cap, and TLS discipline.
 *
 * Phase 16 of the LUA_PLAN added the `_with_cap_and_timeout`
 * variants and response-header capture so the Lua `H.http` host
 * functions can use the same libcurl plumbing (with a larger body
 * cap and a configurable timeout). The original GET/POST signatures
 * are preserved as thin wrappers that pass the OIDC defaults
 * (1 MiB cap, 30 s timeout), so existing OIDC callers are
 * unaffected.
 *
 * The wrappers exist so that all outbound HTTP from the OIDC RP code
 * (and now the Lua scripting subsystem) has a single, testable point
 * of contact with libcurl.
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
 *   - Response headers are captured (Phase 16) and returned as a
 *     NULL-terminated array of `OidcRpHttpHeader` structs. Caller
 *     frees with `oidc_rp_http_response_free`. On test-fixture
 *     responses, the headers array is NULL (the test seam does not
 *     inject headers; this is a documented limitation).
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
 * @date 2026-05-09 (updated 2026-07-04 for Phase 16)
 */

#ifndef OIDC_RP_HTTP_H
#define OIDC_RP_HTTP_H

#include <stdbool.h>
#include <stddef.h>
#include <curl/curl.h>  // CURL (for the internal helpers below)

// Forward-declare the libcurl slist type so callers (the Lua
// scripting layer) can build a headers list without including
// curl.h themselves. Phase 16.
struct curl_slist;

/**
 * @brief A single response header (name + value), both NUL-terminated.
 *
 * Phase 16: added so `H.http` can return a Lua table of response
 * headers. The pair is captured verbatim from libcurl's header
 * callback (lowercased by libcurl, no whitespace trimming). Multi-
 * value headers (e.g. `Set-Cookie`) appear as multiple entries with
 * the same `name`.
 */
typedef struct OidcRpHttpHeader {
    char *name;              // Owned. Lowercased by libcurl.
    char *value;             // Owned. NUL-terminated, no trailing CRLF.
} OidcRpHttpHeader;

/**
 * @brief Result of a synchronous OIDC RP HTTP GET/POST.
 *
 * `body` is heap-allocated on success or on an HTTP failure where
 * the server still sent a payload. On total network failure (no
 * response received) `body` is NULL and `http_status` is 0.
 *
 * `http_status` is 0 when a transport-level failure occurred (e.g.
 * connect timeout, TLS handshake failure). The `error_message`
 * carries the libcurl text for diagnostics; logging policy in the
 * RP code keeps it at DEBUG/ALERT and never logs URL query strings.
 *
 * `headers` (Phase 16) is a heap-allocated, NULL-terminated array
 * of `OidcRpHttpHeader` structs. `headers_count` is the number of
 * entries (excluding the terminating NULL). May be NULL with
 * `headers_count == 0` when the server sent no headers (rare;
 * libcurl always emits at least the status line) or when a test
 * fixture was consumed (test fixtures do not carry headers).
 */
typedef struct OidcRpHttpResponse {
    long  http_status;       // 0 on transport failure, else 100..599
    char *body;              // NUL-terminated; may be NULL
    size_t body_size;        // strlen-equivalent; 0 when body is NULL
    char *error_message;     // Owned. NULL on success.
    OidcRpHttpHeader *headers;  // Owned NULL-terminated array. NULL when no headers.
    size_t headers_count;    // Excluding terminating NULL.
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
 * @brief Synchronous HTTP GET with caller-specified body cap and timeout.
 *
 * Phase 16 of the LUA_PLAN: the Lua `H.http.get` host function uses
 * this variant with a 16 MiB cap and a Lua-supplied timeout. The
 * Phase 9 `oidc_rp_http_get` is a thin wrapper that calls this with
 * the OIDC defaults (1 MiB, 30 s).
 *
 * Constraints (mirrors `oidc_rp_http_get`):
 *   - URL scheme MUST be http or https.
 *   - Redirects are NOT followed.
 *   - Connect timeout is fixed at 10 s (not configurable here; the
 *     connect-timeout knob is for connect-establishment only and the
 *     OIDC's 10 s is a sensible production default for all callers).
 *   - Response body capped at `max_body_bytes`. Larger bodies are
 *     truncated and a transport-level error is reported.
 *
 * @param url                       Absolute http(s) URL. Must be non-NULL.
 * @param verify_ssl                When true, libcurl verifies peer + host.
 * @param accept                    Optional `Accept` header. NULL to omit.
 * @param max_body_bytes            Maximum response body size in bytes.
 *                                  Pass 0 to use the OIDC default (1 MiB).
 * @param request_timeout_seconds   Total request timeout. Pass 0 to use
 *                                  the OIDC default (30 s).
 * @return Newly-allocated response; never NULL on caller mistakes
 *         (returns a struct with `error_message` set instead). Returns
 *         NULL only on calloc failure for the struct itself.
 */
OidcRpHttpResponse *oidc_rp_http_get_with_cap_and_timeout(
    const char *url,
    bool verify_ssl,
    const char *accept,
    size_t max_body_bytes,
    long request_timeout_seconds);

/**
 * @brief Synchronous HTTP POST with caller-specified body cap and timeout.
 *
 * Phase 16 of the LUA_PLAN: the Lua `H.http.post` host function uses
 * this variant with a 16 MiB cap and a Lua-supplied timeout. The
 * Phase 11 `oidc_rp_http_post` is a thin wrapper that calls this with
 * the OIDC defaults (1 MiB, 30 s).
 *
 * Same constraints as `oidc_rp_http_get_with_cap_and_timeout` and the
 * same `body` / `content_type` / `accept` / `authorization` semantics
 * documented on `oidc_rp_http_post`.
 */
OidcRpHttpResponse *oidc_rp_http_post_with_cap_and_timeout(
    const char *url,
    bool verify_ssl,
    const char *body,
    const char *content_type,
    const char *accept,
    const char *authorization,
    size_t max_body_bytes,
    long request_timeout_seconds);

/**
 * @brief Synchronous HTTP GET with caller-built headers slist and full
 *        cap+timeout control.
 *
 * Phase 16 of the LUA_PLAN: the Lua `H.http.get` host function needs
 * to forward arbitrary headers from a Lua table. The scripting layer
 * converts the Lua table to a `struct curl_slist*` and passes it here.
 * `headers` may be NULL (no extra headers beyond libcurl's default
 * `Accept: star-slash-star` which is always present).
 *
 * Other semantics mirror `oidc_rp_http_get_with_cap_and_timeout`.
 * The function takes ownership of `headers` and frees it on return
 * (the scripting layer can strdup-free its own list after this call).
 */
OidcRpHttpResponse *oidc_rp_http_get_with_headers_slist(
    const char *url,
    bool verify_ssl,
    struct curl_slist *headers,
    size_t max_body_bytes,
    long request_timeout_seconds);

/**
 * @brief Synchronous HTTP POST with caller-built headers slist and
 *        full cap+timeout control.
 *
 * Phase 16 sibling of `oidc_rp_http_get_with_headers_slist`. `headers`
 * is merged with the body/content_type plumbing; if a Content-Type is
 * both in `headers` AND passed as the `content_type` parameter, the
 * `content_type` parameter wins (it is appended after the slist).
 *
 * `body` may be NULL (empty POST). `content_type` may be NULL (no
 * Content-Type header). `headers` may be NULL.
 */
OidcRpHttpResponse *oidc_rp_http_post_with_headers_slist(
    const char *url,
    bool verify_ssl,
    const char *body,
    const char *content_type,
    struct curl_slist *headers,
    size_t max_body_bytes,
    long request_timeout_seconds);

/**
 * @brief Test-only: drop every queued fixture.
 *
 * Unity tearDown should call this so no leftover fixture leaks into
 * the next test. After this call the queue is empty and the next
 * `oidc_rp_http_get` / `_post` will fall through to the real network
 * path (or return a transport failure if the URL is unreachable).
 */
void oidc_rp_http_test_clear_responses(void);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly. `HeaderList` and `ResponseBuffer`
// are the module's private accumulation types (defined in oidc_rp_http.c);
// forward-declared here as opaque.
// ---------------------------------------------------------------------------
typedef struct HeaderList HeaderList;
typedef struct ResponseBuffer ResponseBuffer;

OidcRpHttpResponse *response_alloc(void);
void response_set_error(OidcRpHttpResponse *r, const char *msg);
void response_clear_headers(OidcRpHttpResponse *r);
bool header_list_append(HeaderList *list, const char *name, size_t name_len, const char *value, size_t value_len);
size_t write_callback(const void *contents, size_t size, size_t nmemb, void *userp);
size_t header_callback(const void *contents, size_t size, size_t nmemb, void *userp);
bool url_matches(const char *url, const char *substring);
bool take_fixture(const char *url, long *out_status, char **out_body);
bool preflight_request(OidcRpHttpResponse *resp, const char *url, bool *out_should_proceed);
void apply_common_curl_opts(CURL *curl, bool verify_ssl, long request_timeout_seconds);
void perform_and_finalize(CURL *curl, ResponseBuffer *body, OidcRpHttpResponse *resp, const char *method_for_log);
size_t resolve_max_body(size_t requested);
long resolve_request_timeout(long requested);

#endif // OIDC_RP_HTTP_H
