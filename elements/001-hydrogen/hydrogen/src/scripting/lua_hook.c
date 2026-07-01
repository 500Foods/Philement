/*
 * Scripting Subsystem - Progress Hook
 *
 * Phase 8 of the LUA_PLAN. See lua_hook.h for the design.
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
 *     mark the scoreboard FAILED, and lua_close the state.
 *
 *   - The hook is uninstalled by H_lua_uninstall_progress_hook.
 *     Workers call this between jobs and on shutdown.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdint.h>

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

    // Enforce limits only when the job opted in (workers do; the
    // Orchestrator does not, so the hook never kills the long-running
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
