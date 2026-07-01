/*
 * Scripting Subsystem - Worker Pool
 *
 * Phase 7 of the LUA_PLAN. See worker_pool.h for the design.
 *
 * Job lifecycle (per worker, per job):
 *   1. dequeue a job_id (char*) from the job queue
 *   2. scoreboard_update_status(RUNNING) - stamps started_at
 *   3. scoreboard_find - get a heap copy of the entry
 *   4. script_registry_lookup - get the source for script_name
 *   5. H_lua_create_context - fresh lua_State
 *   6. H_lua_run_string - compile + pcall
 *   7. on success: update_status(COMPLETED); on failure:
 *      copy error string out of Lua (UAF discipline from Phase 1),
 *      lua_pop it, log it, update_status(FAILED)
 *   8. H_lua_destroy_context - close the fresh state
 *   9. free the entry copy and the job_id
 *  10. loop
 *
 * Shutdown: scripting_workers_destroy sets scripting_system_shutdown = 1
 * and waits for the queue to drain. Workers exit their loop when
 * the shutdown flag is set and the queue is empty.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "scripting.h"
#include "script_registry.h"
#include "worker_pool.h"
#include "lua_context.h"
#include "lua_hook.h"
#include "scoreboard.h"
#include <src/threads/threads.h>

extern volatile sig_atomic_t scripting_system_shutdown;
extern ServiceThreads scripting_threads;
extern struct Scoreboard* scripting_scoreboard;

// The worker pool global. NULL when the subsystem is not running.
ScriptingWorkerPool* scripting_workers = NULL;

// Idle sleep between empty queue polls (microseconds). Small enough to
// keep latency low, large enough to keep idle CPU near zero.
#define WORKER_POLL_USEC 1000

// Forward declarations.
static void* scripting_worker_thread(void* arg);
static void scripting_worker_process_one(ScriptingWorkerPool* pool,
                                          const char* job_id);
static bool scripting_worker_should_exit(ScriptingWorkerPool* pool);

/*
 * Initialize the worker pool. See worker_pool.h.
 */
bool scripting_workers_init(int worker_count) {
    if (scripting_workers) {
        // Idempotent.
        return true;
    }
    if (worker_count < 1) {
        log_this(SR_SCRIPTING, "scripting_workers_init: invalid worker_count",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    ScriptingWorkerPool* pool = calloc(1, sizeof(ScriptingWorkerPool));
    if (!pool) {
        return false;
    }

    pool->worker_count = worker_count;
    pool->worker_tids = calloc((size_t)worker_count, sizeof(pthread_t));
    pool->registry = script_registry_create();
    QueueAttributes attrs = {0};
    pool->job_queue = queue_create_with_label(SCRIPTING_JOB_QUEUE_NAME,
                                              &attrs, SR_SCRIPTING);

    if (!pool->worker_tids || !pool->registry || !pool->job_queue) {
        log_this(SR_SCRIPTING, "scripting_workers_init: allocation failure",
                 LOG_LEVEL_ERROR, 0);
        if (pool->worker_tids) free(pool->worker_tids);
        if (pool->registry) script_registry_destroy(pool->registry);
        if (pool->job_queue) queue_release(pool->job_queue);
        free(pool);
        return false;
    }

    // Take a reference for our own use.
    queue_acquire(pool->job_queue);

    // Defensive: if a prior shutdown left items in the queue
    // (shouldn't happen because destroy calls queue_clear, but we
    // are sharing the queue across inits via the global hash table),
    // clear it again so this init starts empty.
    queue_clear(pool->job_queue);

    // Reset the shutdown flag so this pool is active.
    scripting_system_shutdown = 0;

    // Publish the pool BEFORE starting the workers. The workers'
    // should_exit() checks scripting_workers, so it must be non-NULL
    // by the time they run their first loop iteration.
    scripting_workers = pool;

    // Start workers.
    bool ok = true;
    for (int i = 0; i < worker_count; i++) {
        if (pthread_create(&pool->worker_tids[i], NULL,
                           scripting_worker_thread, pool) != 0) {
            log_this(SR_SCRIPTING, "scripting_workers_init: pthread_create failed",
                     LOG_LEVEL_ERROR, 0);
            ok = false;
            break;
        }
        char desc[32];
        snprintf(desc, sizeof(desc), "worker-%d", i);
        add_service_thread_with_description(&scripting_threads,
                                            pool->worker_tids[i], desc);
    }

    if (!ok) {
        // Best-effort cleanup. Signal any already-started workers to
        // exit and join them; then tear down the rest. The workers
        // check scripting_workers->job_queue, so we keep `pool`
        // non-NULL until they are joined.
        scripting_system_shutdown = 1;
        scripting_workers = NULL;
        for (int i = 0; i < worker_count; i++) {
            if (pool->worker_tids[i]) {
                pthread_join(pool->worker_tids[i], NULL);
                remove_service_thread(&scripting_threads, pool->worker_tids[i]);
            }
        }
        queue_release(pool->job_queue);
        script_registry_destroy(pool->registry);
        free(pool->worker_tids);
        free(pool);
        return false;
    }

    log_this(SR_SCRIPTING, "Worker pool started: %d worker(s)",
             LOG_LEVEL_STATE, 1, worker_count);
    return true;
}

/*
 * Signal shutdown, drain, join, and free.
 */
void scripting_workers_destroy(void) {
    ScriptingWorkerPool* pool = scripting_workers;
    if (!pool) {
        return;
    }

    // Set the shutdown flag FIRST, then NULL the pool pointer. The
    // workers' should_exit() checks the shutdown flag AND an empty
    // queue, so setting shutdown first lets them drain the queue
    // before exiting. If we NULLed first, the workers' NULL check
    // would fire and they'd exit before draining.
    scripting_system_shutdown = 1;
    scripting_workers = NULL;

    for (int i = 0; i < pool->worker_count; i++) {
        pthread_join(pool->worker_tids[i], NULL);
        remove_service_thread(&scripting_threads, pool->worker_tids[i]);
    }

    // Drain any remaining job_ids in the queue. Workers may have been
    // mid-dequeue when shutdown was signalled, and a fresh init may
    // find the same queue in the hash table - so we must clear it
    // here to keep tests and re-launches deterministic.
    if (pool->job_queue) {
        queue_clear(pool->job_queue);
        queue_release(pool->job_queue);
        pool->job_queue = NULL;
    }

    if (pool->registry) {
        script_registry_destroy(pool->registry);
        pool->registry = NULL;
    }

    free(pool->worker_tids);
    free(pool);
    log_this(SR_SCRIPTING, "Worker pool stopped", LOG_LEVEL_STATE, 0);
}

/*
 * Submit a job whose source is already registered.
 */
char* scripting_submit_job(const char* script_name, const char* params_json) {
    if (!scripting_workers || !scripting_scoreboard || !script_name) {
        return NULL;
    }
    char* job_id = scoreboard_submit(scripting_scoreboard,
                                     script_name, params_json);
    if (!job_id) {
        return NULL;
    }
    if (!queue_enqueue(scripting_workers->job_queue,
                       job_id, strlen(job_id), 0)) {
        // Could not enqueue: mark the entry as FAILED so the scoreboard
        // doesn't carry an orphan PENDING row, and free the job_id.
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
        free(job_id);
        return NULL;
    }
    return job_id;
}

/*
 * Submit a job and register the source inline.
 */
char* scripting_submit_job_with_source(const char* script_name,
                                       const char* source,
                                       const char* params_json) {
    if (!scripting_workers || !scripting_scoreboard
        || !script_name || !source) {
        return NULL;
    }
    if (!script_registry_register(scripting_workers->registry,
                                  script_name, source)) {
        return NULL;
    }
    return scripting_submit_job(script_name, params_json);
}

/*
 * Phase 8: submit with explicit per-job resource limits.
 */
char* scripting_submit_job_with_source_and_limits(
    const char* script_name,
    const char* source,
    const char* params_json,
    const ScoreboardJobLimits* limits) {
    if (!scripting_workers || !scripting_scoreboard
        || !script_name || !source) {
        return NULL;
    }
    if (!script_registry_register(scripting_workers->registry,
                                  script_name, source)) {
        return NULL;
    }
    // Direct path: bypass the wrapper that re-resolves NULL limits to
    // config defaults (we already have the limits we want).
    char* job_id = scoreboard_submit_with_limits(scripting_scoreboard,
                                                 script_name, params_json,
                                                 limits);
    if (!job_id) {
        return NULL;
    }
    if (!queue_enqueue(scripting_workers->job_queue,
                       job_id, strlen(job_id), 0)) {
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
        free(job_id);
        return NULL;
    }
    return job_id;
}

/*
 * Should the worker exit? Yes if shutdown is signalled and the
 * queue is empty. We deliberately do NOT check the global
 * scripting_workers pointer here: it is NULLed in
 * scripting_workers_destroy() before pthread_join, and the workers
 * must keep draining the queue between those two events. Use the
 * worker's local pool pointer for the queue check.
 */
static bool scripting_worker_should_exit(ScriptingWorkerPool* pool) {
    if (!scripting_system_shutdown) {
        return false;
    }
    if (!pool || !pool->job_queue) {
        return true;
    }
    return queue_size(pool->job_queue) == 0;
}

/*
 * Worker thread main loop. Each worker is independent; no worker
 * shares its lua_State with any other (Phase 1 two-tier architecture).
 */
static void* scripting_worker_thread(void* arg) {
    ScriptingWorkerPool* pool = (ScriptingWorkerPool*)arg;

    while (1) {
        size_t size = 0;
        int prio = 0;
        char* job_id = queue_dequeue_nonblocking(pool->job_queue, &size, &prio);
        if (!job_id) {
            if (scripting_worker_should_exit(pool)) {
                return NULL;
            }
            usleep(WORKER_POLL_USEC);
            continue;
        }

        scripting_worker_process_one(pool, job_id);
        free(job_id);
    }
}

/*
 * Run one job. UAF discipline: any error string is copied out of Lua
 * memory to a C-owned buffer before the lua_State is destroyed.
 *
 * This function uses the worker's local pool pointer for the script
 * registry lookup, NOT the global scripting_workers. The destroy
 * path NULLs scripting_workers before pthread_join, and the workers
 * must continue processing the in-flight jobs through that window.
 */
static void scripting_worker_process_one(ScriptingWorkerPool* pool,
                                          const char* job_id) {
    if (!scripting_scoreboard || !pool) {
        return;
    }

    if (!scoreboard_update_status(scripting_scoreboard, job_id,
                                  SCOREBOARD_JOB_RUNNING)) {
        log_this(SR_SCRIPTING, "Worker: unknown job_id %s",
                 LOG_LEVEL_ALERT, 1, job_id);
        return;
    }

    ScoreboardEntry* entry = scoreboard_find(scripting_scoreboard, job_id);
    if (!entry) {
        // Lost the race with someone else; mark as FAILED.
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
        return;
    }

    const char* source = script_registry_lookup(pool->registry,
                                                entry->script_name);
    if (!source) {
        log_this(SR_SCRIPTING, "Worker [%s]: script not registered: %s",
                 LOG_LEVEL_ERROR, 2, job_id, entry->script_name);
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
        scoreboard_entry_free(entry);
        return;
    }

    lua_State* L = H_lua_create_context();
    if (!L) {
        log_this(SR_SCRIPTING, "Worker [%s]: failed to create Lua state",
                 LOG_LEVEL_ERROR, 1, job_id);
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
        scoreboard_entry_free(entry);
        return;
    }

    char chunk_name[64];
    snprintf(chunk_name, sizeof(chunk_name), "[job:%s]", job_id);

    // Phase 8: install the progress hook. We snapshot the per-job
    // limits from the entry copy (which is what scoreboard_find gave
    // us above) into the extraspace context, then call
    // H_lua_install_progress_hook to register the lua_sethook. The
    // hook reads the context on every tick to write progress back
    // to the scoreboard and enforce soft/hard limits.
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", job_id);
    ctx.scoreboard = scripting_scoreboard;
    ctx.hook_interval = entry->instruction_hook_interval;
    ctx.soft_limit_kb = entry->memory_soft_limit_kb;
    ctx.hard_limit_kb = entry->memory_hard_limit_kb;
    ctx.enforce_limits = entry->enforce_limits;
    ctx.soft_warned = false;
    ctx.local_instruction_count = 0;
    H_lua_set_job_context(L, &ctx);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, source, chunk_name);

    // Always tear down the hook and clear the context, even on error.
    // The hook was installed on a fresh per-job state, so uninstall
    // is cheap (lua_sethook(L, NULL, 0, 0)). Clearing the context
    // also frees the heap-allocated copy held in extraspace.
    H_lua_uninstall_progress_hook(L);
    H_lua_set_job_context(L, NULL);

    if (rc == LUA_OK) {
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_COMPLETED);
        // Drop any return values left on the stack.
        lua_pop(L, lua_gettop(L));
    } else {
        // Copy the error string out of Lua memory before closing the
        // state (UAF discipline from Phase 1).
        const char* lua_err = lua_tostring(L, -1);
        char* err_copy = lua_err ? strdup(lua_err) : NULL;
        if (err_copy) {
            log_this(SR_SCRIPTING, "Worker [%s]: job failed: %s",
                     LOG_LEVEL_ERROR, 2, job_id, err_copy);
            free(err_copy);
        } else {
            log_this(SR_SCRIPTING, "Worker [%s]: job failed (no message)",
                     LOG_LEVEL_ERROR, 1, job_id);
        }
        // Pop the error from the stack (H_lua_run_string leaves it).
        lua_pop(L, 1);
        scoreboard_update_status(scripting_scoreboard, job_id,
                                 SCOREBOARD_JOB_FAILED);
    }

    H_lua_destroy_context(L);
    scoreboard_entry_free(entry);
}
