/*
 * Scripting Subsystem - Worker Pool
 *
 * Phase 7 of the LUA_PLAN. The job execution pool for the Scripting
 * subsystem.
 *
 * Design (matches the LUA_PLAN Phase 1 two-tier architecture):
 *   - Workers are pthreads that loop, dequeue a job_id, create a FRESH
 *     lua_State for the job, run the script, update the scoreboard,
 *     destroy the lua_State, and loop. Threads are recycled; lua_State
 *     instances are not. This is the migration engine's proven pattern.
 *   - The job queue is a Queue* from src/queue/queue.h.
 *   - The script registry (script_registry.h) is the source-of-truth
 *     for script_name -> source. Workers look up the source by name
 *     before running. In Phase 20 this will be replaced by a
 *     DB-backed loader; the worker-side API is unchanged.
 *   - The Orchestrator (Phase 11) and the REST submit path (Phase 23)
 *     call scripting_submit_job() / scripting_submit_job_with_source()
 *     to enqueue work; the worker pool picks it up.
 *
 * Shutdown: setting scripting_system_shutdown = 1 makes workers exit
 * their loop as soon as the queue is drained. The
 * scripting_workers_destroy() call joins all worker threads before
 * the scoreboard is torn down (see land_scripting_subsystem()).
 */

#ifndef HYDROGEN_SCRIPTING_WORKER_POOL_H
#define HYDROGEN_SCRIPTING_WORKER_POOL_H

// System headers
#include <stdbool.h>
#include <stddef.h>

// Project headers
#include <pthread.h>
#include <src/queue/queue.h>
#include "script_registry.h"
#include "scoreboard.h"   // ScoreboardJobLimits

#define SCRIPTING_JOB_QUEUE_NAME "scripting_jobs"

/*
 * The worker pool: one job queue, N pthreads, one in-memory script
 * registry. Allocated by scripting_workers_init() and freed by
 * scripting_workers_destroy().
 *
 * Fields are private; callers use the public functions below.
 */
typedef struct ScriptingWorkerPool {
    Queue*             job_queue;
    ScriptRegistry*    registry;
    pthread_t*         worker_tids;
    int                worker_count;
} ScriptingWorkerPool;

/*
 * Initialize the worker pool. Creates the job queue, the script
 * registry, and starts `worker_count` pthreads. Each pthread is
 * registered with ServiceThreads (scripting_threads) with a
 * description like "worker-0", "worker-1", ...
 *
 * Idempotent: calling twice is a no-op the second time.
 *
 * Returns true on success, false on allocation or pthread failure
 * (in which case any partially-created state is torn down).
 */
bool scripting_workers_init(int worker_count);

/*
 * Signal shutdown, drain the queue (workers continue processing
 * until the queue is empty), and join all worker threads. Then free
 * the pool. Safe to call with NULL pool (no-op). Idempotent.
 *
 * After this call scripting_workers is NULL.
 */
void scripting_workers_destroy(void);

/*
 * Submit a job whose source is expected to be already registered
 * (production path: Orchestrator / REST submit go through a DB
 * loader that calls script_registry_register). Creates a scoreboard
 * entry and enqueues its job_id.
 *
 *   script_name - non-NULL script identifier.
 *   params_json - opaque JSON string for parameters, or NULL.
 *
 * Returns the heap-allocated job_id (caller frees) or NULL on
 * failure. The job_id is also enqueued onto the job queue; a
 * worker will pick it up.
 */
char* scripting_submit_job(const char* script_name, const char* params_json);

/*
 * Submit a job and register the source inline (testing / REPL /
 * eval path). The source is registered under script_name (replacing
 * any prior registration) before the job is enqueued.
 *
 * Returns the heap-allocated job_id (caller frees) or NULL on
 * failure.
 */
char* scripting_submit_job_with_source(const char* script_name,
                                       const char* source,
                                       const char* params_json);

/*
 * Phase 8: per-job resource limits. The canonical definition lives
 * in scoreboard.h (ScoreboardJobLimits) so the scoreboard, the
 * worker pool, and the hook all share one type. We include that
 * header above.
 *
 * enforce_limits = false opts the job out of soft/hard enforcement
 * while still updating progress in the scoreboard (the Orchestrator
 * uses this). NULL is equivalent to "all defaults, enforce=true".
 */

/*
 * Submit a job and register the source inline, with explicit
 * per-job resource limits. NULL limits behaves identically to
 * scripting_submit_job_with_source().
 *
 * Returns the heap-allocated job_id (caller frees) or NULL on
 * failure.
 */
char* scripting_submit_job_with_source_and_limits(
    const char* script_name,
    const char* source,
    const char* params_json,
    const ScoreboardJobLimits* limits);

#endif /* HYDROGEN_SCRIPTING_WORKER_POOL_H */
