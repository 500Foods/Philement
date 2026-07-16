/*
 * Database Query Watchdog
 *
 * Process-wide watchdog that tracks in-flight database queries and logs
 * an ALERT when any query exceeds its configured timeout. This is the
 * client-side counterpart to engine-specific server-side timeouts (e.g.
 * PostgreSQL statement_timeout) and detects hangs caused by network or
 * client-side problems that the server cannot observe.
 *
 * Step 1 of the database timeout plan: registration, observation, and
 * logging only. Engine-specific cancel hooks and caller-side retry
 * logic are added in later steps.
 *
 * The watchdog is a single background thread that wakes once per second
 * and walks its registry. It never touches the database client library
 * itself - the goal is to *observe* hangs, not cancel them. Callers
 * (e.g. database_engine_execute) wrap their blocking call with
 * register/deregister and may use database_watchdog_is_expired to detect
 * that the call is taking too long.
 */

#ifndef DATABASE_WATCHDOG_H
#define DATABASE_WATCHDOG_H

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "database.h"  // DatabaseHandle, QueryRequest

/*
 * Bounds applied to registered query timeouts. Any value outside this
 * range is clamped. The lower bound prevents pathological short timeouts
 * from firing spuriously against slow but legitimate queries; the upper
 * bound prevents a hung query from holding a connection in the registry
 * longer than an hour.
 */
#define DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS 30
#define DATABASE_WATCHDOG_MAX_TIMEOUT_SECONDS 3600

/*
 * Default timeout applied when QueryRequest->timeout_seconds is 0 or
 * negative. The default matches the timeout currently hard-coded in
 * database_bootstrap.c so the watchdog behavior is a strict superset
 * of today's behavior until callers opt into per-request values.
 */
#define DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS 30

/*
 * Heartbeat interval for an already-expired query. Once a query has
 * crossed its timeout, the watchdog re-logs the ALERT every
 * HEARTBEAT seconds so monitoring tools see a regular heartbeat
 * (every 30s) confirming the watchdog is still on the job. Without
 * this, a query that hangs for an hour would log exactly one line.
 */
#define DATABASE_WATCHDOG_HEARTBEAT_SECONDS 30

/*
 * Registry entry. Returned as an opaque handle from
 * database_watchdog_register; the layout is exposed here so unit tests
 * (where the timer thread is mocked and never runs) can drive the
 * expiration scan directly by adjusting start_time and alerted_at.
 *
 * Callers should treat the handle as opaque: only the watchdog itself
 * reads/writes these fields in production code. The 'next' pointer
 * is internal linked-list linkage.
 *
 * Fields:
 *   next                       - internal linked-list linkage
 *   query_id_copy              - copy of the caller's request query_id
 *   designator_copy            - copy of the caller's connection designator
 *   start_time                 - wall-clock seconds when register was called
 *   effective_timeout_seconds  - clamped timeout the watchdog enforces
 *   connection                 - borrowed pointer to the caller's
 *                                DatabaseHandle; needed by the cancel
 *                                hook on first expiry. The caller
 *                                must keep the handle valid until
 *                                deregister.
 *   expired                    - set true once the query has exceeded
 *                                its timeout (sticky)
 *   cancelled                  - set true once the engine cancel hook
 *                                has been invoked for this entry
 *                                (one-shot; we don't re-cancel on
 *                                heartbeat re-alerts)
 *   alerted_at                 - wall-clock seconds of the most recent
 *                                ALERT log, or 0 if never alerted. The
 *                                scan re-alerts when now - alerted_at
 *                                >= DATABASE_WATCHDOG_HEARTBEAT_SECONDS.
 */
struct DatabaseWatchdogEntry {
    struct DatabaseWatchdogEntry* next;
    char* query_id_copy;
    char* designator_copy;
    time_t start_time;
    int effective_timeout_seconds;
    DatabaseHandle* connection;
    volatile bool expired;
    volatile bool cancelled;
    volatile time_t alerted_at;
};
typedef struct DatabaseWatchdogEntry DatabaseWatchdogHandle;

/*
 * Start the watchdog timer thread. Idempotent: a second call is a
 * no-op and returns true. Returns false on initialization failure
 * (e.g. thread creation failed). Must be called before any register
 * calls; the database subsystem's launch_database_subsystem() is the
 * intended caller.
 */
bool database_watchdog_init(void);

/*
 * Override the watchdog's timeout clamp bounds and default. The
 * values are read on every database_watchdog_register call, so
 * changes take effect immediately for new registrations. Calling
 * this BEFORE database_watchdog_init is recommended so the init
 * log line reports the active values; the compile-time defaults
 * from this header apply if the function is never called.
 *
 * Zero or negative arguments are ignored, leaving the corresponding
 * field at its previous value. If the resulting min exceeds max,
 * min is clamped down to max.
 *
 * The database subsystem's launch_database_subsystem() pulls these
 * values from app_config->databases.{watchdog_min_seconds,
 * watchdog_max_seconds, bootstrap_timeout_seconds} and calls this
 * function before init. Tests may also call it to verify the
 * runtime override path.
 */
void database_watchdog_set_bounds(int min_seconds,
                                  int max_seconds,
                                  int default_seconds);

/*
 * Stop the watchdog timer thread. Blocks until the thread exits.
 * Frees any registry entries that callers failed to deregister
 * (defensive - callers should always deregister). Idempotent and
 * safe to call even if init was never called. Intended caller is
 * the database subsystem's land_database_subsystem().
 */
void database_watchdog_shutdown(void);

/*
 * Register an in-flight query with the watchdog. The watchdog will
 * mark the handle as expired and log a single ALERT (via log_this) if
 * the query remains registered longer than the effective timeout
 * (request->timeout_seconds clamped to [MIN, MAX] bounds, with 0
 * treated as DEFAULT).
 *
 * Only the query_id and connection designator are read from the
 * request; they are copied internally so the caller may free the
 * request immediately after this call returns.
 *
 * Returns NULL on failure (watchdog not initialized, allocation
 * failure, or invalid arguments). The returned handle must be passed
 * to database_watchdog_deregister exactly once when the query
 * completes, regardless of outcome. A second deregister of the same
 * handle is a no-op.
 */
DatabaseWatchdogHandle* database_watchdog_register(DatabaseHandle* connection,
                                                   QueryRequest* request);

/*
 * Remove a previously-registered entry. Safe to call with NULL. The
 * handle is invalid after this call. No-op if the handle was already
 * deregistered or the watchdog is not initialized.
 */
void database_watchdog_deregister(DatabaseWatchdogHandle* handle);

/*
 * Returns true if the watchdog has marked this handle as expired
 * (i.e. the query has exceeded its timeout). Returns false for a
 * NULL handle, for a handle that was never registered, or for a
 * handle whose query is still within its timeout window. Callers
 * typically only use this for diagnostics - the watchdog's primary
 * job is to log the ALERT.
 */
bool database_watchdog_is_expired(const DatabaseWatchdogHandle* handle);

/*
 * Diagnostic: number of entries currently in the registry. Useful
 * for tests and for sanity-checking that all queries have been
 * deregistered at shutdown.
 */
size_t database_watchdog_active_count(void);

/*
 * Scan the registry once and mark any entries whose elapsed time
 * exceeds their timeout as expired (logging a one-shot ALERT for
 * each). Called by the watchdog timer thread once per second; also
 * exposed so unit tests (where pthread_create is mocked and the
 * timer thread never runs) can drive the scan directly.
 */
void database_watchdog_check_expirations(void);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 * -------------------------------------------------------------------------- */
int database_watchdog_effective_timeout(int requested);
void database_watchdog_entry_free(DatabaseWatchdogHandle* entry);

#endif // DATABASE_WATCHDOG_H
