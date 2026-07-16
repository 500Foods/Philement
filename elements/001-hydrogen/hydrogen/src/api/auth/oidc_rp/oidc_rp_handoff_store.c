/*
 * OIDC Relying Party in-memory handoff store — Phase 13
 *
 * See `oidc_rp_handoff_store.h` for the API contract and
 * `docs/OIDC-PLAN.md` Phase 13 for the design rationale. The
 * structural pattern mirrors the Phase 7 state store
 * (`oidc_rp_state.{c,h}`) deliberately:
 *
 *   - A chained hash table with FNV-1a hashing, fixed bucket count.
 *   - Guarded by a single mutex; every public function takes/releases it.
 *   - Reaped by a background sweeper thread that uses
 *     `pthread_cond_timedwait` so shutdown wakes it immediately
 *     rather than waiting for the next sweep tick.
 *   - Opportunistically sweeped inline on every `put` (capped work),
 *     so even with the sweeper disabled the store cannot grow
 *     unboundedly under traffic.
 *   - The sensitive `jwt` field is scrubbed before free per the
 *     plan's logging/secrets discipline (line 765).
 *
 * The handoff store is intentionally NOT merged with the state
 * store: their value types are different (state holds nonce +
 * code_verifier + return_to; handoff holds jwt + account_id +
 * roles + expires_at). A typed-wrapper merge would force the
 * larger struct on every state record. Two parallel files cost
 * ~30 lines of structural duplication and read more cleanly.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_handoff_store.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Bucket count. Handoff codes are 64 hex chars in production (32
// random bytes), same as state keys, so collisions are dominated by
// load not by clustering. Handoff records live for at most a minute
// each so concurrency is even lower than the state store; 64
// buckets is plenty.
#define OIDC_RP_HANDOFF_BUCKETS 64

// Inline sweep cap. `put` walks at most this many entries looking
// for expired records; bounded so a single put is always fast.
#define OIDC_RP_HANDOFF_INLINE_SWEEP_MAX 8

// Sweeper thread tick. Plan default handoff TTL is 60s; a 15s tick
// gives ~4x oversampling so expired records are reaped within a
// quarter-TTL of their actual expiry.
#define OIDC_RP_HANDOFF_SWEEPER_TICK_SECONDS 15

typedef struct HandoffEntry {
    OidcRpHandoffRecord  record;   // Owned strings, see header
    struct HandoffEntry *next;     // Chain
} HandoffEntry;

typedef struct HandoffStore {
    HandoffEntry *buckets[OIDC_RP_HANDOFF_BUCKETS];
    size_t        entry_count;
    int           default_ttl_seconds;

    pthread_mutex_t lock;          // Guards everything above

    // Sweeper thread machinery
    pthread_t       sweeper_thread;
    pthread_mutex_t sweeper_mutex; // Pairs with sweeper_cond
    pthread_cond_t  sweeper_cond;
    bool            sweeper_running;     // True between thread spawn and join
    bool            shutdown_requested;  // Set by shutdown(); checked by sweeper
} HandoffStore;

// Single global store. NULL when not initialized.
static HandoffStore *g_handoff_store = NULL;

// Test-mode flag; checked by init() before spawning the sweeper.
// When `true`, init skips `pthread_create` and the store relies
// solely on inline-sweep + manual `oidc_rp_handoff_store_sweep_expired`.
static bool g_handoff_test_disable_sweeper = false;

// ---------------------------------------------------------------------------
// Hashing + small string helpers
// ---------------------------------------------------------------------------

// FNV-1a, 64-bit. Same hashing as the state store; the small fixed
// bucket count makes both choices equivalent in practice.
uint64_t handoff_fnv1a_hash(const char *s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 0x100000001b3ULL;
    }
    return h;
}

size_t handoff_bucket_for(const char *key) {
    return (size_t)(handoff_fnv1a_hash(key) % OIDC_RP_HANDOFF_BUCKETS);
}

// strdup that returns NULL when src is NULL (instead of aborting).
char *handoff_opt_strdup(const char *src) {
    if (!src) return NULL;
    return strdup(src);
}

// Scrub a heap string in place (zero its bytes) and free it.
// Used for the JWT, which is the only sensitive field in a handoff
// record (per the plan's deny-list at line 765).
void handoff_scrub_free(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    // Use volatile pointer dance to defeat over-eager dead-store elimination.
    volatile char *vp = (volatile char *)s;
    for (size_t i = 0; i < n; i++) vp[i] = 0;
    free(s);
}

// Free the heap-owned strings of a record without freeing the record
// itself. The `jwt` field is scrubbed; the others use plain free.
void handoff_record_free_fields(OidcRpHandoffRecord *r) {
    if (!r) return;
    free(r->handoff);                    r->handoff   = NULL;
    handoff_scrub_free(r->jwt);          r->jwt       = NULL;
    free(r->username);                   r->username  = NULL;
    free(r->roles);                      r->roles     = NULL;
    free(r->client_ip);                  r->client_ip = NULL;
}

// ---------------------------------------------------------------------------
// Lookup helpers (assume lock is held)
// ---------------------------------------------------------------------------

// Returns true if `entry` has aged past its TTL relative to `now`.
bool handoff_entry_expired(const HandoffEntry *entry, time_t now) {
    if (entry->record.ttl_seconds <= 0) return false;
    return (now - entry->record.created_at) >= entry->record.ttl_seconds;
}

// Detach `entry` from its bucket chain. `prev` is NULL when `entry`
// is the head. Caller must hold `g_handoff_store->lock`.
void handoff_detach_entry(size_t bucket_idx,
                                 HandoffEntry *prev,
                                 HandoffEntry *entry) {
    if (prev) {
        prev->next = entry->next;
    } else {
        g_handoff_store->buckets[bucket_idx] = entry->next;
    }
    g_handoff_store->entry_count--;
}

// Free an entry (record fields + entry itself).
void handoff_free_entry(HandoffEntry *entry) {
    if (!entry) return;
    handoff_record_free_fields(&entry->record);
    free(entry);
}

// Remove and free any entry matching `handoff`. Caller holds the lock.
// Returns true if something was removed.
bool handoff_remove_locked(const char *handoff) {
    size_t b = handoff_bucket_for(handoff);
    HandoffEntry *prev = NULL;
    for (HandoffEntry *cur = g_handoff_store->buckets[b];
         cur;
         prev = cur, cur = cur->next) {
        if (cur->record.handoff &&
            strcmp(cur->record.handoff, handoff) == 0) {
            handoff_detach_entry(b, prev, cur);
            handoff_free_entry(cur);
            return true;
        }
    }
    return false;
}

// Walk the table once, removing every expired entry. Returns the
// number removed. Caller holds the lock.
size_t handoff_sweep_expired_locked(time_t now) {
    size_t removed = 0;
    for (size_t b = 0; b < OIDC_RP_HANDOFF_BUCKETS; b++) {
        HandoffEntry *prev = NULL;
        HandoffEntry *cur  = g_handoff_store->buckets[b];
        while (cur) {
            HandoffEntry *next = cur->next;
            if (handoff_entry_expired(cur, now)) {
                handoff_detach_entry(b, prev, cur);
                handoff_free_entry(cur);
                removed++;
                // prev unchanged; cur was detached
            } else {
                prev = cur;
            }
            cur = next;
        }
    }
    return removed;
}

// Cheap inline sweep: walks at most OIDC_RP_HANDOFF_INLINE_SWEEP_MAX
// entries (across buckets) and removes any that are expired. Bounded
// work so callers' latency stays predictable. Caller holds the lock.
void handoff_inline_sweep_locked(time_t now) {
    size_t visited = 0;
    for (size_t b = 0; b < OIDC_RP_HANDOFF_BUCKETS; b++) {
        HandoffEntry *prev = NULL;
        HandoffEntry *cur  = g_handoff_store->buckets[b];
        while (cur && visited < OIDC_RP_HANDOFF_INLINE_SWEEP_MAX) {
            HandoffEntry *next = cur->next;
            visited++;
            if (handoff_entry_expired(cur, now)) {
                handoff_detach_entry(b, prev, cur);
                handoff_free_entry(cur);
            } else {
                prev = cur;
            }
            cur = next;
        }
        if (visited >= OIDC_RP_HANDOFF_INLINE_SWEEP_MAX) break;
    }
}

// ---------------------------------------------------------------------------
// Sweeper thread
// ---------------------------------------------------------------------------

void *handoff_sweeper_loop(void *arg) {
    (void)arg;
    log_this(SR_AUTH, "OIDC RP handoff-store sweeper started",
             LOG_LEVEL_DEBUG, 0);

    while (true) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += OIDC_RP_HANDOFF_SWEEPER_TICK_SECONDS;

        pthread_mutex_lock(&g_handoff_store->sweeper_mutex);
        if (!g_handoff_store->shutdown_requested) {
            pthread_cond_timedwait(&g_handoff_store->sweeper_cond,
                                   &g_handoff_store->sweeper_mutex, &ts);
        }
        bool stop = g_handoff_store->shutdown_requested;
        pthread_mutex_unlock(&g_handoff_store->sweeper_mutex);

        if (stop) break;

        // Do the actual sweep under the store lock.
        pthread_mutex_lock(&g_handoff_store->lock);
        size_t removed = handoff_sweep_expired_locked(time(NULL));
        pthread_mutex_unlock(&g_handoff_store->lock);

        if (removed > 0) {
            log_this(SR_AUTH,
                     "OIDC RP handoff-store swept %zu expired records",
                     LOG_LEVEL_DEBUG, 1, removed);
        }
    }

    log_this(SR_AUTH, "OIDC RP handoff-store sweeper exiting",
             LOG_LEVEL_DEBUG, 0);
    return NULL;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool oidc_rp_handoff_store_init(int default_ttl_seconds) {
    if (g_handoff_store) {
        log_this(SR_AUTH,
                 "OIDC RP handoff-store already initialized; ignoring re-init",
                 LOG_LEVEL_DEBUG, 0);
        return true;
    }

    HandoffStore *store = calloc(1, sizeof(*store));
    if (!store) {
        log_this(SR_AUTH, "OIDC RP handoff-store: calloc failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    store->default_ttl_seconds =
        (default_ttl_seconds > 0) ? default_ttl_seconds : 60;

    if (pthread_mutex_init(&store->lock, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP handoff-store: lock init failed",
                 LOG_LEVEL_ERROR, 0);
        free(store);
        return false;
    }
    if (pthread_mutex_init(&store->sweeper_mutex, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP handoff-store: sweeper mutex init failed",
                 LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&store->lock);
        free(store);
        return false;
    }
    if (pthread_cond_init(&store->sweeper_cond, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP handoff-store: sweeper cond init failed",
                 LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&store->sweeper_mutex);
        pthread_mutex_destroy(&store->lock);
        free(store);
        return false;
    }

    g_handoff_store = store;

    if (!g_handoff_test_disable_sweeper) {
        store->sweeper_running = true;
        if (pthread_create(&store->sweeper_thread, NULL,
                           handoff_sweeper_loop, NULL) != 0) {
            log_this(SR_AUTH,
                     "OIDC RP handoff-store: sweeper thread create failed; "
                     "continuing without background sweeps",
                     LOG_LEVEL_ALERT, 0);
            store->sweeper_running = false;
            // Not fatal — inline sweeps + manual sweep cover correctness.
        }
    }

    log_this(SR_AUTH,
             "OIDC RP handoff-store initialized (default TTL=%ds, sweeper=%s)",
             LOG_LEVEL_STATE, 2,
             store->default_ttl_seconds,
             store->sweeper_running ? "on" : "off");
    return true;
}

void oidc_rp_handoff_store_shutdown(void) {
    HandoffStore *store = g_handoff_store;
    if (!store) return;

    // Signal sweeper to stop, then join.
    if (store->sweeper_running) {
        pthread_mutex_lock(&store->sweeper_mutex);
        store->shutdown_requested = true;
        pthread_cond_broadcast(&store->sweeper_cond);
        pthread_mutex_unlock(&store->sweeper_mutex);

        pthread_join(store->sweeper_thread, NULL);
        store->sweeper_running = false;
    }

    // Drain remaining entries.
    pthread_mutex_lock(&store->lock);
    for (size_t b = 0; b < OIDC_RP_HANDOFF_BUCKETS; b++) {
        HandoffEntry *cur = store->buckets[b];
        while (cur) {
            HandoffEntry *next = cur->next;
            handoff_free_entry(cur);
            cur = next;
        }
        store->buckets[b] = NULL;
    }
    store->entry_count = 0;
    pthread_mutex_unlock(&store->lock);

    pthread_cond_destroy(&store->sweeper_cond);
    pthread_mutex_destroy(&store->sweeper_mutex);
    pthread_mutex_destroy(&store->lock);

    g_handoff_store = NULL;
    free(store);

    log_this(SR_AUTH, "OIDC RP handoff-store shut down",
             LOG_LEVEL_STATE, 0);
}

bool oidc_rp_handoff_store_put(const char *handoff,
                               const char *jwt,
                               int         account_id,
                               const char *username,
                               const char *roles,
                               const char *client_ip,
                               time_t      expires_at,
                               int         ttl_seconds) {
    if (!g_handoff_store) {
        log_this(SR_AUTH,
                 "OIDC RP handoff-store: put called before init",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!handoff || !*handoff || !jwt) {
        log_this(SR_AUTH,
                 "OIDC RP handoff-store: put rejected — missing required field",
                 LOG_LEVEL_ALERT, 0);
        return false;
    }

    HandoffEntry *entry = calloc(1, sizeof(*entry));
    if (!entry) return false;

    entry->record.handoff     = strdup(handoff);
    entry->record.jwt         = strdup(jwt);
    entry->record.username    = handoff_opt_strdup(username);
    entry->record.roles       = handoff_opt_strdup(roles);
    entry->record.client_ip   = handoff_opt_strdup(client_ip);
    entry->record.account_id  = account_id;
    entry->record.expires_at  = expires_at;
    entry->record.created_at  = time(NULL);
    entry->record.ttl_seconds =
        (ttl_seconds > 0) ? ttl_seconds : g_handoff_store->default_ttl_seconds;

    if (!entry->record.handoff || !entry->record.jwt ||
        (username  && !entry->record.username) ||
        (roles     && !entry->record.roles) ||
        (client_ip && !entry->record.client_ip)) {
        // Partial allocation failure — release what we have.
        handoff_free_entry(entry);
        log_this(SR_AUTH, "OIDC RP handoff-store: put strdup failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    pthread_mutex_lock(&g_handoff_store->lock);

    // Replace any prior entry under the same key (defence in depth;
    // collision should not happen with random 32-byte handoff codes).
    handoff_remove_locked(handoff);

    size_t b = handoff_bucket_for(handoff);
    entry->next = g_handoff_store->buckets[b];
    g_handoff_store->buckets[b] = entry;
    g_handoff_store->entry_count++;

    // Opportunistic bounded sweep.
    handoff_inline_sweep_locked(time(NULL));

    pthread_mutex_unlock(&g_handoff_store->lock);
    return true;
}

OidcRpHandoffRecord *oidc_rp_handoff_store_take(const char *handoff) {
    if (!g_handoff_store || !handoff || !*handoff) return NULL;

    pthread_mutex_lock(&g_handoff_store->lock);
    size_t b = handoff_bucket_for(handoff);
    HandoffEntry *prev = NULL;
    HandoffEntry *cur  = g_handoff_store->buckets[b];
    while (cur) {
        if (cur->record.handoff &&
            strcmp(cur->record.handoff, handoff) == 0) {
            handoff_detach_entry(b, prev, cur);
            pthread_mutex_unlock(&g_handoff_store->lock);

            // Treat expired-on-take as a miss: free the record and
            // return NULL. The caller cannot tell expired-vs-unknown
            // apart, which matches the plan's "handoff_invalid"
            // semantics (single error envelope for all failures).
            if (handoff_entry_expired(cur, time(NULL))) {
                handoff_free_entry(cur);
                return NULL;
            }

            // Detach the embedded record and free the wrapping entry.
            OidcRpHandoffRecord *out = calloc(1, sizeof(*out));
            if (!out) {
                handoff_free_entry(cur);
                return NULL;
            }
            *out = cur->record;
            // The strings now belong to *out; null them in cur to
            // prevent double-free when we release the entry shell.
            memset(&cur->record, 0, sizeof(cur->record));
            free(cur);
            return out;
        }
        prev = cur;
        cur  = cur->next;
    }
    pthread_mutex_unlock(&g_handoff_store->lock);
    return NULL;
}

void oidc_rp_handoff_record_free(OidcRpHandoffRecord *record) {
    if (!record) return;
    handoff_record_free_fields(record);
    free(record);
}

size_t oidc_rp_handoff_store_sweep_expired(void) {
    if (!g_handoff_store) return 0;
    pthread_mutex_lock(&g_handoff_store->lock);
    size_t removed = handoff_sweep_expired_locked(time(NULL));
    pthread_mutex_unlock(&g_handoff_store->lock);
    return removed;
}

size_t oidc_rp_handoff_store_size(void) {
    if (!g_handoff_store) return 0;
    pthread_mutex_lock(&g_handoff_store->lock);
    size_t n = g_handoff_store->entry_count;
    pthread_mutex_unlock(&g_handoff_store->lock);
    return n;
}

void oidc_rp_handoff_store_test_disable_sweeper(void) {
    g_handoff_test_disable_sweeper = true;
}

void oidc_rp_handoff_store_test_enable_sweeper(void) {
    g_handoff_test_disable_sweeper = false;
}
