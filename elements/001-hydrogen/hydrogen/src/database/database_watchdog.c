/*
 * Database Query Watchdog - Implementation
 *
 * Process-wide watchdog that tracks in-flight database queries and logs
 * an ALERT when any query exceeds its configured timeout. Step 1 of
 * the database timeout plan: detection and logging only. Engine-
 * specific cancel hooks and caller-side retry logic are added in
 * later steps.
 *
 * See database_watchdog.h for the public API and rationale.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "database.h"
#include "database_engine.h"
#include "database_watchdog.h"

/*
 * One-second tick. The timer thread sleeps for this long between
 * scans of the registry. Long enough to keep CPU idle, short enough
 * that an ALERT appears in the log within a second of the timeout
 * firing. The cancel hook added in step 3 will use this same tick
 * as its firing interval.
 */
#define DATABASE_WATCHDOG_TICK_SECONDS 1

/*
 * Process-wide watchdog state. Allocated by database_watchdog_init
 * and freed by database_watchdog_shutdown. Held behind a pointer so
 * the static initializer isn't required (the mutex/cond need real
 * pthread_*_init calls).
 */
typedef struct {
    pthread_mutex_t registry_mutex;
    pthread_cond_t wakeup_cond;
    DatabaseWatchdogHandle* head;
    size_t count;
    volatile bool initialized;
    volatile bool shutdown_requested;
    pthread_t timer_thread;
} DatabaseWatchdogState;

static DatabaseWatchdogState* watchdog_state = NULL;

/*
 * Runtime timeout bounds and default. Initialized from the
 * DATABASE_WATCHDOG_*_SECONDS compile-time defaults; may be
 * overridden at init time via database_watchdog_set_bounds. Read
 * by watchdog_effective_timeout and used in the init log line.
 */
static int g_watchdog_min = DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS;
static int g_watchdog_max = DATABASE_WATCHDOG_MAX_TIMEOUT_SECONDS;
static int g_watchdog_default = DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS;

void database_watchdog_set_bounds(int min_seconds, int max_seconds, int default_seconds) {
    if (min_seconds > 0) {
        g_watchdog_min = min_seconds;
    }
    if (max_seconds > 0) {
        g_watchdog_max = max_seconds;
    }
    if (default_seconds > 0) {
        g_watchdog_default = default_seconds;
    }
    /*
     * Clamp min to max if the supplied values crossed. A misconfigured
     * max smaller than min would otherwise make every registered query
     * get pinned to the min, hiding the misconfiguration.
     */
    if (g_watchdog_min > g_watchdog_max) {
        g_watchdog_min = g_watchdog_max;
    }
}

/*
 * Forward declaration of the timer thread entry point. Defined
 * below. Kept non-static so it has a prototype visible to the
 * compiler (required by the build's strict prototype warnings).
 */
void* database_watchdog_thread_main(void* arg);

/*
 * Compute the effective timeout from a request. Clamps to the
 * configured [min, max] range, with 0 or negative values treated
 * as the default. The bounds and default are read from the
 * runtime globals set by database_watchdog_set_bounds; if that
 * setter was never called the compile-time defaults apply.
 */
static int watchdog_effective_timeout(int requested) {
    if (requested <= 0) {
        return g_watchdog_default;
    }
    if (requested < g_watchdog_min) {
        return g_watchdog_min;
    }
    if (requested > g_watchdog_max) {
        return g_watchdog_max;
    }
    return requested;
}

/*
 * Free a single entry. Caller is responsible for unlinking it from
 * the registry first.
 */
static void watchdog_entry_free(DatabaseWatchdogHandle* entry) {
    if (!entry) {
        return;
    }
    if (entry->query_id_copy) {
        free(entry->query_id_copy);
    }
    if (entry->designator_copy) {
        free(entry->designator_copy);
    }
    free(entry);
}

bool database_watchdog_init(void) {
    if (watchdog_state && watchdog_state->initialized) {
        return true;
    }

    if (!watchdog_state) {
        watchdog_state = calloc(1, sizeof(*watchdog_state));
        if (!watchdog_state) {
            log_this(SR_DATABASE, "Database watchdog: failed to allocate state", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    if (pthread_mutex_init(&watchdog_state->registry_mutex, NULL) != 0) {
        log_this(SR_DATABASE, "Database watchdog: failed to initialize registry mutex", LOG_LEVEL_ERROR, 0);
        free(watchdog_state);
        watchdog_state = NULL;
        return false;
    }

    if (pthread_cond_init(&watchdog_state->wakeup_cond, NULL) != 0) {
        log_this(SR_DATABASE, "Database watchdog: failed to initialize wakeup condition", LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&watchdog_state->registry_mutex);
        free(watchdog_state);
        watchdog_state = NULL;
        return false;
    }

    watchdog_state->head = NULL;
    watchdog_state->count = 0;
    watchdog_state->shutdown_requested = false;
    watchdog_state->initialized = true;

    if (pthread_create(&watchdog_state->timer_thread, NULL,
                       database_watchdog_thread_main, NULL) != 0) {
        log_this(SR_DATABASE, "Database watchdog: failed to start timer thread", LOG_LEVEL_ERROR, 0);
        pthread_cond_destroy(&watchdog_state->wakeup_cond);
        pthread_mutex_destroy(&watchdog_state->registry_mutex);
        free(watchdog_state);
        watchdog_state = NULL;
        return false;
    }

    log_this(SR_DATABASE, "Database query watchdog initialized (min=%ds, max=%ds, default=%ds, heartbeat=%ds)",
             LOG_LEVEL_STATE, 4,
             g_watchdog_min,
             g_watchdog_max,
             g_watchdog_default,
             DATABASE_WATCHDOG_HEARTBEAT_SECONDS);
    return true;
}

void database_watchdog_shutdown(void) {
    if (!watchdog_state || !watchdog_state->initialized) {
        return;
    }

    pthread_mutex_lock(&watchdog_state->registry_mutex);
    watchdog_state->shutdown_requested = true;
    pthread_cond_broadcast(&watchdog_state->wakeup_cond);
    pthread_mutex_unlock(&watchdog_state->registry_mutex);

    pthread_join(watchdog_state->timer_thread, NULL);

    DatabaseWatchdogHandle* entry = watchdog_state->head;
    while (entry) {
        DatabaseWatchdogHandle* next = entry->next;
        watchdog_entry_free(entry);
        entry = next;
    }

    pthread_cond_destroy(&watchdog_state->wakeup_cond);
    pthread_mutex_destroy(&watchdog_state->registry_mutex);
    free(watchdog_state);
    watchdog_state = NULL;
}

DatabaseWatchdogHandle* database_watchdog_register(DatabaseHandle* connection,
                                                   QueryRequest* request) {
    if (!watchdog_state || !watchdog_state->initialized) {
        return NULL;
    }
    if (!request) {
        return NULL;
    }

    DatabaseWatchdogHandle* entry = calloc(1, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    const char* query_id_src = request->query_id ? request->query_id : "?";
    entry->query_id_copy = strdup(query_id_src);
    if (!entry->query_id_copy) {
        watchdog_entry_free(entry);
        return NULL;
    }

    const char* designator_src = (connection && connection->designator)
                                     ? connection->designator
                                     : SR_DATABASE;
    entry->designator_copy = strdup(designator_src);
    if (!entry->designator_copy) {
        watchdog_entry_free(entry);
        return NULL;
    }

    entry->start_time = time(NULL);
    entry->effective_timeout_seconds = watchdog_effective_timeout(request->timeout_seconds);
    entry->connection = connection;
    entry->expired = false;
    entry->cancelled = false;
    entry->alerted_at = 0;

    pthread_mutex_lock(&watchdog_state->registry_mutex);
    entry->next = watchdog_state->head;
    watchdog_state->head = entry;
    watchdog_state->count++;
    pthread_mutex_unlock(&watchdog_state->registry_mutex);

    return entry;
}

void database_watchdog_deregister(DatabaseWatchdogHandle* handle) {
    if (!handle) {
        return;
    }

    if (!watchdog_state || !watchdog_state->initialized) {
        /*
         * Watchdog was never started (or was already shut down).
         * The entry was allocated by register and the caller still
         * expects it to be freed, so we free it here even though it
         * is not in the registry.
         */
        watchdog_entry_free(handle);
        return;
    }

    pthread_mutex_lock(&watchdog_state->registry_mutex);

    DatabaseWatchdogHandle** link = &watchdog_state->head;
    bool found = false;
    while (*link) {
        if (*link == handle) {
            *link = handle->next;
            if (watchdog_state->count > 0) {
                watchdog_state->count--;
            }
            found = true;
            break;
        }
        link = &(*link)->next;
    }

    pthread_mutex_unlock(&watchdog_state->registry_mutex);

    if (found) {
        watchdog_entry_free(handle);
    }
    /*
     * If not found, the handle was already deregistered. The
     * conventional C contract is that the caller sets its pointer to
     * NULL after the first deregister; we do not double-free.
     */
}

bool database_watchdog_is_expired(const DatabaseWatchdogHandle* handle) {
    if (!handle) {
        return false;
    }
    return handle->expired;
}

size_t database_watchdog_active_count(void) {
    if (!watchdog_state || !watchdog_state->initialized) {
        return 0;
    }
    pthread_mutex_lock(&watchdog_state->registry_mutex);
    size_t n = watchdog_state->count;
    pthread_mutex_unlock(&watchdog_state->registry_mutex);
    return n;
}

void database_watchdog_check_expirations(void) {
    if (!watchdog_state || !watchdog_state->initialized) {
        return;
    }

    time_t now = time(NULL);

    /*
     * Stack buffer for connections we plan to cancel this tick. The
     * vast majority of ticks will have zero or one expired entry; the
     * fallback heap allocation only kicks in if more than 8 expire in
     * the same second. Holding the registry mutex while calling
     * engine cancel hooks would stall ALERT logging for other entries
     * (and engine cancel can perform network I/O), so we collect the
     * targets under the lock and invoke cancel after releasing it.
     */
    DatabaseHandle* inline_to_cancel[8];
    DatabaseHandle** to_cancel = inline_to_cancel;
    size_t to_cancel_count = 0;
    size_t to_cancel_cap = sizeof(inline_to_cancel) / sizeof(inline_to_cancel[0]);
    bool used_heap = false;

    pthread_mutex_lock(&watchdog_state->registry_mutex);
    for (DatabaseWatchdogHandle* entry = watchdog_state->head;
         entry != NULL;
         entry = entry->next) {
        if (!entry->expired) {
            time_t elapsed = now - entry->start_time;
            if (elapsed < entry->effective_timeout_seconds) {
                continue;
            }
            entry->expired = true;
        }

        if (entry->alerted_at != 0 &&
            (now - entry->alerted_at) < DATABASE_WATCHDOG_HEARTBEAT_SECONDS) {
            continue;
        }
        entry->alerted_at = now;

        const char* designator = entry->designator_copy ? entry->designator_copy : SR_DATABASE;
        const char* query_id = entry->query_id_copy ? entry->query_id_copy : "?";
        time_t elapsed = now - entry->start_time;
        long elapsed_s = (long)elapsed;
        long timeout_s = (long)entry->effective_timeout_seconds;
        log_this(designator,
                 "Database query exceeded watchdog timeout: query_id=%s, timeout=%lds, elapsed=%lds",
                 LOG_LEVEL_ALERT, 3,
                 query_id, timeout_s, elapsed_s);

        /*
         * Cancel the in-flight query exactly once per entry, on the
         * first transition to expired (or the first heartbeat re-alert
         * if a previous cancel attempt failed silently). Subsequent
         * heartbeats leave cancelled=true so we do not re-attempt.
         */
        if (!entry->cancelled && entry->connection != NULL) {
            entry->cancelled = true;
            if (to_cancel_count == to_cancel_cap) {
                size_t new_cap = to_cancel_cap * 2;
                DatabaseHandle** resized = realloc(used_heap ? to_cancel : NULL,
                                                   new_cap * sizeof(DatabaseHandle*));
                if (resized) {
                    if (!used_heap) {
                        memcpy(resized, inline_to_cancel,
                               to_cancel_count * sizeof(DatabaseHandle*));
                        used_heap = true;
                    }
                    to_cancel = resized;
                    to_cancel_cap = new_cap;
                } else {
                    /*
                     * Allocation failed - skip the cancel for this
                     * entry this tick. The cancelled flag is still
                     * set so we do not retry every heartbeat. The
                     * ALERT log already fired, so the hang is
                     * visible to operators regardless.
                     */
                    continue;
                }
            }
            to_cancel[to_cancel_count++] = entry->connection;
        }
    }
    pthread_mutex_unlock(&watchdog_state->registry_mutex);

    /*
     * Invoke the engine cancel hook on each collected connection
     * without holding the registry mutex. Engine cancels may perform
     * network I/O (PostgreSQL PQcancel opens a new TCP connection;
     * MySQL mysql_kill writes a KILL on the same socket) and we do
     * not want that I/O to block ALERT logging for other entries.
     */
    for (size_t i = 0; i < to_cancel_count; i++) {
        DatabaseHandle* conn = to_cancel[i];
        if (!conn || conn->engine_type >= DB_ENGINE_MAX) {
            continue;
        }
        DatabaseEngineInterface* engine = database_engine_get(conn->engine_type);
        if (engine && engine->cancel_inflight) {
            engine->cancel_inflight(conn);
        }
    }

    if (used_heap) {
        free(to_cancel);
    }
}

void* database_watchdog_thread_main(void* arg) {
    (void)arg;

    while (true) {
        struct timespec wake_at;
        clock_gettime(CLOCK_REALTIME, &wake_at);
        wake_at.tv_sec += DATABASE_WATCHDOG_TICK_SECONDS;

        pthread_mutex_lock(&watchdog_state->registry_mutex);
        bool do_shutdown = watchdog_state->shutdown_requested;
        if (!do_shutdown) {
            /*
             * Sleep until the next tick or until shutdown signals
             * us. The return value is intentionally ignored - a
             * timeout (normal tick) and a broadcast (shutdown) are
             * both handled by re-reading shutdown_requested below.
             */
            (void)pthread_cond_timedwait(&watchdog_state->wakeup_cond,
                                         &watchdog_state->registry_mutex,
                                         &wake_at);
            do_shutdown = watchdog_state->shutdown_requested;
        }
        pthread_mutex_unlock(&watchdog_state->registry_mutex);

        if (do_shutdown) {
            break;
        }

        database_watchdog_check_expirations();
    }

    return NULL;
}
