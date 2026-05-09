/**
 * @file oidc_rp_handoff_store.h
 * @brief OIDC Relying Party in-memory handoff store.
 *
 * Phase 13 of the OIDC plan (`docs/OIDC-PLAN.md`). This module owns
 * the single-use, short-lived handoff records that bridge the
 * `/api/auth/oidc/callback` redirect and the
 * `/api/auth/oidc/handoff` exchange. Each record carries the data
 * the SPA needs to receive in the success envelope:
 *
 *   - `handoff`     — opaque server-generated key (URL-safe hex)
 *   - `jwt`         — Hydrogen-issued JWT minted by the callback
 *                     (Phase 14 wires this); held in plaintext only
 *                     until the SPA collects it via `/handoff`
 *   - `account_id`  — the user the JWT belongs to; surfaces as
 *                     `user_id` in the success response
 *   - `username`    — display name; mirrors `/auth/login`'s response
 *   - `roles`       — comma-joined role string (login uses the same
 *                     shape — `account_info_t::roles`)
 *   - `expires_at`  — JWT expiry as a UNIX timestamp; surfaces as
 *                     `expires_at` in the success response
 *   - `client_ip`   — IP that completed the OIDC dance via
 *                     `/callback`; checked against the requester
 *                     when `BindHandoffToIp = true`
 *   - `created_at`  — UNIX timestamp; combined with TTL drives expiry
 *
 * Storage mirrors the Phase 7 state store: a chained hash table
 * guarded by a single mutex, reaped by a background sweeper thread
 * on a condvar tick (so shutdown wakes it instantly), with an
 * inline bounded sweep on every `put`.
 *
 * The JWT itself is treated as sensitive: the field is scrubbed
 * with `memset` before being freed, defeating over-eager dead-store
 * elimination at -O2. The other strings (username, roles) are not
 * sensitive at this level — they appear in routine logs.
 *
 * Thread safety: every public function is safe to call from any
 * thread once `oidc_rp_handoff_store_init()` has returned. `take`
 * is atomic (lookup + remove under the same lock acquisition) so a
 * concurrent replay attempt loses the race deterministically.
 *
 * The store has no on-disk persistence by design — these records
 * live for at most a minute (default `HandoffTtlSeconds` = 60).
 * A Hydrogen restart simply invalidates any in-flight handoffs;
 * the SPA shows "sign-in didn't complete, please try again". See
 * `docs/OIDC-PLAN.md` lines 318–334 (the same rationale that
 * applies to the state store).
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_HANDOFF_STORE_H
#define OIDC_RP_HANDOFF_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/**
 * @brief A single handoff record returned by `oidc_rp_handoff_store_take`.
 *
 * All string fields are heap-allocated and owned by the record.
 * Use `oidc_rp_handoff_record_free` to release a record returned
 * by `take`. The `jwt` field is scrubbed before free.
 */
typedef struct OidcRpHandoffRecord {
    char *handoff;     // Always non-NULL on a returned record
    char *jwt;         // Always non-NULL on a returned record (sensitive)
    char *username;    // May be NULL if caller did not supply one
    char *roles;       // May be NULL if caller did not supply one
    char *client_ip;   // May be NULL if caller did not supply one
    int   account_id;  // Hydrogen account_id (= user_id in response)
    time_t expires_at; // UNIX seconds; mirrors the JWT's `exp`
    time_t created_at; // UNIX seconds when this record was put
    int    ttl_seconds; // TTL chosen at put time
} OidcRpHandoffRecord;

/**
 * @brief Initialize the handoff store.
 *
 * Allocates the hash table, initializes the mutex and condvar, and
 * (unless suppressed for tests) starts the sweeper thread.
 * Idempotent — calling twice without an intervening shutdown logs a
 * debug message and returns success.
 *
 * @param default_ttl_seconds Default TTL in seconds applied when
 *                            `oidc_rp_handoff_store_put` is called
 *                            with `ttl_seconds <= 0`. The plan
 *                            specifies 60 seconds for handoff
 *                            records (much shorter than state
 *                            records' 10 minutes).
 * @return `true` on success, `false` on allocation failure.
 */
bool oidc_rp_handoff_store_init(int default_ttl_seconds);

/**
 * @brief Tear down the handoff store.
 *
 * Stops the sweeper thread (waking it via the condvar so shutdown
 * is fast — no waiting for the next sweep tick), then frees every
 * remaining record and the hash table. Safe to call when the store
 * was never initialized, in which case it is a no-op.
 */
void oidc_rp_handoff_store_shutdown(void);

/**
 * @brief Insert a new handoff record.
 *
 * Copies every input string into freshly heap-allocated memory; the
 * caller retains ownership of the inputs. Subsequent inserts of the
 * same `handoff` key (which should never happen with random handoff
 * codes) replace the prior record.
 *
 * Also opportunistically sweeps a small bounded number of expired
 * entries before returning, amortizing the sweep cost across normal
 * traffic.
 *
 * @param handoff       Key. Must be non-NULL and non-empty.
 * @param jwt           Required. The Hydrogen-issued JWT for this
 *                      session. Must be non-NULL.
 * @param account_id    Required. The Hydrogen account_id.
 * @param username      Optional; may be NULL.
 * @param roles         Optional; may be NULL.
 * @param client_ip     Optional; may be NULL. Stored for IP-bind
 *                      enforcement at take time.
 * @param expires_at    UNIX timestamp the JWT expires at; surfaced
 *                      to the SPA. Pass 0 to indicate unknown.
 * @param ttl_seconds   TTL for the *handoff record itself*. If
 *                      `<= 0`, the `default_ttl_seconds` from
 *                      `init` is used. This is independent of the
 *                      JWT's `exp` — the handoff record exists
 *                      only between callback and SPA pickup.
 * @return `true` on success, `false` on bad inputs, allocation
 *         failure, or if the store has not been initialized.
 */
bool oidc_rp_handoff_store_put(const char *handoff,
                               const char *jwt,
                               int         account_id,
                               const char *username,
                               const char *roles,
                               const char *client_ip,
                               time_t      expires_at,
                               int         ttl_seconds);

/**
 * @brief Atomically look up and remove a handoff record.
 *
 * If the store has a non-expired entry for `handoff`, returns a
 * heap record (caller must free with `oidc_rp_handoff_record_free`)
 * and removes it from the store so it cannot be replayed.
 *
 * Returns NULL if `handoff` is unknown, the record has expired (in
 * which case it is also removed), or the store is uninitialized.
 *
 * @param handoff The handoff key generated by `/callback` and
 *                echoed back by the SPA as the body of `/handoff`.
 * @return Heap-allocated record, or NULL.
 */
OidcRpHandoffRecord *oidc_rp_handoff_store_take(const char *handoff);

/**
 * @brief Free a record returned by `oidc_rp_handoff_store_take`.
 *
 * Scrubs the sensitive `jwt` field with `memset` before releasing
 * it, mirroring the discipline applied to other secret-bearing
 * structs in the codebase. NULL-safe.
 *
 * @param record Record to release.
 */
void oidc_rp_handoff_record_free(OidcRpHandoffRecord *record);

/**
 * @brief Sweep all expired records right now.
 *
 * Normally callers do not need to invoke this — the sweeper thread
 * and the `put` inline sweep handle it. Exposed for unit tests
 * (which want a deterministic sweep) and for the `/handoff`
 * handler to call defensively when it sees a "handoff_invalid"
 * event burst.
 *
 * @return Number of records removed.
 */
size_t oidc_rp_handoff_store_sweep_expired(void);

/**
 * @brief Number of records currently held in the store.
 *
 * Returned value is a snapshot; concurrent puts/takes may have
 * already changed the count by the time the caller reads it.
 * Returns 0 when the store is uninitialized.
 */
size_t oidc_rp_handoff_store_size(void);

/**
 * @brief Test-only: disable the background sweeper thread.
 *
 * When called before `oidc_rp_handoff_store_init`, the init
 * function will not spawn the sweeper. Tests use this so they can
 * drive `oidc_rp_handoff_store_sweep_expired` deterministically
 * without racing a background thread. Calls after init have no
 * effect on the current lifecycle.
 *
 * Production code MUST NOT call this.
 */
void oidc_rp_handoff_store_test_disable_sweeper(void);

/**
 * @brief Test-only: re-enable the background sweeper thread.
 *
 * Counterpart to `oidc_rp_handoff_store_test_disable_sweeper` for
 * tests that toggle the flag mid-suite.
 */
void oidc_rp_handoff_store_test_enable_sweeper(void);

#endif // OIDC_RP_HANDOFF_STORE_H
