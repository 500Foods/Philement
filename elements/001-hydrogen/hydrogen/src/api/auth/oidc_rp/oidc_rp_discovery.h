/**
 * @file oidc_rp_discovery.h
 * @brief OIDC Relying Party — discovery document fetch + cache.
 *
 * Phase 9 of the OIDC plan (`docs/OIDC-PLAN.md`). This module owns
 * the per-provider cache of `.well-known/openid-configuration`
 * responses. Phase 10 (`/oidc/start`) will be the first caller; Phase
 * 11 (`/token` exchange) and Phase 12 (ID-token validation) will
 * also pull endpoint URLs from here.
 *
 * The plan calls for:
 *
 *   - The discovery doc cached for `DiscoveryCacheSeconds` (default
 *     3600 s) per provider.
 *   - Refetch on cache miss or TTL expiry.
 *   - TLS verification mandatory in production.
 *   - Discovery and JWKS URLs derived from the issuer + the doc; the
 *     issuer string in the parsed doc must match the configured one.
 *
 * Storage is a small fixed-size per-provider table guarded by a single
 * mutex. Multi-provider support is built in (the plan permits up to
 * `OIDC_RP_MAX_PROVIDERS`); the first entry will always be the
 * 500passwords provider.
 *
 * Thread safety: every public function is safe to call from any
 * thread once `oidc_rp_discovery_init()` has returned.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_DISCOVERY_H
#define OIDC_RP_DISCOVERY_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <jansson.h>

/**
 * @brief Parsed contents of an OIDC `.well-known/openid-configuration`
 *        document, restricted to the fields Hydrogen actually consumes.
 *
 * All string fields are heap-allocated and owned by the document.
 * Optional fields may be NULL when the IdP did not advertise them.
 *
 * The struct is treated as immutable from the caller's perspective:
 * callers receive a `const` pointer from `oidc_rp_discovery_get` and
 * must not free or modify it. The cache owns the lifetime; the
 * pointer remains valid until the next cache refresh for the same
 * provider name or until `oidc_rp_discovery_shutdown`.
 */
typedef struct OidcRpDiscoveryDoc {
    char *issuer;                     // Required by spec
    char *authorization_endpoint;     // Required for the auth-code flow
    char *token_endpoint;             // Required for code → token
    char *userinfo_endpoint;          // Optional — not used by Phase 12
    char *jwks_uri;                   // Required for ID-token validation
    char *end_session_endpoint;       // Optional — used by RP-initiated logout

    // Bookkeeping (not part of the spec doc itself).
    char *fetched_from_url;           // The URL we actually GET'd
    long  cached_at;                  // UNIX seconds; for TTL expiry
    int   ttl_seconds;
} OidcRpDiscoveryDoc;

/**
 * @brief Initialize the discovery cache.
 *
 * Allocates the per-provider table and its mutex. Idempotent — a
 * second init is a no-op. Returns false on allocation failure.
 *
 * The cache itself does not pre-fetch anything; entries are populated
 * lazily on first `oidc_rp_discovery_get`.
 */
bool oidc_rp_discovery_init(void);

/**
 * @brief Tear down the discovery cache, freeing all entries.
 *
 * Safe to call when never initialized (no-op).
 */
void oidc_rp_discovery_shutdown(void);

/**
 * @brief Get the discovery document for a provider, fetching if needed.
 *
 * On hit (cached entry not yet expired), returns a const pointer to
 * the cached document. On miss or expiry, GETs
 * `<issuer>/.well-known/openid-configuration` over HTTPS, validates
 * the response, replaces any stale cache entry, and returns the new
 * document.
 *
 * Returns NULL on any failure: HTTP error, invalid JSON, missing
 * required field, issuer mismatch, allocation failure. Failures are
 * logged at `LOG_LEVEL_ALERT` via `SR_AUTH` with the prefix
 * `OIDC_RP:`. The cache is not poisoned — a failed fetch leaves any
 * prior valid entry untouched if one exists, otherwise the cache
 * has no entry for that provider.
 *
 * @param provider_name  Stable per-provider key (matches the `name`
 *                       field in `OIDCRPProviderConfig`). Must be
 *                       non-NULL and non-empty.
 * @param issuer         Configured issuer URL (e.g.
 *                       `https://www.500passwords.com/realms/philement`).
 *                       Used both to construct the discovery URL and
 *                       to verify the `iss` field of the response.
 * @param verify_ssl     TLS verify flag. Production passes `true`.
 * @param ttl_seconds    Cache TTL for the new entry. Re-applied on
 *                       every refetch. ≤ 0 falls back to 3600.
 * @return Const pointer to the cached document, or NULL on failure.
 */
const OidcRpDiscoveryDoc *oidc_rp_discovery_get(const char *provider_name,
                                                const char *issuer,
                                                bool verify_ssl,
                                                int ttl_seconds);

/**
 * @brief Drop the cached entry for a provider, if any.
 *
 * Forces the next `oidc_rp_discovery_get` call to refetch. Used by
 * tests and (in Phase 12) for the rotation-recovery flow when JWKS
 * verification fails and we want to re-pull both docs.
 *
 * @param provider_name  Provider key. NULL or unknown name is a no-op.
 */
void oidc_rp_discovery_invalidate(const char *provider_name);

/**
 * @brief Number of cached discovery entries currently held.
 *
 * Snapshot value; concurrent gets/invalidates may have changed it
 * by the time the caller reads it. Returns 0 when uninitialized.
 *
 * Exposed primarily for unit tests.
 */
size_t oidc_rp_discovery_size(void);

/**
 * @brief Parse a JSON discovery document string into a heap doc.
 *
 * Exposed for unit tests so they can validate the parser without
 * going through the cache. Production code should call
 * `oidc_rp_discovery_get` instead.
 *
 * Returns NULL on JSON parse error, missing required field, or
 * allocation failure. Caller frees with `oidc_rp_discovery_doc_free`.
 *
 * @param json_text NUL-terminated JSON document.
 * @param expected_issuer If non-NULL, the parsed `issuer` must match
 *                        this value byte-for-byte. Mismatch yields NULL.
 */
OidcRpDiscoveryDoc *oidc_rp_discovery_parse(const char *json_text,
                                            const char *expected_issuer);

/**
 * @brief Free a parsed discovery document.
 *
 * NULL-safe. Callers MUST NOT use this on pointers returned by
 * `oidc_rp_discovery_get` — those are owned by the cache.
 */
void oidc_rp_discovery_doc_free(OidcRpDiscoveryDoc *doc);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly. `DiscoveryEntry` is the cache's
// private slot type (defined in oidc_rp_discovery.c); it is forward-declared
// here as an opaque type for the pointer-taking helpers.
// ---------------------------------------------------------------------------
typedef struct DiscoveryEntry DiscoveryEntry;

void doc_clear_fields(OidcRpDiscoveryDoc *d);
bool build_discovery_url(const char *issuer, char *out, size_t out_size);
bool extract_required_string(json_t *obj, const char *key, char **out);
void extract_optional_string(json_t *obj, const char *key, char **out);
DiscoveryEntry *discovery_find_slot_locked(const char *name);
DiscoveryEntry *discovery_find_empty_slot_locked(void);
bool discovery_entry_expired(const DiscoveryEntry *entry, time_t now);
void replace_slot_locked(DiscoveryEntry *slot, const char *name, OidcRpDiscoveryDoc *new_doc);

#endif // OIDC_RP_DISCOVERY_H
