/*
 * OIDC Relying Party — outbound HTTP GET wrapper (Phase 9)
 *
 * Implementation notes:
 *
 *   - libcurl global init/cleanup happens in src/launch/launch.c and
 *     src/state/state.c respectively. Each call here uses
 *     `curl_easy_init` / `curl_easy_cleanup` per request, the same
 *     pattern as `chat_proxy_send_request` in
 *     `src/api/wschat/helpers/proxy.c`.
 *   - Body is collected through a small write callback into a
 *     dynamically-grown buffer; capped at OIDC_RP_HTTP_MAX_BODY.
 *   - The test seam (`oidc_rp_http_test_set_response`) is a single-
 *     element queue guarded by a mutex. Production code never
 *     populates the queue, so the only cost in production is an
 *     unconditional mutex lock + queue check at the top of
 *     `oidc_rp_http_get`. Negligible.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_http.h"

#include <curl/curl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// 1 MiB cap on response body. Discovery docs and JWKS are typically
// 1–4 KiB; 1 MiB leaves several orders of magnitude of headroom while
// still protecting Hydrogen from a malicious or runaway IdP response.
#define OIDC_RP_HTTP_MAX_BODY (1024 * 1024)

// Initial response buffer (4 KiB). Doubles as needed.
#define OIDC_RP_HTTP_INITIAL_BUFFER 4096

#define OIDC_RP_HTTP_CONNECT_TIMEOUT_SECONDS 10L
#define OIDC_RP_HTTP_REQUEST_TIMEOUT_SECONDS 30L

// Initial header capacity. Most responses have < 32 headers; we
// double on demand.
#define OIDC_RP_HTTP_INITIAL_HEADERS 16

typedef struct ResponseBuffer {
    char *data;
    size_t size;
    size_t capacity;
    size_t max_body;          // Phase 16: configurable cap (was a #define before)
} ResponseBuffer;

// Phase 16: growable array of response headers captured via libcurl's
// header callback. Each `name`/`value` is a strdup'd copy of what
// libcurl handed us (the source buffer is reused by libcurl across
// callbacks, so we must copy). `capacity` is the number of slots in
// the malloc'd array; `count` is the number of valid entries.
typedef struct HeaderList {
    OidcRpHttpHeader *items;
    size_t count;
    size_t capacity;
} HeaderList;

// Test-only fixture FIFO. Each entry is single-use: take_fixture()
// removes the matching head entry. Phase 9 originally shipped this as
// a one-slot queue; Phase 12's rotation-recovery flow needs to inject
// two consecutive JWKS responses, so the queue grew up to a small
// fixed-capacity ring. Production code never enqueues; the only cost
// at runtime is an empty-queue check under the mutex.
#define OIDC_RP_HTTP_TEST_QUEUE_CAP 8

typedef struct TestFixture {
    char *url_substring;   // NULL means "match anything"
    long  http_status;
    char *body;
} TestFixture;

static TestFixture g_test_queue[OIDC_RP_HTTP_TEST_QUEUE_CAP];
static size_t g_test_queue_count = 0;
static pthread_mutex_t g_test_fixture_lock = PTHREAD_MUTEX_INITIALIZER;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

OidcRpHttpResponse *response_alloc(void) {
    OidcRpHttpResponse *r = calloc(1, sizeof(*r));
    return r;
}

void response_set_error(OidcRpHttpResponse *r, const char *msg) {
    if (!r) return;
    free(r->error_message);
    r->error_message = msg ? strdup(msg) : NULL;
}

// Phase 16: free the headers array and its owned strings. NULL-safe
// and idempotent (the response is memset(0) on alloc, so calling
// this on a never-populated response is a no-op).
void response_clear_headers(OidcRpHttpResponse *r) {
    if (!r || !r->headers) return;
    for (size_t i = 0; i < r->headers_count; i++) {
        free(r->headers[i].name);
        free(r->headers[i].value);
    }
    free(r->headers);
    r->headers = NULL;
    r->headers_count = 0;
}

// Phase 16: append a single header to the HeaderList, growing the
// underlying array on demand. Returns true on success, false on
// alloc failure (caller treats that as a non-fatal header-loss —
// the body still completes; the user just sees fewer headers).
bool header_list_append(HeaderList *list, const char *name, size_t name_len,
                               const char *value, size_t value_len) {
    if (list->count + 1 >= list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2
                                        : OIDC_RP_HTTP_INITIAL_HEADERS;
        OidcRpHttpHeader *new_items = realloc(list->items,
                                              new_cap * sizeof(OidcRpHttpHeader));
        if (!new_items) return false;
        list->items = new_items;
        list->capacity = new_cap;
    }
    char *n = malloc(name_len + 1);
    char *v = malloc(value_len + 1);
    if (!n || !v) {
        free(n);
        free(v);
        return false;
    }
    memcpy(n, name, name_len);
    n[name_len] = '\0';
    memcpy(v, value, value_len);
    v[value_len] = '\0';
    list->items[list->count].name = n;
    list->items[list->count].value = v;
    list->count++;
    // Sentinel: an empty (NULL name) entry at the end makes the
    // array safely NULL-terminatable in a single realloc if the
    // caller wants that, but the public surface uses
    // `headers_count` rather than scanning for a sentinel, so we
    // keep things simple and do not maintain a sentinel.
    return true;
}

size_t write_callback(const void *contents, size_t size, size_t nmemb,
                             void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    if (buf->size + realsize >= buf->max_body) {
        // Cap exceeded — abort the transfer.
        return 0;
    }

    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2
                                       : OIDC_RP_HTTP_INITIAL_BUFFER;
        while (new_cap < buf->size + realsize + 1) new_cap *= 2;
        char *new_data = realloc(buf->data, new_cap);
        if (!new_data) return 0;
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';
    return realsize;
}

// Phase 16: libcurl header callback. libcurl invokes this once per
// header line (including the status line) and once for the final
// blank line that ends the header block. We skip the status line
// (it has no `:` separator and isn't a real header) and the trailing
// blank line (zero length). For real headers we split on the first
// `:` and strip leading whitespace from the value.
//
// libcurl's contract: return the number of bytes consumed; returning
// a smaller number aborts the transfer. We always return the full
// `realsize`.
size_t header_callback(const void *contents, size_t size, size_t nmemb,
                              void *userp) {
    size_t realsize = size * nmemb;
    HeaderList *list = (HeaderList *)userp;
    const char *line = (const char *)contents;

    // Skip the status line (e.g. "HTTP/1.1 200 OK") — it has no ':'.
    const char *colon = memchr(line, ':', realsize);
    if (!colon) {
        return realsize;
    }
    size_t name_len = (size_t)(colon - line);
    const char *value = colon + 1;
    size_t value_len = realsize - name_len - 1;  // skip the ':'
    // Strip leading whitespace from the value (RFC 7230 says optional
    // OWS is allowed and servers differ). Stop at the first
    // non-whitespace, or at end.
    while (value_len > 0 && (*value == ' ' || *value == '\t')) {
        value++;
        value_len--;
    }
    // Strip a trailing CR/LF if present (libcurl usually passes the
    // CRLF as part of the line). We don't fail on whitespace; we
    // just trim it.
    while (value_len > 0 &&
           (value[value_len - 1] == '\r' || value[value_len - 1] == '\n' ||
            value[value_len - 1] == ' '  || value[value_len - 1] == '\t')) {
        value_len--;
    }
    if (name_len == 0 || value_len == 0) {
        return realsize;
    }

    // Best-effort append: if it fails, we silently drop this header
    // rather than aborting the transfer. The body is more important
    // than the headers.
    (void)header_list_append(list, line, name_len, value, value_len);
    return realsize;
}

// Returns true if `url` contains `substring` (or substring is NULL/empty).
bool url_matches(const char *url, const char *substring) {
    if (!substring || !*substring) return true;
    if (!url) return false;
    return strstr(url, substring) != NULL;
}

// Drain the head fixture matching `url` out of the FIFO under lock.
// Returns true if a fixture was claimed; the caller then owns the
// body string. Walks the queue head-to-tail; the first match is
// removed and remaining entries shift up. This preserves enqueue
// order for tests that register multiple fixtures.
bool take_fixture(const char *url, long *out_status, char **out_body) {
    bool taken = false;
    pthread_mutex_lock(&g_test_fixture_lock);
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        if (url_matches(url, g_test_queue[i].url_substring)) {
            *out_status = g_test_queue[i].http_status;
            *out_body   = g_test_queue[i].body;  // transfer ownership
            free(g_test_queue[i].url_substring);
            // Shift remaining entries down.
            for (size_t j = i + 1; j < g_test_queue_count; ++j) {
                g_test_queue[j - 1] = g_test_queue[j];
            }
            g_test_queue_count--;
            // Zero the now-vacant tail slot for safety.
            memset(&g_test_queue[g_test_queue_count], 0, sizeof(TestFixture));
            taken = true;
            break;
        }
    }
    pthread_mutex_unlock(&g_test_fixture_lock);
    return taken;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void oidc_rp_http_response_free(OidcRpHttpResponse *response) {
    if (!response) return;
    free(response->body);
    free(response->error_message);
    response_clear_headers(response);
    free(response);
}

// Internal: shared preamble for GET and POST.
//
// Returns true if a test fixture was claimed (caller should return
// `resp` immediately), false if the caller should proceed to the
// network call. Sets `resp->error_message` and leaves `resp->body`
// NULL when the URL is invalid (caller still returns `resp`).
//
// Side effects: sets `*out_should_proceed = false` if the caller
// must abort early (URL invalid OR fixture claimed). When the
// fixture was claimed, the response struct is populated; when the
// URL was invalid, only `error_message` is set.
bool preflight_request(OidcRpHttpResponse *resp,
                              const char *url,
                              bool *out_should_proceed) {
    *out_should_proceed = false;

    if (!url || !*url) {
        response_set_error(resp, "URL is NULL or empty");
        return false;
    }

    // Test seam: drain any registered fixture before touching the network.
    long fx_status = 0;
    char *fx_body = NULL;
    if (take_fixture(url, &fx_status, &fx_body)) {
        resp->http_status = fx_status;
        resp->body = fx_body;
        resp->body_size = fx_body ? strlen(fx_body) : 0;
        return true;  // fixture claimed
    }

    // Reject obvious non-http(s) URLs early. libcurl would also reject
    // them, but doing it here gives a stable error message.
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        response_set_error(resp, "URL scheme must be http or https");
        return false;
    }

    *out_should_proceed = true;
    return false;  // no fixture claimed; proceed
}

// Internal: apply the shared TLS, redirect, and UA options every
// OIDC RP outbound request must use. Timeout is configurable
// (Phase 16) and passed by the caller. Caller has already done
// `curl_easy_init` and set the URL.
void apply_common_curl_opts(CURL *curl,
                                   bool verify_ssl,
                                   long request_timeout_seconds) {
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, OIDC_RP_HTTP_CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, request_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hydrogen-OIDC-RP/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_ssl ? 2L : 0L);
}

// Internal: perform the request and translate the result into `resp`.
// Frees `body.data` when an error occurred; transfers ownership to
// `resp->body` on success.
void perform_and_finalize(CURL *curl,
                                 ResponseBuffer *body,
                                 OidcRpHttpResponse *resp,
                                 const char *method_for_log) {
    CURLcode result = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    resp->http_status = http_code;

    if (result == CURLE_OK) {
        resp->body = body->data;
        resp->body_size = body->size;
        body->data = NULL;  // ownership transferred
    } else {
        // Transport-level error. Keep body NULL so callers can rely on
        // (status==0 OR body==NULL) ⇒ failure. Some libcurl error
        // paths leave a stale response code; force 0 so callers can
        // tell transport-failure from HTTP-failure deterministically.
        free(body->data);
        body->data = NULL;
        resp->http_status = 0;
        response_set_error(resp, curl_easy_strerror(result));
        log_this(SR_AUTH,
                 "OIDC_RP: HTTP %s transport error: %s",
                 LOG_LEVEL_ALERT, 2,
                 method_for_log,
                 curl_easy_strerror(result));
    }
}

// Resolve a caller-supplied `max_body_bytes` / `request_timeout_seconds`
// to the effective value (0 => OIDC default). Done as a small helper
// so the GET/POST thin wrappers stay readable.
size_t resolve_max_body(size_t requested) {
    return requested ? requested : OIDC_RP_HTTP_MAX_BODY;
}

long resolve_request_timeout(long requested) {
    return requested ? requested : OIDC_RP_HTTP_REQUEST_TIMEOUT_SECONDS;
}

OidcRpHttpResponse *oidc_rp_http_get(const char *url,
                                     bool verify_ssl,
                                     const char *accept) {
    return oidc_rp_http_get_with_cap_and_timeout(
        url, verify_ssl, accept, 0, 0);
}

OidcRpHttpResponse *oidc_rp_http_get_with_cap_and_timeout(
    const char *url,
    bool verify_ssl,
    const char *accept,
    size_t max_body_bytes,
    long request_timeout_seconds) {

    size_t max_body = resolve_max_body(max_body_bytes);
    long timeout = resolve_request_timeout(request_timeout_seconds);

    OidcRpHttpResponse *resp = response_alloc();
    if (!resp) return NULL;

    bool proceed = false;
    if (preflight_request(resp, url, &proceed)) {
        return resp;  // fixture claimed (no headers for fixtures)
    }
    if (!proceed) {
        return resp;  // URL invalid; error_message set
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        response_set_error(resp, "curl_easy_init failed");
        return resp;
    }

    ResponseBuffer body = {0};
    body.data = malloc(OIDC_RP_HTTP_INITIAL_BUFFER);
    if (!body.data) {
        curl_easy_cleanup(curl);
        response_set_error(resp, "malloc failed");
        return resp;
    }
    body.data[0] = '\0';
    body.capacity = OIDC_RP_HTTP_INITIAL_BUFFER;
    body.max_body = max_body;

    HeaderList hdrs = {0};

    struct curl_slist *headers = NULL;
    if (accept && *accept) {
        char header[256];
        snprintf(header, sizeof(header), "Accept: %s", accept);
        headers = curl_slist_append(headers, header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&hdrs);
    apply_common_curl_opts(curl, verify_ssl, timeout);

    perform_and_finalize(curl, &body, resp, "GET");

    // Transfer headers ownership to the response (even on transport
    // failure we may have partial headers from a TLS/early-error path,
    // but in practice the header callback won't fire without a successful
    // response, so hdrs.count is usually 0 on error).
    resp->headers = hdrs.items;
    resp->headers_count = hdrs.count;

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return resp;
}

OidcRpHttpResponse *oidc_rp_http_post(const char *url,
                                      bool verify_ssl,
                                      const char *body_in,
                                      const char *content_type,
                                      const char *accept,
                                      const char *authorization) {
    return oidc_rp_http_post_with_cap_and_timeout(
        url, verify_ssl, body_in, content_type, accept, authorization, 0, 0);
}

OidcRpHttpResponse *oidc_rp_http_post_with_cap_and_timeout(
    const char *url,
    bool verify_ssl,
    const char *body_in,
    const char *content_type,
    const char *accept,
    const char *authorization,
    size_t max_body_bytes,
    long request_timeout_seconds) {

    size_t max_body = resolve_max_body(max_body_bytes);
    long timeout = resolve_request_timeout(request_timeout_seconds);

    OidcRpHttpResponse *resp = response_alloc();
    if (!resp) return NULL;

    bool proceed = false;
    if (preflight_request(resp, url, &proceed)) {
        return resp;  // fixture claimed
    }
    if (!proceed) {
        return resp;  // URL invalid; error_message set
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        response_set_error(resp, "curl_easy_init failed");
        return resp;
    }

    ResponseBuffer body = {0};
    body.data = malloc(OIDC_RP_HTTP_INITIAL_BUFFER);
    if (!body.data) {
        curl_easy_cleanup(curl);
        response_set_error(resp, "malloc failed");
        return resp;
    }
    body.data[0] = '\0';
    body.capacity = OIDC_RP_HTTP_INITIAL_BUFFER;
    body.max_body = max_body;

    HeaderList hdrs = {0};

    struct curl_slist *headers = NULL;
    if (content_type && *content_type) {
        char header[256];
        snprintf(header, sizeof(header), "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, header);
    }
    if (accept && *accept) {
        char header[256];
        snprintf(header, sizeof(header), "Accept: %s", accept);
        headers = curl_slist_append(headers, header);
    }
    if (authorization && *authorization) {
        // Caller provides the full value (e.g. "Basic dXNlcjpwYXNz").
        // The header buffer must accommodate base64-encoded
        // `client_id:client_secret` strings up to a few hundred bytes.
        size_t auth_len = strlen(authorization);
        size_t hdr_len = auth_len + 32;  // "Authorization: " + NUL
        char *header = malloc(hdr_len);
        if (header) {
            snprintf(header, hdr_len, "Authorization: %s", authorization);
            headers = curl_slist_append(headers, header);
            free(header);
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // POSTFIELDS without POSTFIELDSIZE makes libcurl call strlen on
    // the buffer; we set POSTFIELDSIZE explicitly so the request body
    // can include NUL bytes safely (defence-in-depth — token-endpoint
    // bodies are url-encoded text, but the helper should be reusable).
    if (body_in) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_in);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body_in));
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }

    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&hdrs);
    apply_common_curl_opts(curl, verify_ssl, timeout);

    perform_and_finalize(curl, &body, resp, "POST");

    resp->headers = hdrs.items;
    resp->headers_count = hdrs.count;

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return resp;
}

OidcRpHttpResponse *oidc_rp_http_get_with_headers_slist(
    const char *url,
    bool verify_ssl,
    struct curl_slist *headers,
    size_t max_body_bytes,
    long request_timeout_seconds) {

    size_t max_body = resolve_max_body(max_body_bytes);
    long timeout = resolve_request_timeout(request_timeout_seconds);

    OidcRpHttpResponse *resp = response_alloc();
    if (!resp) {
        if (headers) curl_slist_free_all(headers);
        return NULL;
    }

    bool proceed = false;
    if (preflight_request(resp, url, &proceed)) {
        if (headers) curl_slist_free_all(headers);
        return resp;  // fixture claimed
    }
    if (!proceed) {
        if (headers) curl_slist_free_all(headers);
        return resp;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        if (headers) curl_slist_free_all(headers);
        response_set_error(resp, "curl_easy_init failed");
        return resp;
    }

    ResponseBuffer body = {0};
    body.data = malloc(OIDC_RP_HTTP_INITIAL_BUFFER);
    if (!body.data) {
        curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
        response_set_error(resp, "malloc failed");
        return resp;
    }
    body.data[0] = '\0';
    body.capacity = OIDC_RP_HTTP_INITIAL_BUFFER;
    body.max_body = max_body;

    HeaderList hdrs = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&hdrs);
    apply_common_curl_opts(curl, verify_ssl, timeout);

    perform_and_finalize(curl, &body, resp, "GET");

    resp->headers = hdrs.items;
    resp->headers_count = hdrs.count;

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return resp;
}

OidcRpHttpResponse *oidc_rp_http_post_with_headers_slist(
    const char *url,
    bool verify_ssl,
    const char *body_in,
    const char *content_type,
    struct curl_slist *headers,
    size_t max_body_bytes,
    long request_timeout_seconds) {

    size_t max_body = resolve_max_body(max_body_bytes);
    long timeout = resolve_request_timeout(request_timeout_seconds);

    OidcRpHttpResponse *resp = response_alloc();
    if (!resp) {
        if (headers) curl_slist_free_all(headers);
        return NULL;
    }

    bool proceed = false;
    if (preflight_request(resp, url, &proceed)) {
        if (headers) curl_slist_free_all(headers);
        return resp;  // fixture claimed
    }
    if (!proceed) {
        if (headers) curl_slist_free_all(headers);
        return resp;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        if (headers) curl_slist_free_all(headers);
        response_set_error(resp, "curl_easy_init failed");
        return resp;
    }

    ResponseBuffer body = {0};
    body.data = malloc(OIDC_RP_HTTP_INITIAL_BUFFER);
    if (!body.data) {
        curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
        response_set_error(resp, "malloc failed");
        return resp;
    }
    body.data[0] = '\0';
    body.capacity = OIDC_RP_HTTP_INITIAL_BUFFER;
    body.max_body = max_body;

    HeaderList hdrs = {0};

    // If the caller provided a content_type and it's NOT already in
    // `headers`, append it. (The headers slist may already include a
    // Content-Type from the Lua table; if so, the slist wins, since
    // the OIDC helper appends the named Content-Type AFTER the slist
    // would clobber it. To honour the convention that the named
    // parameter overrides, we only append when absent.)
    struct curl_slist *headers_owned = headers;
    bool has_content_type_in_slist = false;
    if (headers_owned) {
        for (struct curl_slist *p = headers_owned; p; p = p->next) {
            if (p->data && strncasecmp(p->data, "Content-Type:", 12) == 0) {
                has_content_type_in_slist = true;
                break;
            }
        }
    }
    if (content_type && *content_type && !has_content_type_in_slist) {
        char header[256];
        snprintf(header, sizeof(header), "Content-Type: %s", content_type);
        headers_owned = curl_slist_append(headers_owned, header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    if (body_in) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_in);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body_in));
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }

    if (headers_owned) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_owned);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&hdrs);
    apply_common_curl_opts(curl, verify_ssl, timeout);

    perform_and_finalize(curl, &body, resp, "POST");

    resp->headers = hdrs.items;
    resp->headers_count = hdrs.count;

    if (headers_owned) curl_slist_free_all(headers_owned);
    curl_easy_cleanup(curl);

    return resp;
}

void oidc_rp_http_test_set_response(const char *url_substring,
                                    long http_status,
                                    const char *body) {
    pthread_mutex_lock(&g_test_fixture_lock);
    if (g_test_queue_count >= OIDC_RP_HTTP_TEST_QUEUE_CAP) {
        // Queue full — drop the oldest entry to keep the most recent
        // fixtures, matching test author intent. Tests that hit this
        // case probably have a leak in tearDown; it is louder to drop
        // than to silently fail.
        free(g_test_queue[0].url_substring);
        free(g_test_queue[0].body);
        for (size_t j = 1; j < g_test_queue_count; ++j) {
            g_test_queue[j - 1] = g_test_queue[j];
        }
        g_test_queue_count--;
    }
    g_test_queue[g_test_queue_count].url_substring =
        (url_substring && *url_substring) ? strdup(url_substring) : NULL;
    g_test_queue[g_test_queue_count].http_status = http_status;
    g_test_queue[g_test_queue_count].body = body ? strdup(body) : NULL;
    g_test_queue_count++;
    pthread_mutex_unlock(&g_test_fixture_lock);
}

void oidc_rp_http_test_clear_responses(void) {
    pthread_mutex_lock(&g_test_fixture_lock);
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        free(g_test_queue[i].url_substring);
        free(g_test_queue[i].body);
    }
    memset(g_test_queue, 0, sizeof(g_test_queue));
    g_test_queue_count = 0;
    pthread_mutex_unlock(&g_test_fixture_lock);
}
