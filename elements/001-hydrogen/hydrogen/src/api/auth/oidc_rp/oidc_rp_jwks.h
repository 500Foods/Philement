/**
 * @file oidc_rp_jwks.h
 * @brief OIDC Relying Party — JWKS fetch + cache.
 *
 * Phase 9 of the OIDC plan (`docs/OIDC-PLAN.md`). The JWKS cache
 * holds the IdP's public keys, fetched from the `jwks_uri` advertised
 * in the discovery document. Phase 12 (ID token validation) will be
 * the first real consumer; Phase 9 lands the cache and the
 * lookup-by-`kid` API.
 *
 * Cache rules from the plan (lines 337–344):
 *
 *   - Cached for `JwksCacheSeconds` (default 3600 s) per provider.
 *   - Refreshed on TTL expiry, on cache miss, **and** on signature
 *     verification failure (`oidc_rp_jwks_invalidate`).
 *   - TLS verification mandatory in production.
 *
 * Each JWK is stored verbatim as a JSON object string, plus a small
 * extracted bag (`kid`, `kty`, `alg`, `use`) for filtering. Phase 12
 * will pass the JSON string straight into OpenSSL via the existing
 * Hydrogen JWK utilities; this module deliberately does no
 * cryptography.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_JWKS_H
#define OIDC_RP_JWKS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <jansson.h>

/**
 * @brief A single JWK extracted from the IdP's `keys[]` array.
 *
 * `json_text` is the original JSON object for the key, NUL-terminated,
 * heap-allocated. Phase 12 hands this directly to OpenSSL/`libssl`
 * via the existing JWK helpers in `src/utils/`. The other fields are
 * extracted for convenience (logging, lookup, alg filtering).
 */
typedef struct OidcRpJwk {
    char *kid;       // Key ID — the JWS header's `kid` claim
    char *kty;       // Key type, e.g. "RSA"
    char *alg;       // Algorithm hint, e.g. "RS256". May be NULL.
    char *use;       // Intended use, e.g. "sig". May be NULL.
    char *json_text; // The full JSON object for this key
} OidcRpJwk;

/**
 * @brief Initialize the JWKS cache.
 *
 * Idempotent. Returns false on allocation failure.
 */
bool oidc_rp_jwks_init(void);

/**
 * @brief Tear down the JWKS cache, freeing every cached key set.
 */
void oidc_rp_jwks_shutdown(void);

/**
 * @brief Get the JWK with the given `kid` for a provider.
 *
 * Returns a const pointer into the cache. The pointer is valid until
 * the next mutating call (`oidc_rp_jwks_invalidate`,
 * `oidc_rp_jwks_shutdown`, or a refetch triggered by another caller).
 * Callers should copy the `json_text` they need for verification
 * rather than retaining the pointer across blocking work.
 *
 * On cache miss or TTL expiry, fetches the JWKS from `jwks_uri` and
 * installs it. On a `kid` miss against an *already-cached* set, this
 * function does NOT auto-invalidate; Phase 12's validation flow will
 * call `oidc_rp_jwks_invalidate` then retry once, per the plan's
 * rotation-recovery rule.
 *
 * @param provider_name Stable provider key.
 * @param jwks_uri      Absolute URL of the JWKS endpoint, taken from
 *                      the discovery doc.
 * @param verify_ssl    TLS verify flag.
 * @param ttl_seconds   Cache TTL for the new entry. ≤ 0 → 3600.
 * @param kid           Required. Must be non-NULL and non-empty.
 * @return Const pointer to the matching JWK, or NULL on miss/error.
 */
const OidcRpJwk *oidc_rp_jwks_find(const char *provider_name,
                                   const char *jwks_uri,
                                   bool verify_ssl,
                                   int ttl_seconds,
                                   const char *kid);

/**
 * @brief Drop the cached JWKS for a provider.
 *
 * Forces the next `oidc_rp_jwks_find` to refetch. This is the
 * rotation-recovery primitive: when a token signature fails to verify
 * against the currently-cached key, Phase 12 calls `invalidate`, then
 * `find` again, and only fails the validation if the second attempt
 * still cannot find or verify the key.
 *
 * @param provider_name Provider key. NULL or unknown name is a no-op.
 */
void oidc_rp_jwks_invalidate(const char *provider_name);

/**
 * @brief Number of cached provider key sets.
 *
 * Snapshot value, exposed for unit tests.
 */
size_t oidc_rp_jwks_size(void);

/**
 * @brief Number of keys in the currently-cached set for a provider.
 *
 * Returns 0 if the provider has no cached set or the cache is
 * uninitialized. Snapshot value.
 */
size_t oidc_rp_jwks_key_count(const char *provider_name);

/**
 * @brief Parse a JWKS JSON document into a heap-allocated key array.
 *
 * Exposed for unit tests so they can validate parsing without going
 * through the cache or HTTP. Production code should call
 * `oidc_rp_jwks_find`.
 *
 * On success, returns a heap pointer to an array of `OidcRpJwk`
 * structs and writes the count to `*out_count`. The caller must free
 * the array via `oidc_rp_jwks_keys_free`.
 *
 * Returns NULL on JSON parse error, missing `keys` array, an empty
 * array, or allocation failure.
 *
 * @param json_text NUL-terminated JWKS JSON.
 * @param out_count Receives the number of parsed keys (≥ 1 on success).
 */
OidcRpJwk *oidc_rp_jwks_parse(const char *json_text, size_t *out_count);

/**
 * @brief Free an array returned by `oidc_rp_jwks_parse`.
 *
 * NULL-safe. Frees every owned string and the array itself.
 */
void oidc_rp_jwks_keys_free(OidcRpJwk *keys, size_t count);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly. `JwksEntry` is the cache's private
// slot type (defined in oidc_rp_jwks.c); forward-declared here as opaque.
// ---------------------------------------------------------------------------
typedef struct JwksEntry JwksEntry;

void jwk_clear_fields(OidcRpJwk *k);
void slot_clear_locked(JwksEntry *s);
void copy_optional_string(json_t *obj, const char *key, char **out);
bool copy_required_string(json_t *obj, const char *key, char **out);
JwksEntry *jwks_find_slot_locked(const char *name);
JwksEntry *jwks_find_empty_slot_locked(void);
bool jwks_entry_expired(const JwksEntry *entry, time_t now);
const OidcRpJwk *find_kid_in_slot_locked(const JwksEntry *slot, const char *kid);

#endif // OIDC_RP_JWKS_H
