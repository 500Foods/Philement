/*
 * Scripting Subsystem - HTTP client (Phase 16)
 *
 * Thin wrapper over the OIDC RP HTTP helpers
 * (oidc_rp_http_get_with_headers_slist / _post_with_headers_slist)
 * that gives the Lua scripting subsystem:
 *
 *   - A larger body cap (16 MiB by default, vs OIDC's 1 MiB) so LLM
 *     and Canvas/Report Writer payloads fit.
 *   - A caller-supplied timeout (Lua's opts.timeout, falling back to
 *     config->scripting.DefaultHTTPTimeout).
 *   - The same OidcRpHttpResponse shape, including the Phase 16
 *     response headers array.
 *
 * The OIDC helper is the single point of contact with libcurl. The
 * scripting wrapper exists so the H.http host functions in
 * scripting_api_http.c do not have to depend on the OIDC include
 * path directly, and so the body cap / timeout policy for scripting
 * is visible in one place.
 *
 * The wrappers take a pre-built `struct curl_slist*` of headers
 * (NULL allowed). The Lua layer in scripting_api_http.c is
 * responsible for converting a Lua headers table into a slist using
 * jansson + libcurl. The OIDC helper takes ownership of the slist
 * and frees it on return.
 *
 * Threading: the public functions block the calling thread on the
 * libcurl transfer. The Lua H.wait path calls these from the worker
 * thread (or the Orchestrator's thread), so this is the expected
 * pattern. Phase 17 will move the call onto a background thread so
 * multiple H.http.get calls can fan out.
 *
 * @see docs/H/plans/LUA_PLAN_COMPLETE.md Phase 16
 */

#ifndef HYDROGEN_SCRIPTING_HTTP_CLIENT_H
#define HYDROGEN_SCRIPTING_HTTP_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

// Forward-declare the libcurl slist so callers
// (scripting_api_http.c) can pass headers built from a Lua table.
struct curl_slist;

// Forward-declare the OIDC response so we don't drag the OIDC header
// into every translation unit. The shape is a thin view, not the
// struct definition; callers that need the fields include
// oidc_rp_http.h directly (scripting_api_http.c does).
struct OidcRpHttpResponse;

/*
 * Default body cap used by the scripting H.http host functions when
 * the caller does not pass opts.max_body. 16 MiB matches the plan's
 * LLM/Canvas design budget; tests can pass a smaller value via the
 * underlying OIDC `_with_headers_slist` form.
 */
#define SCRIPTING_HTTP_DEFAULT_MAX_BODY (16 * 1024 * 1024)

/*
 * Synchronous HTTP GET for the scripting subsystem.
 *
 * Always blocks the calling thread (the libcurl transfer is
 * synchronous). Returns a heap-allocated OidcRpHttpResponse*; the
 * caller (H_lua_http_wait_one in scripting_api_http.c) owns the
 * response and frees it with oidc_rp_http_response_free.
 *
 * @param url              Absolute http(s) URL. Must be non-NULL.
 * @param headers          Pre-built libcurl headers slist. May be NULL
 *                         to send no extra headers. Ownership passes
 *                         to the underlying helper, which frees it.
 * @param timeout_seconds  Total request timeout. <=0 means "use
 *                         config->scripting.DefaultHTTPTimeout".
 * @param verify_ssl       true in production; false only for tests.
 * @return Heap-allocated response; never NULL on caller mistakes
 *         (returns a struct with error_message set instead).
 */
struct OidcRpHttpResponse *scripting_http_get(
    const char *url,
    struct curl_slist *headers,
    int timeout_seconds,
    bool verify_ssl);

/*
 * Synchronous HTTP POST for the scripting subsystem.
 *
 * @param url              Absolute http(s) URL. Must be non-NULL.
 * @param body             Request body string. NULL or empty for an
 *                         empty-bodied POST.
 * @param content_type     Content-Type header value (e.g.
 *                         "application/json"). NULL to omit; if NULL
 *                         but a Content-Type is in `headers`, the
 *                         slist value is used. (See
 *                         oidc_rp_http_post_with_headers_slist for
 *                         the precise rule.)
 * @param headers          Pre-built libcurl headers slist. NULL OK.
 *                         Ownership passes to the underlying helper.
 * @param timeout_seconds  Total request timeout. <=0 means "use
 *                         config->scripting.DefaultHTTPTimeout".
 * @param verify_ssl       true in production; false only for tests.
 * @return Heap-allocated response.
 */
struct OidcRpHttpResponse *scripting_http_post(
    const char *url,
    const char *body,
    const char *content_type,
    struct curl_slist *headers,
    int timeout_seconds,
    bool verify_ssl);

/*
 * Phase 17 test injection seam. When the test fixture for `url_substring`
 * is set, the next scripting_http_get (or scripting_http_post) call
 * whose URL contains the substring returns a canned response without
 * touching the network. Tests use this to drive the HTTP pool +
 * condvar path end-to-end without a real HTTP server.
 *
 * The fixture queue is FIFO: a call to set_response adds an entry,
 * a get call consumes the oldest entry whose substring matches.
 * Callers should clear the queue in tearDown.
 *
 * url_substring is strdup'd; body is strdup'd. Either may be NULL
 * (an empty string is treated as "no body"; NULL body is "no body").
 * http_status is the canned HTTP status (e.g. 200, 404, 500).
 */
void scripting_http_test_set_response(const char *url_substring,
                                       long http_status,
                                       const char *body);

/*
 * Phase 17 test injection seam. Clear all pending canned responses.
 * Safe to call when the queue is empty. Tests should call this in
 * tearDown to keep fixtures from leaking between tests.
 */
void scripting_http_test_clear_responses(void);

/*
 * Phase 17 test injection seam. Returns the number of canned
 * responses that have been consumed by scripting_http_get/_post
 * since the last clear. Used to assert the background-thread
 * path actually performs the HTTP call.
 */
int scripting_http_test_get_consumed_count(void);

/*
 * Exposed for Unity tests (NOT part of the stable public API).
 */
struct OidcRpHttpResponse* try_test_injection(const char* url);
long resolve_timeout(int requested);

#endif /* HYDROGEN_SCRIPTING_HTTP_CLIENT_H */
