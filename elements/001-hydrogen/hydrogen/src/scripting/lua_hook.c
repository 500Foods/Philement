/*
 * Scripting Subsystem - Progress Hook
 *
 * Phase 8 of the LUA_PLAN. See lua_hook.h for the design.
 * Phase 10 added per-tick kill checks (kill_requested and
 * max_runtime_seconds).
 *
 * Implementation notes:
 *
 *   - lua_sethook(L, fn, LUA_MASKCOUNT, N) fires the hook every N
 *     VM instructions. We read N from the per-state job context,
 *     which was filled from the scoreboard entry at submit time.
 *
 *   - The hook function reads the job context via
 *     H_lua_get_job_context() and writes the updated counters to the
 *     scoreboard via scoreboard_update_progress(). The scoreboard
 *     write is a mutex-bounded critical section that just touches
 *     two fields, so it is safe to call from inside a hook.
 *
 *   - We never call back into the host API (H.*) from the hook.
 *     The hook is a leaf, in keeping with the Phase 6 rule that
 *     the host log path should never crash the host.
 *
 *   - On hard-limit breach we call luaL_error, which longjmps to
 *     the nearest lua_pcall. The worker (worker_pool.c) treats
 *     that the same as any other runtime error: copy the error
 *     out of Lua memory (UAF discipline from Phase 1), log it,
 *     mark the scoreboard FAILED, and lua_close the state. If the
 *     job's kill_requested flag was set, the worker marks the job
 *     KILLED instead.
 *
 *   - Phase 10 adds two more luaL_error triggers, both checked on
 *     every tick (not just every sample_every tick) so kill latency
 *     is bounded by hook_interval (microseconds at typical rates):
 *       1. scoreboard_is_kill_requested() returned true. This is a
 *          live read of the scoreboard (mutex-bounded bool), so an
 *          external caller can flip the flag mid-run and the next
 *          tick trips the kill.
 *       2. elapsed time since the snapshotted started_at >=
 *          max_runtime_seconds. The worker copies started_at and
 *          max_runtime_seconds into the per-state context at submit
 *          time so the hook does not re-read the scoreboard for
 *          these.
 *     Both checks are gated by enforce_limits so the Orchestrator
 *     (which sets enforce_limits=false) is never killed by the hook.
 *
 *   - The hook is uninstalled by H_lua_uninstall_progress_hook.
 *     Workers call this between jobs and on shutdown.
 */

 // Project includes
#include <src/hydrogen.h>

 // Standard includes
#include <stdint.h>
#include <limits.h>   // INT_MAX (Phase 10: "no max runtime" sentinel)
#include <time.h>     // clock_gettime, struct timespec (Phase 10: elapsed time)

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "lua_hook.h"
#include "lua_context.h"
#include "scoreboard.h"

// Default: sample memory every 20 hook calls. At the default
// InstructionHookInterval=5000, that is one GC sample per 100K VM
// instructions. The actual value used at runtime is read from
// app_config->scripting.MemorySampleEveryNHooks (filled into the
// per-state job context by the worker).
#define H_LUA_HOOK_DEFAULT_SAMPLE_EVERY 20

// The hook function. Signature matches lua_Hook.
static void H_lua_progress_hook_fn(lua_State* L, lua_Debug* ar) {
    (void)ar;
    if (!L) {
        return;
    }
    H_lua_job_context* ctx = H_lua_get_job_context(L);
    if (!ctx || ctx->job_id[0] == '\0' || !ctx->scoreboard) {
        return;
    }

    // Cheap path: bump the local counter on every hook tick.
    ctx->local_instruction_count++;

    // Phase 10: per-tick kill checks. Both are gated by
    // enforce_limits so the Orchestrator (and any opt-out job)
    // can run indefinitely. We check these on every tick (not just
    // every sample_every tick) so kill latency is bounded by
    // hook_interval. The cost is two short, mutex-bounded reads.
    if (ctx->enforce_limits) {
        // 1. External kill request: live read of the scoreboard
        //    flag. Any thread can flip the flag via
        //    scoreboard_request_kill; the next tick trips the kill.
        bool kill_now = false;
        if (scoreboard_is_kill_requested(ctx->scoreboard, ctx->job_id, &kill_now)
            && kill_now) {
            // The flag is already set (by an external caller); just
            // raise the kill error.
            char msg[96];
            snprintf(msg, sizeof(msg), "killed: requested (job %s)", ctx->job_id);
            luaL_error(L, "%s", msg);
            // Unreachable - luaL_error does not return.
        }

        // 2. Max runtime: elapsed since the snapshotted started_at.
        //    started_at.tv_sec == 0 means "not yet started"; skip
        //    the check to avoid a spurious kill on the first tick
        //    after a fast PENDING->RUNNING transition (the worker's
        //    scoreboard_update_status stamps started_at before
        //    installing the hook, so this branch is defensive).
        //    max_runtime_seconds <= 0 or INT_MAX means "no limit".
        if (ctx->max_runtime_seconds > 0
            && ctx->max_runtime_seconds < INT_MAX
            && ctx->started_at.tv_sec > 0) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed = (long)(now.tv_sec - ctx->started_at.tv_sec);
            if (elapsed < 0) {
                elapsed = 0;  // clock skew / very fast transitions
            }
            if (elapsed >= ctx->max_runtime_seconds) {
                // Mark the entry as kill_requested BEFORE raising
                // luaL_error, so the worker's KILLED-vs-FAILED
                // classifier (which calls scoreboard_is_kill_requested
                // after pcall returns) sees a true flag and
                // classifies as KILLED. Without this, the worker
                // has to second-guess "was this a kill or a normal
                // error?" by inspecting the error string or the
                // wall-clock seconds since RUNNING, both of which
                // are racy (string format may change; sub-second
                // wall-clock advances mean the time-elapsed check
                // can miss by 1).
                scoreboard_request_kill(ctx->scoreboard, ctx->job_id);
                char msg[160];
                snprintf(msg, sizeof(msg),
                         "max runtime exceeded (job %s: %lds >= %ds)",
                         ctx->job_id, elapsed, ctx->max_runtime_seconds);
                luaL_error(L, "%s", msg);
                // Unreachable - luaL_error does not return.
            }
        }
    }

    // Expensive path: every sample_every hook calls, read the GC
    // memory counter and flush the progress to the scoreboard.
    // sample_every is 0 means "sample every tick" (defensive - the
    // worker should always set a positive value).
    int sample_every = H_LUA_HOOK_DEFAULT_SAMPLE_EVERY;
    if (app_config && app_config->scripting.MemorySampleEveryNHooks > 0) {
        sample_every = app_config->scripting.MemorySampleEveryNHooks;
    }
    if (sample_every <= 0) {
        sample_every = 1;
    }

    if ((ctx->local_instruction_count % (uint64_t)sample_every) != 0) {
        return;
    }

    // Sample memory. lua_gc(L, LUA_GCCOUNT, 0) returns memory in KB
    // as a lua_Integer. We treat 0 as a valid value (very small state).
    lua_Integer kb = lua_gc(L, LUA_GCCOUNT, 0);
    if (kb < 0) {
        kb = 0;
    }
    size_t memory_kb = (size_t)kb;

    // Write to scoreboard (cheap, mutex-bounded).
    scoreboard_update_progress(ctx->scoreboard, ctx->job_id,
                               ctx->local_instruction_count, memory_kb);

    // Enforce memory limits only when the job opted in (workers do;
    // the Orchestrator does not, so the hook never kills the long-running
    // tier).
    if (!ctx->enforce_limits) {
        return;
    }

    // Soft limit: one-shot WARN + a single GC step. lua_gc GCSTEP is
    // a bounded incremental step (not a full collection) so it does
    // not pause the worker for long.
    if (ctx->soft_limit_kb > 0 && memory_kb >= ctx->soft_limit_kb) {
        if (!ctx->soft_warned) {
            log_this(SR_LUA,
                     "Job %s exceeded soft memory limit (%zu KB, soft=%zu KB)",
                     LOG_LEVEL_ALERT, 3, ctx->job_id, memory_kb, ctx->soft_limit_kb);
            ctx->soft_warned = true;
        }
        lua_gc(L, LUA_GCSTEP, 0);
    }

    // Hard limit: abort the job. luaL_error does a longjmp to the
    // nearest lua_pcall (the worker's H_lua_run_string). SIZE_MAX is
    // the sentinel for "no hard limit" (set by the submit path when
    // the config default is 0).
    //
    // Note on formatting: lua_pushfstring (used by luaL_error) does
    // NOT support %zu. We build a static C string with snprintf
    // and pass it as a single %s argument.
    if (ctx->hard_limit_kb > 0
        && ctx->hard_limit_kb != SIZE_MAX
        && memory_kb >= ctx->hard_limit_kb) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "memory limit exceeded (job %s: %zu KB >= hard %zu KB)",
                 ctx->job_id, memory_kb, ctx->hard_limit_kb);
        luaL_error(L, "%s", msg);
        // Unreachable - luaL_error does not return.
    }
}

void H_lua_install_progress_hook(lua_State* L) {
    if (!L) {
        return;
    }
    H_lua_job_context* ctx = H_lua_get_job_context(L);
    if (!ctx || ctx->job_id[0] == '\0') {
        log_this(SR_LUA, "H_lua_install_progress_hook: no job context",
                 LOG_LEVEL_ERROR, 0);
        return;
    }
    if (ctx->hook_interval <= 0) {
        log_this(SR_LUA,
                 "H_lua_install_progress_hook: hook_interval<=0 for job %s; "
                 "hook not installed",
                 LOG_LEVEL_ERROR, 1, ctx->job_id);
        return;
    }

    // Reset the per-state progress counters so a state that is
    // re-used (Orchestrator long-lived tier, or a worker that reuses
    // a state for testing) starts fresh.
    ctx->local_instruction_count = 0;
    ctx->soft_warned = false;

    lua_sethook(L, H_lua_progress_hook_fn, LUA_MASKCOUNT, ctx->hook_interval);
}

void H_lua_uninstall_progress_hook(lua_State* L) {
    if (!L) {
        return;
    }
    lua_sethook(L, NULL, 0, 0);
}
