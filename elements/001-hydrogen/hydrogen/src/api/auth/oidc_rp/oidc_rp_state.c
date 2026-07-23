/*
 * OIDC Relying Party in-memory state store — Phase 7
 *
 * See `oidc_rp_state.h` for the API contract and `docs/H/plans/OIDC-PLAN.md`
 * Phase 7 for the design rationale. This implementation is:
 *
 *   - A chained hash table with FNV-1a hashing, fixed bucket count.
 *   - Guarded by a single mutex; every public function takes/releases it.
 *   - Reaped by a background sweeper thread that uses
 *     `pthread_cond_timedwait` so shutdown wakes it immediately
 *     rather than waiting for the next sweep tick.
 *   - Opportunistically sweeped inline on every `put` (capped work),
 *     so even with the sweeper disabled the store cannot grow
 *     unboundedly under traffic.
 *   - Sensitive fields (`nonce`, `code_verifier`) are scrubbed before
 *     free per the plan's logging/secrets discipline.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_state.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// Bucket count. State keys are 64 hex chars in production (32 random
// bytes), so collisions are dominated by load not by clustering. 64
// buckets keeps memory tiny and chains short for the realistic
// in-flight count (single-digit tens at most).
#define OIDC_RP_STATE_BUCKETS 64

// Inline sweep cap. `put` walks at most this many entries looking for
// expired records; bounded so a single put is always fast.
#define OIDC_RP_STATE_INLINE_SWEEP_MAX 8

// Sweeper thread tick. Plan default state TTL is 600s (10 min); 30s
// gives ample reap responsiveness without busy work.
#define OIDC_RP_STATE_SWEEPER_TICK_SECONDS 30

typedef struct StateEntry {
    OidcRpStateRecord record;     // Owned strings, see header
    struct StateEntry *next;      // Chain
} StateEntry;

typedef struct StateStore {
    StateEntry *buckets[OIDC_RP_STATE_BUCKETS];
    size_t entry_count;
    int default_ttl_seconds;

    pthread_mutex_t lock;          // Guards everything above

    // Sweeper thread machinery
    pthread_t sweeper_thread;
    pthread_mutex_t sweeper_mutex; // Pairs with sweeper_cond
    pthread_cond_t sweeper_cond;
    bool sweeper_running;          // True between thread spawn and join
    bool shutdown_requested;       // Set by shutdown(); checked by sweeper
} StateStore;

// Single global store. NULL when not initialized.
static StateStore *g_store = NULL;

// Test-mode flag; checked by init() before spawning the sweeper.
// When `true`, init skips `pthread_create` and the store relies
// solely on inline-sweep + manual `oidc_rp_state_sweep_expired`.
static bool g_test_disable_sweeper = false;

// ---------------------------------------------------------------------------
// Hashing + small string helpers
// ---------------------------------------------------------------------------

// FNV-1a, 64-bit. Plenty for our bucket count.
uint64_t fnv1a_hash(const char *s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 0x100000001b3ULL;
    }
    return h;
}

size_t bucket_for(const char *key) {
    return (size_t)(fnv1a_hash(key) % OIDC_RP_STATE_BUCKETS);
}

// strdup that returns NULL when src is NULL (instead of aborting).
char *opt_strdup(const char *src) {
    if (!src) return NULL;
    return strdup(src);
}

// Scrub a heap string in place (zero its bytes) and free it.
void state_scrub_free(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    // Use volatile pointer dance to defeat over-eager dead-store elimination.
    volatile char *vp = (volatile char *)s;
    for (size_t i = 0; i < n; i++) vp[i] = 0;
    free(s);
}

// Free the heap-owned strings of a record without freeing the record
// itself. Sensitive fields are scrubbed; non-sensitive fields use
// plain free.
void record_free_fields(OidcRpStateRecord *r) {
    if (!r) return;
    free(r->state);          r->state = NULL;
    state_scrub_free(r->nonce);    r->nonce = NULL;
    state_scrub_free(r->code_verifier); r->code_verifier = NULL;
    free(r->database);       r->database = NULL;
    free(r->return_to);      r->return_to = NULL;
    free(r->client_ip);      r->client_ip = NULL;
    free(r->provider_name);  r->provider_name = NULL;
}

// ---------------------------------------------------------------------------
// Lookup helpers (assume lock is held)
// ---------------------------------------------------------------------------

// Returns true if `entry` has aged past its TTL relative to `now`.
bool state_entry_expired(const StateEntry *entry, time_t now) {
    if (entry->record.ttl_seconds <= 0) return false;
    return (now - entry->record.created_at) >= entry->record.ttl_seconds;
}

// Detach `entry` from its bucket chain. `prev` is NULL when `entry`
// is the head. Caller must hold `g_store->lock`.
void detach_entry(size_t bucket_idx, StateEntry *prev, StateEntry *entry) {
    if (prev) {
        prev->next = entry->next;
    } else {
        g_store->buckets[bucket_idx] = entry->next;
    }
    g_store->entry_count--;
}

// Free an entry (record fields + entry itself).
void free_entry(StateEntry *entry) {
    if (!entry) return;
    record_free_fields(&entry->record);
    free(entry);
}

// Remove and free any entry matching `state`. Caller holds the lock.
// Returns true if something was removed.
bool remove_locked(const char *state) {
    size_t b = bucket_for(state);
    StateEntry *prev = NULL;
    for (StateEntry *cur = g_store->buckets[b]; cur; prev = cur, cur = cur->next) {
        if (cur->record.state && strcmp(cur->record.state, state) == 0) {
            detach_entry(b, prev, cur);
            free_entry(cur);
            return true;
        }
    }
    return false;
}

// Walk the table once, removing every expired entry. Returns the
// number removed. Caller holds the lock.
size_t sweep_expired_locked(time_t now) {
    size_t removed = 0;
    for (size_t b = 0; b < OIDC_RP_STATE_BUCKETS; b++) {
        StateEntry *prev = NULL;
        StateEntry *cur  = g_store->buckets[b];
        while (cur) {
            StateEntry *next = cur->next;
            if (state_entry_expired(cur, now)) {
                detach_entry(b, prev, cur);
                free_entry(cur);
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

// Cheap inline sweep: walks at most OIDC_RP_STATE_INLINE_SWEEP_MAX
// entries (across buckets) and removes any that are expired. Bounded
// work so callers' latency stays predictable. Caller holds the lock.
void inline_sweep_locked(time_t now) {
    size_t visited = 0;
    for (size_t b = 0; b < OIDC_RP_STATE_BUCKETS; b++) {
        StateEntry *prev = NULL;
        StateEntry *cur  = g_store->buckets[b];
        while (cur && visited < OIDC_RP_STATE_INLINE_SWEEP_MAX) {
            StateEntry *next = cur->next;
            visited++;
            if (state_entry_expired(cur, now)) {
                detach_entry(b, prev, cur);
                free_entry(cur);
            } else {
                prev = cur;
            }
            cur = next;
        }
        if (visited >= OIDC_RP_STATE_INLINE_SWEEP_MAX) break;
    }
}

// ---------------------------------------------------------------------------
// Sweeper thread
// ---------------------------------------------------------------------------

void *sweeper_loop(void *arg) {
    (void)arg;
    log_this(SR_AUTH, "OIDC RP state-store sweeper started", LOG_LEVEL_DEBUG, 0);

    while (true) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += OIDC_RP_STATE_SWEEPER_TICK_SECONDS;

        pthread_mutex_lock(&g_store->sweeper_mutex);
        if (!g_store->shutdown_requested) {
            pthread_cond_timedwait(&g_store->sweeper_cond,
                                   &g_store->sweeper_mutex, &ts);
        }
        bool stop = g_store->shutdown_requested;
        pthread_mutex_unlock(&g_store->sweeper_mutex);

        if (stop) break;

        // Do the actual sweep under the store lock.
        pthread_mutex_lock(&g_store->lock);
        size_t removed = sweep_expired_locked(time(NULL));
        pthread_mutex_unlock(&g_store->lock);

        if (removed > 0) {
            log_this(SR_AUTH,
                     "OIDC RP state-store swept %zu expired records",
                     LOG_LEVEL_DEBUG, 1, removed);
        }
    }

    log_this(SR_AUTH, "OIDC RP state-store sweeper exiting", LOG_LEVEL_DEBUG, 0);
    return NULL;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool oidc_rp_state_init(int default_ttl_seconds) {
    if (g_store) {
        log_this(SR_AUTH,
                 "OIDC RP state-store already initialized; ignoring re-init",
                 LOG_LEVEL_DEBUG, 0);
        return true;
    }

    StateStore *store = calloc(1, sizeof(*store));
    if (!store) {
        log_this(SR_AUTH, "OIDC RP state-store: calloc failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    store->default_ttl_seconds =
        (default_ttl_seconds > 0) ? default_ttl_seconds : 600;

    if (pthread_mutex_init(&store->lock, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP state-store: lock init failed",
                 LOG_LEVEL_ERROR, 0);
        free(store);
        return false;
    }
    if (pthread_mutex_init(&store->sweeper_mutex, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP state-store: sweeper mutex init failed",
                 LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&store->lock);
        free(store);
        return false;
    }
    if (pthread_cond_init(&store->sweeper_cond, NULL) != 0) {
        log_this(SR_AUTH, "OIDC RP state-store: sweeper cond init failed",
                 LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&store->sweeper_mutex);
        pthread_mutex_destroy(&store->lock);
        free(store);
        return false;
    }

    g_store = store;

    if (!g_test_disable_sweeper) {
        store->sweeper_running = true;
        if (pthread_create(&store->sweeper_thread, NULL,
                           sweeper_loop, NULL) != 0) {
            log_this(SR_AUTH,
                     "OIDC RP state-store: sweeper thread create failed; "
                     "continuing without background sweeps",
                     LOG_LEVEL_ALERT, 0);
            store->sweeper_running = false;
            // Not fatal — inline sweeps + manual sweep cover correctness.
        }
    }

    log_this(SR_AUTH,
             "OIDC RP state-store initialized (default TTL=%ds, sweeper=%s)",
             LOG_LEVEL_STATE, 2,
             store->default_ttl_seconds,
             store->sweeper_running ? "on" : "off");
    return true;
}

void oidc_rp_state_shutdown(void) {
    StateStore *store = g_store;
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
    for (size_t b = 0; b < OIDC_RP_STATE_BUCKETS; b++) {
        StateEntry *cur = store->buckets[b];
        while (cur) {
            StateEntry *next = cur->next;
            free_entry(cur);
            cur = next;
        }
        store->buckets[b] = NULL;
    }
    store->entry_count = 0;
    pthread_mutex_unlock(&store->lock);

    pthread_cond_destroy(&store->sweeper_cond);
    pthread_mutex_destroy(&store->sweeper_mutex);
    pthread_mutex_destroy(&store->lock);

    g_store = NULL;
    free(store);

    log_this(SR_AUTH, "OIDC RP state-store shut down", LOG_LEVEL_STATE, 0);
}

bool oidc_rp_state_put(const char *state,
                       const char *nonce,
                       const char *code_verifier,
                       const char *database,
                       const char *return_to,
                       const char *client_ip,
                       const char *provider_name,
                       int ttl_seconds) {
    if (!g_store) {
        log_this(SR_AUTH,
                 "OIDC RP state-store: put called before init",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!state || !*state || !nonce || !code_verifier) {
        log_this(SR_AUTH,
                 "OIDC RP state-store: put rejected — missing required field",
                 LOG_LEVEL_ALERT, 0);
        return false;
    }

    StateEntry *entry = calloc(1, sizeof(*entry));
    if (!entry) return false;

    entry->record.state         = strdup(state);
    entry->record.nonce         = strdup(nonce);
    entry->record.code_verifier = strdup(code_verifier);
    entry->record.database      = opt_strdup(database);
    entry->record.return_to     = opt_strdup(return_to);
    entry->record.client_ip     = opt_strdup(client_ip);
    entry->record.provider_name = opt_strdup(provider_name);
    entry->record.created_at    = time(NULL);
    entry->record.ttl_seconds   =
        (ttl_seconds > 0) ? ttl_seconds : g_store->default_ttl_seconds;

    if (!entry->record.state || !entry->record.nonce ||
        !entry->record.code_verifier ||
        (database  && !entry->record.database) ||
        (return_to && !entry->record.return_to) ||
        (client_ip && !entry->record.client_ip) ||
        (provider_name && !entry->record.provider_name)) {
        // Partial allocation failure — release what we have.
        free_entry(entry);
        log_this(SR_AUTH, "OIDC RP state-store: put strdup failed",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    pthread_mutex_lock(&g_store->lock);

    // Replace any prior entry under the same key (defence in depth;
    // collision should not happen with random 32-byte state values).
    remove_locked(state);

    size_t b = bucket_for(state);
    entry->next = g_store->buckets[b];
    g_store->buckets[b] = entry;
    g_store->entry_count++;

    // Opportunistic bounded sweep.
    inline_sweep_locked(time(NULL));

    pthread_mutex_unlock(&g_store->lock);
    return true;
}

OidcRpStateRecord *oidc_rp_state_take(const char *state) {
    if (!g_store || !state || !*state) return NULL;

    pthread_mutex_lock(&g_store->lock);
    size_t b = bucket_for(state);
    StateEntry *prev = NULL;
    StateEntry *cur  = g_store->buckets[b];
    while (cur) {
        if (cur->record.state && strcmp(cur->record.state, state) == 0) {
            detach_entry(b, prev, cur);
            pthread_mutex_unlock(&g_store->lock);

            // Treat expired-on-take as a miss: free the record and
            // return NULL. The caller cannot tell expired-vs-unknown
            // apart, which matches the plan's "state_invalid" semantics.
            if (state_entry_expired(cur, time(NULL))) {
                free_entry(cur);
                return NULL;
            }

            // Detach the embedded record and free the wrapping entry.
            OidcRpStateRecord *out = calloc(1, sizeof(*out));
            if (!out) {
                free_entry(cur);
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
    pthread_mutex_unlock(&g_store->lock);
    return NULL;
}

void oidc_rp_state_record_free(OidcRpStateRecord *record) {
    if (!record) return;
    record_free_fields(record);
    free(record);
}

size_t oidc_rp_state_sweep_expired(void) {
    if (!g_store) return 0;
    pthread_mutex_lock(&g_store->lock);
    size_t removed = sweep_expired_locked(time(NULL));
    pthread_mutex_unlock(&g_store->lock);
    return removed;
}

size_t oidc_rp_state_size(void) {
    if (!g_store) return 0;
    pthread_mutex_lock(&g_store->lock);
    size_t n = g_store->entry_count;
    pthread_mutex_unlock(&g_store->lock);
    return n;
}

void oidc_rp_state_test_disable_sweeper(void) {
    g_test_disable_sweeper = true;
}

void oidc_rp_state_test_enable_sweeper(void) {
    g_test_disable_sweeper = false;
}
