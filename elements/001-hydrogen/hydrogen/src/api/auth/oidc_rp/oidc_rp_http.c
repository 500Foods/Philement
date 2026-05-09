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

typedef struct ResponseBuffer {
    char *data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

// Test-only fixture queue (single entry, single-use).
typedef struct TestFixture {
    char *url_substring;   // NULL means "match anything"
    long  http_status;
    char *body;
    bool  pending;
} TestFixture;

static TestFixture g_test_fixture = {NULL, 0, NULL, false};
static pthread_mutex_t g_test_fixture_lock = PTHREAD_MUTEX_INITIALIZER;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static OidcRpHttpResponse *response_alloc(void) {
    OidcRpHttpResponse *r = calloc(1, sizeof(*r));
    return r;
}

static void response_set_error(OidcRpHttpResponse *r, const char *msg) {
    if (!r) return;
    free(r->error_message);
    r->error_message = msg ? strdup(msg) : NULL;
}

static size_t write_callback(const void *contents, size_t size, size_t nmemb,
                             void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    if (buf->size + realsize >= OIDC_RP_HTTP_MAX_BODY) {
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

// Returns true if `url` contains `substring` (or substring is NULL/empty).
static bool url_matches(const char *url, const char *substring) {
    if (!substring || !*substring) return true;
    if (!url) return false;
    return strstr(url, substring) != NULL;
}

// Drain a fixture out of the queue under lock. Returns true if a
// fixture was claimed; the caller then owns the body string.
static bool take_fixture(const char *url, long *out_status, char **out_body) {
    bool taken = false;
    pthread_mutex_lock(&g_test_fixture_lock);
    if (g_test_fixture.pending && url_matches(url, g_test_fixture.url_substring)) {
        *out_status = g_test_fixture.http_status;
        *out_body   = g_test_fixture.body;  // transfer ownership
        g_test_fixture.body = NULL;
        free(g_test_fixture.url_substring);
        g_test_fixture.url_substring = NULL;
        g_test_fixture.pending = false;
        taken = true;
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
static bool preflight_request(OidcRpHttpResponse *resp,
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

// Internal: apply the shared TLS, timeout, redirect, and UA options
// every OIDC RP outbound request must use. Caller has already done
// `curl_easy_init` and set the URL.
static void apply_common_curl_opts(CURL *curl,
                                   bool verify_ssl,
                                   ResponseBuffer *body) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)body);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, OIDC_RP_HTTP_CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, OIDC_RP_HTTP_REQUEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hydrogen-OIDC-RP/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_ssl ? 2L : 0L);
}

// Internal: perform the request and translate the result into `resp`.
// Frees `body.data` when an error occurred; transfers ownership to
// `resp->body` on success.
static void perform_and_finalize(CURL *curl,
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

OidcRpHttpResponse *oidc_rp_http_get(const char *url,
                                     bool verify_ssl,
                                     const char *accept) {
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

    struct curl_slist *headers = NULL;
    if (accept && *accept) {
        char header[256];
        snprintf(header, sizeof(header), "Accept: %s", accept);
        headers = curl_slist_append(headers, header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    apply_common_curl_opts(curl, verify_ssl, &body);

    perform_and_finalize(curl, &body, resp, "GET");

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
    apply_common_curl_opts(curl, verify_ssl, &body);

    perform_and_finalize(curl, &body, resp, "POST");

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return resp;
}

void oidc_rp_http_test_set_response(const char *url_substring,
                                    long http_status,
                                    const char *body) {
    pthread_mutex_lock(&g_test_fixture_lock);
    free(g_test_fixture.url_substring);
    free(g_test_fixture.body);
    g_test_fixture.url_substring = (url_substring && *url_substring)
                                       ? strdup(url_substring)
                                       : NULL;
    g_test_fixture.http_status = http_status;
    g_test_fixture.body = body ? strdup(body) : NULL;
    g_test_fixture.pending = true;
    pthread_mutex_unlock(&g_test_fixture_lock);
}

void oidc_rp_http_test_clear_responses(void) {
    pthread_mutex_lock(&g_test_fixture_lock);
    free(g_test_fixture.url_substring);
    free(g_test_fixture.body);
    g_test_fixture.url_substring = NULL;
    g_test_fixture.body = NULL;
    g_test_fixture.http_status = 0;
    g_test_fixture.pending = false;
    pthread_mutex_unlock(&g_test_fixture_lock);
}
