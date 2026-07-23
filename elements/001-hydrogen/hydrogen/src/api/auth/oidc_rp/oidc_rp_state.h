/**
 * @file oidc_rp_state.h
 * @brief OIDC Relying Party in-memory state store.
 *
 * Phase 7 of the OIDC plan (`docs/H/plans/OIDC-PLAN.md`). This module owns
 * the short-lived `state` records that bind an outgoing
 * `/api/auth/oidc/start` request to its eventual
 * `/api/auth/oidc/callback` arrival. Each record carries the data
 * the callback needs to validate and complete the flow:
 *
 *   - `state`         — opaque server-generated key (URL-safe hex)
 *   - `nonce`         — included in the auth URL, echoed back in the
 *                       Keycloak ID token (`nonce` claim)
 *   - `code_verifier` — PKCE verifier kept server-side; sent to the
 *                       Keycloak `/token` endpoint at code-exchange time
 *   - `database`      — the database the eventual JWT will be minted for
 *   - `return_to`     — relative path inside Lithium to deep-link back
 *                       to after sign-in
 *   - `client_ip`     — IP that initiated `/start`; used for sanity
 *                       checks and audit
 *   - `created_at`    — UNIX timestamp; combined with TTL drives expiry
 *
 * Storage is a chained hash table guarded by a single mutex. A
 * background sweeper thread runs on a condvar-driven schedule to
 * reap expired records; a cheap inline sweep also runs on every
 * `put`. All sensitive fields (nonce, verifier) are scrubbed from
 * memory before being freed.
 *
 * Thread safety: every public function is safe to call from any
 * thread once `oidc_rp_state_init()` has returned. `take` is atomic
 * (lookup + remove under the same lock acquisition).
 *
 * The store has no on-disk persistence by design — these records are
 * minutes old at most and a Hydrogen restart simply invalidates any
 * in-flight OIDC sessions. See `docs/H/plans/OIDC-PLAN.md` lines 318–334.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_STATE_H
#define OIDC_RP_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief A single state record returned by `oidc_rp_state_take`.
 *
 * All string fields are heap-allocated and owned by the record.
 * Use `oidc_rp_state_record_free` to release a record returned by
 * `take`.
 */
typedef struct OidcRpStateRecord {
    char *state;          // Always non-NULL on a returned record
    char *nonce;          // Always non-NULL on a returned record
    char *code_verifier;  // Always non-NULL on a returned record
    char *database;       // May be NULL if caller did not supply one
    char *return_to;      // May be NULL if caller did not supply one
    char *client_ip;      // May be NULL if caller did not supply one
    time_t created_at;    // UNIX seconds
    int    ttl_seconds;   // TTL chosen at put time
} OidcRpStateRecord;

/**
 * @brief Initialize the state store.
 *
 * Allocates the hash table, initializes the mutex and condvar, and
 * (unless suppressed for tests) starts the sweeper thread. Idempotent
 * — calling twice without an intervening shutdown logs a debug
 * message and returns success.
 *
 * @param default_ttl_seconds Default TTL in seconds applied when
 *                            `oidc_rp_state_put` is called with
 *                            `ttl_seconds <= 0`. The plan specifies
 *                            10 minutes for `state` records; the
 *                            handoff store (Phase 13) uses a much
 *                            shorter TTL.
 * @return `true` on success, `false` on allocation failure.
 */
bool oidc_rp_state_init(int default_ttl_seconds);

/**
 * @brief Tear down the state store.
 *
 * Stops the sweeper thread (waking it via the condvar so shutdown
 * is fast — no waiting for the next sweep tick), then frees every
 * remaining record and the hash table. Safe to call when the store
 * was never initialized, in which case it is a no-op.
 */
void oidc_rp_state_shutdown(void);

/**
 * @brief Insert a new state record.
 *
 * Copies every input string into freshly heap-allocated memory; the
 * caller retains ownership of the inputs. Subsequent inserts of the
 * same `state` key (which should never happen in practice with
 * correctly-generated state values) replace the prior record.
 *
 * Also opportunistically sweeps a small bounded number of expired
 * entries before returning, amortizing the sweep cost across normal
 * traffic.
 *
 * @param state         Key. Must be non-NULL and non-empty.
 * @param nonce         Required. Must be non-NULL.
 * @param code_verifier Required. Must be non-NULL.
 * @param database      Optional; may be NULL.
 * @param return_to     Optional; may be NULL.
 * @param client_ip     Optional; may be NULL.
 * @param ttl_seconds   TTL for this record. If `<= 0`, the
 *                      `default_ttl_seconds` from `init` is used.
 * @return `true` on success, `false` on bad inputs, allocation
 *         failure, or if the store has not been initialized.
 */
bool oidc_rp_state_put(const char *state,
                       const char *nonce,
                       const char *code_verifier,
                       const char *database,
                       const char *return_to,
                       const char *client_ip,
                       int ttl_seconds);

/**
 * @brief Atomically look up and remove a state record.
 *
 * If the store has a non-expired entry for `state`, returns a heap
 * record (caller must free with `oidc_rp_state_record_free`) and
 * removes it from the store so it cannot be replayed.
 *
 * Returns NULL if `state` is unknown, the record has expired (in
 * which case it is also removed), or the store is uninitialized.
 *
 * @param state The state key generated by `/start` and echoed back
 *              by Keycloak on `/callback`.
 * @return Heap-allocated record, or NULL.
 */
OidcRpStateRecord *oidc_rp_state_take(const char *state);

/**
 * @brief Free a record returned by `oidc_rp_state_take`.
 *
 * Scrubs the sensitive `nonce` and `code_verifier` fields with
 * `memset` before releasing them, mirroring the discipline applied
 * to other secret-bearing structs in the codebase. NULL-safe.
 *
 * @param record Record to release.
 */
void oidc_rp_state_record_free(OidcRpStateRecord *record);

/**
 * @brief Sweep all expired records right now.
 *
 * Normally callers do not need to invoke this — the sweeper thread
 * and the `put` inline sweep handle it. Exposed for unit tests
 * (which want a deterministic sweep) and for the Phase 14 callback
 * to call defensively before logging "state_invalid" events.
 *
 * @return Number of records removed.
 */
size_t oidc_rp_state_sweep_expired(void);

/**
 * @brief Number of records currently held in the store.
 *
 * Returned value is a snapshot; concurrent puts/takes may have
 * already changed the count by the time the caller reads it.
 * Returns 0 when the store is uninitialized.
 */
size_t oidc_rp_state_size(void);

/**
 * @brief Test-only: disable the background sweeper thread.
 *
 * When called before `oidc_rp_state_init`, the init function will
 * not spawn the sweeper. Tests use this so they can drive
 * `oidc_rp_state_sweep_expired` deterministically without racing a
 * background thread. Calls after init have no effect on the current
 * lifecycle.
 *
 * Production code MUST NOT call this.
 */
void oidc_rp_state_test_disable_sweeper(void);

/**
 * @brief Test-only: re-enable the background sweeper thread.
 *
 * Counterpart to `oidc_rp_state_test_disable_sweeper` for tests
 * that toggle the flag mid-suite.
 */
void oidc_rp_state_test_enable_sweeper(void);

// ---------------------------------------------------------------------------
// Internal helpers — NOT part of the stable public API. Exposed non-static
// so Unity tests can call them directly. `StateEntry` is the store's private
// hash-bucket node type (defined in oidc_rp_state.c); forward-declared here
// as opaque.
// ---------------------------------------------------------------------------
typedef struct StateEntry StateEntry;

uint64_t fnv1a_hash(const char *s);
size_t bucket_for(const char *key);
char *opt_strdup(const char *src);
void state_scrub_free(char *s);
void record_free_fields(OidcRpStateRecord *r);
bool state_entry_expired(const StateEntry *entry, time_t now);
void detach_entry(size_t bucket_idx, StateEntry *prev, StateEntry *entry);
void free_entry(StateEntry *entry);
bool remove_locked(const char *state);
size_t sweep_expired_locked(time_t now);
void inline_sweep_locked(time_t now);
void *sweeper_loop(void *arg);

#endif // OIDC_RP_STATE_H
