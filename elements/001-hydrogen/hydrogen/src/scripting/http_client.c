/*
 * Scripting Subsystem - HTTP client (Phase 16)
 *
 * Thin wrapper over the OIDC RP HTTP helpers. See http_client.h for
 * the design.
 *
 * Phase 17 added a test injection seam: when a fixture is set via
 * scripting_http_test_set_response, the next call whose URL contains
 * the configured substring returns a canned response without
 * touching the network. This lets the HTTP pool + condvar path be
 * tested end-to-end without a real HTTP server.
 */

#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/config/config.h>

#include "http_client.h"

#include "src/api/auth/oidc_rp/oidc_rp_http.h"  // OidcRpHttpResponse, free

#include <curl/curl.h>  // curl_slist_free_all
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/*
 * Phase 17: test injection seam. A small FIFO queue of canned
 * responses; the next scripting_http_get/_post call whose URL
 * contains a queued substring consumes the oldest matching entry
 * and returns a canned OidcRpHttpResponse. The production path is
 * unchanged when the queue is empty.
 */
#define SCRIPTING_HTTP_TEST_QUEUE_CAP 16

typedef struct HttpTestFixture {
    char* url_substring;
    long  http_status;
    char* body;
    bool  consumed;
} HttpTestFixture;

static HttpTestFixture g_test_queue[SCRIPTING_HTTP_TEST_QUEUE_CAP];
static size_t g_test_queue_count = 0;
static int    g_test_consumed_count = 0;
static pthread_mutex_t g_test_lock = PTHREAD_MUTEX_INITIALIZER;

void scripting_http_test_set_response(const char *url_substring,
                                       long http_status,
                                       const char *body) {
    pthread_mutex_lock(&g_test_lock);
    if (g_test_queue_count >= SCRIPTING_HTTP_TEST_QUEUE_CAP) {
        // Drop the oldest entry. Tests that hit this case probably
        // have a leak in tearDown; it is louder to drop than to
        // silently fail.
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
    g_test_queue[g_test_queue_count].consumed = false;
    g_test_queue_count++;
    pthread_mutex_unlock(&g_test_lock);
}

void scripting_http_test_clear_responses(void) {
    pthread_mutex_lock(&g_test_lock);
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        free(g_test_queue[i].url_substring);
        free(g_test_queue[i].body);
    }
    memset(g_test_queue, 0, sizeof(g_test_queue));
    g_test_queue_count = 0;
    g_test_consumed_count = 0;
    pthread_mutex_unlock(&g_test_lock);
}

int scripting_http_test_get_consumed_count(void) {
    pthread_mutex_lock(&g_test_lock);
    int n = g_test_consumed_count;
    pthread_mutex_unlock(&g_test_lock);
    return n;
}

/*
 * Check the test queue for a fixture matching `url`. If found,
 * consume it (mark consumed, increment counter) and return a
 * heap-allocated OidcRpHttpResponse with the canned values. The
 * caller (scripting_http_get/_post) is responsible for freeing
 * the response via oidc_rp_http_response_free. Returns NULL if
 * no fixture matches.
 */
struct OidcRpHttpResponse* try_test_injection(const char* url) {
    if (!url) {
        return NULL;
    }
    pthread_mutex_lock(&g_test_lock);
    struct OidcRpHttpResponse* resp = NULL;
    for (size_t i = 0; i < g_test_queue_count; ++i) {
        if (g_test_queue[i].consumed) continue;
        if (!g_test_queue[i].url_substring) continue;
        if (strstr(url, g_test_queue[i].url_substring) != NULL) {
            resp = calloc(1, sizeof(*resp));
            if (resp) {
                resp->http_status = g_test_queue[i].http_status;
                if (g_test_queue[i].body) {
                    resp->body = strdup(g_test_queue[i].body);
                }
            }
            g_test_queue[i].consumed = true;
            g_test_consumed_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_test_lock);
    return resp;
}

// Resolve the effective timeout: caller-supplied positive value or
// config->scripting.DefaultHTTPTimeout. A non-positive caller value
// is a "use the default" signal (mirroring the OIDC helper's
// `0 => default` convention).
long resolve_timeout(int requested) {
    if (requested > 0) {
        return (long)requested;
    }
    // The scripting config is always present in app_config because
    // it is part of AppConfig (zeroed by default). When scripting
    // is disabled the field is still 0, in which case we fall back
    // to a hard-coded 30 s so the helper has a meaningful value.
    long cfg = app_config ? app_config->scripting.DefaultHTTPTimeout : 0;
    return cfg > 0 ? cfg : 30L;
}

struct OidcRpHttpResponse *scripting_http_get(
    const char *url,
    struct curl_slist *headers,
    int timeout_seconds,
    bool verify_ssl) {

    if (!url) {
        // Caller mistake: mirror the OIDC helper's "URL is NULL or
        // empty" error so the scripting layer's H.wait returns a
        // consistent message. (Without this guard the OIDC helper
        // would still return an error response, but we want to fail
        // fast and not touch headers ownership here.)
        if (headers) curl_slist_free_all(headers);
        OidcRpHttpResponse *resp = calloc(1, sizeof(*resp));
        if (resp) {
            resp->error_message = strdup("URL is NULL or empty");
        }
        return resp;
    }

    // Phase 17: test injection seam. If a fixture matches the URL,
    // return a canned response without touching the network. The
    // OIDC helper is bypassed entirely. Headers ownership is
    // consumed here (the OIDC helper would have taken it).
    struct OidcRpHttpResponse* test_resp = try_test_injection(url);
    if (test_resp) {
        if (headers) curl_slist_free_all(headers);
        return test_resp;
    }

    return oidc_rp_http_get_with_headers_slist(
        url,
        verify_ssl,
        headers,
        SCRIPTING_HTTP_DEFAULT_MAX_BODY,
        resolve_timeout(timeout_seconds));
}

struct OidcRpHttpResponse *scripting_http_post(
    const char *url,
    const char *body,
    const char *content_type,
    struct curl_slist *headers,
    int timeout_seconds,
    bool verify_ssl) {

    if (!url) {
        if (headers) curl_slist_free_all(headers);
        OidcRpHttpResponse *resp = calloc(1, sizeof(*resp));
        if (resp) {
            resp->error_message = strdup("URL is NULL or empty");
        }
        return resp;
    }

    // Phase 17: test injection seam (same as get above).
    struct OidcRpHttpResponse* test_resp = try_test_injection(url);
    if (test_resp) {
        if (headers) curl_slist_free_all(headers);
        return test_resp;
    }

    return oidc_rp_http_post_with_headers_slist(
        url,
        verify_ssl,
        body,
        content_type,
        headers,
        SCRIPTING_HTTP_DEFAULT_MAX_BODY,
        resolve_timeout(timeout_seconds));
}
