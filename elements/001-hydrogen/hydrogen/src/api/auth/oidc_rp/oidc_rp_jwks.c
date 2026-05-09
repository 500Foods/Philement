/*
 * OIDC Relying Party — JWKS fetch + cache (Phase 9)
 *
 * Same shape as `oidc_rp_discovery.c`: a small fixed-size per-provider
 * table guarded by a single mutex, populated lazily on first
 * `oidc_rp_jwks_find` per provider. Each cache entry holds an array
 * of parsed JWKs plus the original full-document body for diagnostics.
 *
 * Phase 12 will call `oidc_rp_jwks_find` with the `kid` from a JWS
 * header. On miss (no key with that kid in the currently-cached set)
 * Phase 12 itself decides whether to invalidate-and-retry; this module
 * does NOT auto-invalidate, because doing so would amplify a malformed
 * IdP into a fetch storm.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_jwks.h"
#include "oidc_rp_http.h"
#include <src/config/config_oidc_rp.h>

#include <jansson.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OIDC_RP_JWKS_DEFAULT_TTL_SECONDS 3600

typedef struct JwksEntry {
    char *provider_name;       // Owned; NULL means slot empty
    OidcRpJwk *keys;            // Heap array
    size_t key_count;
    long  cached_at;            // UNIX seconds
    int   ttl_seconds;
} JwksEntry;

typedef struct JwksCache {
    JwksEntry slots[OIDC_RP_MAX_PROVIDERS];
    pthread_mutex_t lock;
} JwksCache;

static JwksCache *g_cache = NULL;

// ---------------------------------------------------------------------------
// JWK helpers
// ---------------------------------------------------------------------------

static void jwk_clear_fields(OidcRpJwk *k) {
    if (!k) return;
    free(k->kid);       k->kid = NULL;
    free(k->kty);       k->kty = NULL;
    free(k->alg);       k->alg = NULL;
    free(k->use);       k->use = NULL;
    free(k->json_text); k->json_text = NULL;
}

void oidc_rp_jwks_keys_free(OidcRpJwk *keys, size_t count) {
    if (!keys) return;
    for (size_t i = 0; i < count; i++) jwk_clear_fields(&keys[i]);
    free(keys);
}

static void slot_clear_locked(JwksEntry *s) {
    free(s->provider_name);
    s->provider_name = NULL;
    oidc_rp_jwks_keys_free(s->keys, s->key_count);
    s->keys = NULL;
    s->key_count = 0;
    s->cached_at = 0;
    s->ttl_seconds = 0;
}

// Optional string copy; leaves *out NULL on absent or non-string.
static void copy_optional_string(json_t *obj, const char *key, char **out) {
    json_t *v = json_object_get(obj, key);
    if (v && json_is_string(v)) {
        char *dup = strdup(json_string_value(v));
        if (dup) { free(*out); *out = dup; }
    }
}

// Required string copy; returns false if missing or non-string.
static bool copy_required_string(json_t *obj, const char *key, char **out) {
    json_t *v = json_object_get(obj, key);
    if (!v || !json_is_string(v)) return false;
    *out = strdup(json_string_value(v));
    return *out != NULL;
}

// ---------------------------------------------------------------------------
// Parser (exposed for tests)
// ---------------------------------------------------------------------------

OidcRpJwk *oidc_rp_jwks_parse(const char *json_text, size_t *out_count) {
    if (!json_text || !*json_text || !out_count) return NULL;
    *out_count = 0;

    json_error_t json_err;
    json_t *root = json_loads(json_text, 0, &json_err);
    if (!root) {
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS parse failed: %s",
                 LOG_LEVEL_ALERT, 1, json_err.text);
        return NULL;
    }
    if (!json_is_object(root)) {
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS top-level is not an object",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    json_t *keys_arr = json_object_get(root, "keys");
    if (!keys_arr || !json_is_array(keys_arr)) {
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS missing or non-array `keys`",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    size_t n = json_array_size(keys_arr);
    if (n == 0) {
        log_this(SR_AUTH, "OIDC_RP: JWKS `keys` array is empty",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    OidcRpJwk *keys = calloc(n, sizeof(*keys));
    if (!keys) {
        json_decref(root);
        return NULL;
    }

    size_t parsed = 0;
    for (size_t i = 0; i < n; i++) {
        json_t *jwk_obj = json_array_get(keys_arr, i);
        if (!jwk_obj || !json_is_object(jwk_obj)) {
            log_this(SR_AUTH,
                     "OIDC_RP: JWKS key %zu is not an object; skipping",
                     LOG_LEVEL_ALERT, 1, i);
            continue;
        }

        OidcRpJwk *out = &keys[parsed];

        // `kid` is the only thing we *require* — without it, the key
        // cannot be selected by header `kid` lookup.
        if (!copy_required_string(jwk_obj, "kid", &out->kid)) {
            log_this(SR_AUTH,
                     "OIDC_RP: JWKS key %zu missing required `kid`; skipping",
                     LOG_LEVEL_ALERT, 1, i);
            jwk_clear_fields(out);
            continue;
        }

        // `kty` is required by the spec but we only soft-warn so a
        // misbehaving IdP can be diagnosed without hard-failing.
        if (!copy_required_string(jwk_obj, "kty", &out->kty)) {
            log_this(SR_AUTH,
                     "OIDC_RP: JWKS key %s missing `kty`; keeping anyway",
                     LOG_LEVEL_ALERT, 1, out->kid);
            // Continue — out->kty stays NULL; Phase 12 will reject.
        }
        copy_optional_string(jwk_obj, "alg", &out->alg);
        copy_optional_string(jwk_obj, "use", &out->use);

        char *jt = json_dumps(jwk_obj, JSON_COMPACT);
        if (!jt) {
            log_this(SR_AUTH,
                     "OIDC_RP: JWKS key %s json_dumps failed",
                     LOG_LEVEL_ALERT, 1, out->kid);
            jwk_clear_fields(out);
            continue;
        }
        out->json_text = jt;
        parsed++;
    }

    json_decref(root);

    if (parsed == 0) {
        // Every entry was rejected.
        oidc_rp_jwks_keys_free(keys, n);
        return NULL;
    }

    // Trim the array if some entries were rejected.
    if (parsed < n) {
        OidcRpJwk *shrunk = realloc(keys, parsed * sizeof(*keys));
        if (shrunk) keys = shrunk;
    }

    *out_count = parsed;
    return keys;
}

// ---------------------------------------------------------------------------
// Cache primitives (caller holds g_cache->lock)
// ---------------------------------------------------------------------------

static JwksEntry *find_slot_locked(const char *name) {
    if (!g_cache || !name) return NULL;
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        if (g_cache->slots[i].provider_name &&
            strcmp(g_cache->slots[i].provider_name, name) == 0) {
            return &g_cache->slots[i];
        }
    }
    return NULL;
}

static JwksEntry *find_empty_slot_locked(void) {
    if (!g_cache) return NULL;
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        if (!g_cache->slots[i].provider_name) return &g_cache->slots[i];
    }
    return NULL;
}

static bool entry_expired(const JwksEntry *entry, time_t now) {
    if (entry->ttl_seconds <= 0) return false;
    return (now - entry->cached_at) >= entry->ttl_seconds;
}

static const OidcRpJwk *find_kid_in_slot_locked(const JwksEntry *slot,
                                                const char *kid) {
    for (size_t i = 0; i < slot->key_count; i++) {
        if (slot->keys[i].kid && strcmp(slot->keys[i].kid, kid) == 0) {
            return &slot->keys[i];
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool oidc_rp_jwks_init(void) {
    if (g_cache) {
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS cache already initialized; ignoring re-init",
                 LOG_LEVEL_DEBUG, 0);
        return true;
    }
    JwksCache *cache = calloc(1, sizeof(*cache));
    if (!cache) {
        log_this(SR_AUTH, "OIDC_RP: JWKS cache calloc failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (pthread_mutex_init(&cache->lock, NULL) != 0) {
        log_this(SR_AUTH, "OIDC_RP: JWKS cache mutex init failed",
                 LOG_LEVEL_ERROR, 0);
        free(cache);
        return false;
    }
    g_cache = cache;
    log_this(SR_AUTH, "OIDC_RP: JWKS cache initialized",
             LOG_LEVEL_STATE, 0);
    return true;
}

void oidc_rp_jwks_shutdown(void) {
    JwksCache *cache = g_cache;
    if (!cache) return;
    pthread_mutex_lock(&cache->lock);
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        slot_clear_locked(&cache->slots[i]);
    }
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    g_cache = NULL;
    free(cache);
    log_this(SR_AUTH, "OIDC_RP: JWKS cache shut down", LOG_LEVEL_STATE, 0);
}

const OidcRpJwk *oidc_rp_jwks_find(const char *provider_name,
                                   const char *jwks_uri,
                                   bool verify_ssl,
                                   int ttl_seconds,
                                   const char *kid) {
    if (!g_cache || !provider_name || !*provider_name ||
        !jwks_uri || !*jwks_uri || !kid || !*kid) {
        return NULL;
    }

    int effective_ttl = (ttl_seconds > 0)
                            ? ttl_seconds
                            : OIDC_RP_JWKS_DEFAULT_TTL_SECONDS;

    // First pass: cache hit?
    pthread_mutex_lock(&g_cache->lock);
    JwksEntry *slot = find_slot_locked(provider_name);
    if (slot && !entry_expired(slot, time(NULL))) {
        const OidcRpJwk *match = find_kid_in_slot_locked(slot, kid);
        pthread_mutex_unlock(&g_cache->lock);
        return match; // may be NULL; caller decides whether to invalidate+retry
    }
    pthread_mutex_unlock(&g_cache->lock);

    // Cache miss or expired — fetch.
    OidcRpHttpResponse *resp = oidc_rp_http_get(jwks_uri, verify_ssl,
                                                "application/json");
    if (!resp) return NULL;
    if (resp->http_status < 200 || resp->http_status >= 300 || !resp->body) {
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS fetch failed (status=%ld) provider=%s",
                 LOG_LEVEL_ALERT, 2, resp->http_status, provider_name);
        oidc_rp_http_response_free(resp);
        return NULL;
    }

    size_t parsed_count = 0;
    OidcRpJwk *parsed_keys = oidc_rp_jwks_parse(resp->body, &parsed_count);
    oidc_rp_http_response_free(resp);
    if (!parsed_keys || parsed_count == 0) return NULL;

    // Install in the cache.
    pthread_mutex_lock(&g_cache->lock);
    JwksEntry *target = find_slot_locked(provider_name);
    if (!target) target = find_empty_slot_locked();
    if (!target) {
        pthread_mutex_unlock(&g_cache->lock);
        log_this(SR_AUTH,
                 "OIDC_RP: JWKS cache full; cannot install %s",
                 LOG_LEVEL_ALERT, 1, provider_name);
        oidc_rp_jwks_keys_free(parsed_keys, parsed_count);
        return NULL;
    }

    // Replace any prior set in the slot.
    slot_clear_locked(target);
    target->provider_name = strdup(provider_name);
    target->keys = parsed_keys;
    target->key_count = parsed_count;
    target->cached_at = (long)time(NULL);
    target->ttl_seconds = effective_ttl;

    const OidcRpJwk *match = find_kid_in_slot_locked(target, kid);
    pthread_mutex_unlock(&g_cache->lock);

    log_this(SR_AUTH,
             "OIDC_RP: JWKS cached for provider=%s keys=%zu ttl=%ds",
             LOG_LEVEL_STATE, 3,
             provider_name, parsed_count, effective_ttl);
    return match;
}

void oidc_rp_jwks_invalidate(const char *provider_name) {
    if (!g_cache || !provider_name) return;
    pthread_mutex_lock(&g_cache->lock);
    JwksEntry *slot = find_slot_locked(provider_name);
    if (slot) slot_clear_locked(slot);
    pthread_mutex_unlock(&g_cache->lock);
}

size_t oidc_rp_jwks_size(void) {
    if (!g_cache) return 0;
    size_t count = 0;
    pthread_mutex_lock(&g_cache->lock);
    for (size_t i = 0; i < OIDC_RP_MAX_PROVIDERS; i++) {
        if (g_cache->slots[i].provider_name) count++;
    }
    pthread_mutex_unlock(&g_cache->lock);
    return count;
}

size_t oidc_rp_jwks_key_count(const char *provider_name) {
    if (!g_cache || !provider_name) return 0;
    size_t n = 0;
    pthread_mutex_lock(&g_cache->lock);
    JwksEntry *slot = find_slot_locked(provider_name);
    if (slot) n = slot->key_count;
    pthread_mutex_unlock(&g_cache->lock);
    return n;
}
