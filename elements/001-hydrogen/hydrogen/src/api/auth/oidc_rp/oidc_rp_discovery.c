/*
 * OIDC Relying Party — discovery document fetch + cache (Phase 9)
 *
 * The cache is a fixed-size array of per-provider entries; lookup
 * is by provider name (string compare). At Phase 9 there is one
 * provider; the cap follows OIDC_RP_MAX_PROVIDERS so we can grow.
 *
 * Concurrency: a single mutex guards the table. Public functions
 * acquire the lock for the duration of their work — the cache is
 * tiny enough that finer-grained locking is not worth the
 * complexity.
 *
 * Lifetimes: `oidc_rp_discovery_get` returns a `const OidcRpDiscoveryDoc*`
 * that points directly into the cache slot. The contract is that the
 * pointer is valid until the next call that mutates the same slot
 * (subsequent get-with-refresh, invalidate, or shutdown). In Phase
 * 10 callers will hold the pointer just long enough to copy the URLs
 * they need into a local buffer; that pattern is safe.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_discovery.h"
#include "oidc_rp_http.h"
#include <src/config/config_oidc_rp.h>  // OIDC_RP_MAX_PROVIDERS

#include <jansson.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OIDC_RP_DISCOVERY_DEFAULT_TTL_SECONDS 3600
#define OIDC_RP_DISCOVERY_PATH "/.well-known/openid-configuration"
#define OIDC_RP_DISCOVERY_MAX_URL 1024

typedef struct DiscoveryEntry {
    char *provider_name;            // Owned; NULL means slot empty
    OidcRpDiscoveryDoc doc;         // Embedded; strings owned by us
} DiscoveryEntry;

typedef struct DiscoveryCache {
    DiscoveryEntry slots[OIDC_RP_MAX_PROVIDERS];
    pthread_mutex_t lock;
} DiscoveryCache;

static DiscoveryCache *g_cache = NULL;

// ---------------------------------------------------------------------------
// Doc field helpers
// ---------------------------------------------------------------------------

static void doc_clear_fields(OidcRpDiscoveryDoc *d) {
    if (!d) return;
    free(d->issuer);                d->issuer = NULL;
    free(d->authorization_endpoint); d->authorization_endpoint = NULL;
    free(d->token_endpoint);        d->token_endpoint = NULL;
    free(d->userinfo_endpoint);     d->userinfo_endpoint = NULL;
    free(d->jwks_uri);              d->jwks_uri = NULL;
    free(d->end_session_endpoint);  d->end_session_endpoint = NULL;
    free(d->fetched_from_url);      d->fetched_from_url = NULL;
    d->cached_at = 0;
    d->ttl_seconds = 0;
}

void oidc_rp_discovery_doc_free(OidcRpDiscoveryDoc *doc) {
    if (!doc) return;
    doc_clear_fields(doc);
    free(doc);
}

// Build the full discovery URL: issuer + (issuer ends with '/' ? "" : "/") +
// ".well-known/openid-configuration" but with the standard form
// "<issuer>/.well-known/openid-configuration" (the path always starts
// with a slash; the spec says append).
static bool build_discovery_url(const char *issuer, char *out, size_t out_size) {
    if (!issuer || !*issuer) return false;
    size_t issuer_len = strlen(issuer);
    bool trailing_slash = issuer[issuer_len - 1] == '/';
    int needed;
    if (trailing_slash) {
        // Strip the trailing slash so we don't get "//.well-known".
        needed = snprintf(out, out_size, "%.*s%s",
                          (int)(issuer_len - 1), issuer,
                          OIDC_RP_DISCOVERY_PATH);
    } else {
        needed = snprintf(out, out_size, "%s%s", issuer, OIDC_RP_DISCOVERY_PATH);
    }
    if (needed < 0 || (size_t)needed >= out_size) return false;
    return true;
}

// Required field: must be non-NULL and a string. Sets *out to a
// heap copy. Caller frees only on the failure path.
static bool extract_required_string(json_t *obj, const char *key, char **out) {
    json_t *v = json_object_get(obj, key);
    if (!v || !json_is_string(v)) {
        log_this(SR_AUTH,
                 "OIDC_RP: discovery doc missing required string field: %s",
                 LOG_LEVEL_ALERT, 1, key);
        return false;
    }
    *out = strdup(json_string_value(v));
    return *out != NULL;
}

// Optional string. Sets *out to a heap copy or leaves it as NULL.
static void extract_optional_string(json_t *obj, const char *key, char **out) {
    json_t *v = json_object_get(obj, key);
    if (v && json_is_string(v)) {
        char *dup = strdup(json_string_value(v));
        if (dup) {
            free(*out);  // in case caller pre-populated
            *out = dup;
        }
    }
}

// ---------------------------------------------------------------------------
// Parser (exposed for unit tests)
// ---------------------------------------------------------------------------

OidcRpDiscoveryDoc *oidc_rp_discovery_parse(const char *json_text,
                                            const char *expected_issuer) {
    if (!json_text || !*json_text) return NULL;

    json_error_t json_err;
    json_t *root = json_loads(json_text, 0, &json_err);
    if (!root) {
        log_this(SR_AUTH,
                 "OIDC_RP: discovery doc parse failed: %s",
                 LOG_LEVEL_ALERT, 1, json_err.text);
        return NULL;
    }
    if (!json_is_object(root)) {
        log_this(SR_AUTH,
                 "OIDC_RP: discovery doc top-level is not an object",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    OidcRpDiscoveryDoc *doc = calloc(1, sizeof(*doc));
    if (!doc) {
        json_decref(root);
        return NULL;
    }

    if (!extract_required_string(root, "issuer", &doc->issuer) ||
        !extract_required_string(root, "authorization_endpoint",
                                 &doc->authorization_endpoint) ||
        !extract_required_string(root, "token_endpoint", &doc->token_endpoint) ||
        !extract_required_string(root, "jwks_uri", &doc->jwks_uri)) {
        oidc_rp_discovery_doc_free(doc);
        json_decref(root);
        return NULL;
    }

    extract_optional_string(root, "userinfo_endpoint", &doc->userinfo_endpoint);
    extract_optional_string(root, "end_session_endpoint",
                            &doc->end_session_endpoint);

    if (expected_issuer && *expected_issuer) {
        if (strcmp(expected_issuer, doc->issuer) != 0) {
            log_this(SR_AUTH,
                     "OIDC_RP: discovery issuer mismatch (expected=%s got=%s)",
                     LOG_LEVEL_ALERT, 2, expected_issuer, doc->issuer);
            oidc_rp_discovery_doc_free(doc);
            json_decref(root);
            return NULL;
        }
    }

    json_decref(root);
    return doc;
}

// ---------------------------------------------------------------------------
// Cache primitives (caller holds g_cache->lock)
// ---------------------------------------------------------------------------

// Find slot by provider name. Returns NULL if missing.
static DiscoveryEntry *find_slot_locked(const char *name) {
    if (!g_cache || !name) return NULL;
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        DiscoveryEntry *s = &g_cache->slots[i];
        if (s->provider_name && strcmp(s->provider_name, name) == 0) {
            return s;
        }
    }
    return NULL;
}

// Find first empty slot. Returns NULL if cache is full.
static DiscoveryEntry *find_empty_slot_locked(void) {
    if (!g_cache) return NULL;
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        if (!g_cache->slots[i].provider_name) return &g_cache->slots[i];
    }
    return NULL;
}

// True iff `entry` has aged past its TTL.
static bool entry_expired(const DiscoveryEntry *entry, time_t now) {
    if (entry->doc.ttl_seconds <= 0) return false;
    return (now - entry->doc.cached_at) >= entry->doc.ttl_seconds;
}

// Replace slot contents with `new_doc` and `name`. Frees prior
// strings if any. Always succeeds (assumes inputs valid).
static void replace_slot_locked(DiscoveryEntry *slot,
                                const char *name,
                                OidcRpDiscoveryDoc *new_doc) {
    free(slot->provider_name);
    doc_clear_fields(&slot->doc);
    slot->provider_name = strdup(name);
    slot->doc = *new_doc;            // shallow move
    free(new_doc);                   // shell only; strings transferred
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool oidc_rp_discovery_init(void) {
    if (g_cache) {
        log_this(SR_AUTH,
                 "OIDC_RP: discovery cache already initialized; ignoring re-init",
                 LOG_LEVEL_DEBUG, 0);
        return true;
    }
    DiscoveryCache *cache = calloc(1, sizeof(*cache));
    if (!cache) {
        log_this(SR_AUTH, "OIDC_RP: discovery cache calloc failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (pthread_mutex_init(&cache->lock, NULL) != 0) {
        log_this(SR_AUTH, "OIDC_RP: discovery cache mutex init failed",
                 LOG_LEVEL_ERROR, 0);
        free(cache);
        return false;
    }
    g_cache = cache;
    log_this(SR_AUTH, "OIDC_RP: discovery cache initialized",
             LOG_LEVEL_STATE, 0);
    return true;
}

void oidc_rp_discovery_shutdown(void) {
    DiscoveryCache *cache = g_cache;
    if (!cache) return;

    pthread_mutex_lock(&cache->lock);
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        free(cache->slots[i].provider_name);
        cache->slots[i].provider_name = NULL;
        doc_clear_fields(&cache->slots[i].doc);
    }
    pthread_mutex_unlock(&cache->lock);

    pthread_mutex_destroy(&cache->lock);
    g_cache = NULL;
    free(cache);
    log_this(SR_AUTH, "OIDC_RP: discovery cache shut down",
             LOG_LEVEL_STATE, 0);
}

const OidcRpDiscoveryDoc *oidc_rp_discovery_get(const char *provider_name,
                                                const char *issuer,
                                                bool verify_ssl,
                                                int ttl_seconds) {
    if (!g_cache || !provider_name || !*provider_name || !issuer || !*issuer) {
        return NULL;
    }

    int effective_ttl = (ttl_seconds > 0)
                            ? ttl_seconds
                            : OIDC_RP_DISCOVERY_DEFAULT_TTL_SECONDS;

    // First pass: check the cache while holding the lock.
    pthread_mutex_lock(&g_cache->lock);
    DiscoveryEntry *slot = find_slot_locked(provider_name);
    if (slot && !entry_expired(slot, time(NULL))) {
        const OidcRpDiscoveryDoc *cached = &slot->doc;
        pthread_mutex_unlock(&g_cache->lock);
        return cached;
    }
    pthread_mutex_unlock(&g_cache->lock);

    // Cache miss or expired — fetch outside the lock so other threads
    // are not blocked while libcurl is blocked.
    char url[OIDC_RP_DISCOVERY_MAX_URL];
    if (!build_discovery_url(issuer, url, sizeof(url))) {
        log_this(SR_AUTH, "OIDC_RP: discovery URL too long for issuer",
                 LOG_LEVEL_ALERT, 0);
        return NULL;
    }

    OidcRpHttpResponse *resp = oidc_rp_http_get(url, verify_ssl,
                                                "application/json");
    if (!resp) {
        return NULL;
    }
    if (resp->http_status < 200 || resp->http_status >= 300 || !resp->body) {
        log_this(SR_AUTH,
                 "OIDC_RP: discovery fetch failed (status=%ld) provider=%s",
                 LOG_LEVEL_ALERT, 2, resp->http_status, provider_name);
        oidc_rp_http_response_free(resp);
        return NULL;
    }

    OidcRpDiscoveryDoc *new_doc = oidc_rp_discovery_parse(resp->body, issuer);
    if (!new_doc) {
        oidc_rp_http_response_free(resp);
        return NULL;
    }
    new_doc->fetched_from_url = strdup(url);
    new_doc->cached_at = (long)time(NULL);
    new_doc->ttl_seconds = effective_ttl;

    oidc_rp_http_response_free(resp);

    // Install in the cache.
    pthread_mutex_lock(&g_cache->lock);
    DiscoveryEntry *target = find_slot_locked(provider_name);
    if (!target) {
        target = find_empty_slot_locked();
        if (!target) {
            pthread_mutex_unlock(&g_cache->lock);
            log_this(SR_AUTH,
                     "OIDC_RP: discovery cache full; cannot install %s",
                     LOG_LEVEL_ALERT, 1, provider_name);
            oidc_rp_discovery_doc_free(new_doc);
            return NULL;
        }
    }
    replace_slot_locked(target, provider_name, new_doc);
    const OidcRpDiscoveryDoc *result = &target->doc;
    pthread_mutex_unlock(&g_cache->lock);

    log_this(SR_AUTH,
             "OIDC_RP: discovery cached for provider=%s ttl=%ds",
             LOG_LEVEL_STATE, 2, provider_name, effective_ttl);
    return result;
}

void oidc_rp_discovery_invalidate(const char *provider_name) {
    if (!g_cache || !provider_name) return;
    pthread_mutex_lock(&g_cache->lock);
    DiscoveryEntry *slot = find_slot_locked(provider_name);
    if (slot) {
        free(slot->provider_name);
        slot->provider_name = NULL;
        doc_clear_fields(&slot->doc);
    }
    pthread_mutex_unlock(&g_cache->lock);
}

size_t oidc_rp_discovery_size(void) {
    if (!g_cache) return 0;
    size_t count = 0;
    pthread_mutex_lock(&g_cache->lock);
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        if (g_cache->slots[i].provider_name) count++;
    }
    pthread_mutex_unlock(&g_cache->lock);
    return count;
}
