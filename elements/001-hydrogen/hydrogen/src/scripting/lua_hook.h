/*
 * Scripting Subsystem - Progress Hook
 *
 * Phase 8 of the LUA_PLAN. Installs a lua_sethook-driven progress
 * hook on a lua_State that:
 *
 *   1. Bumps a per-state instruction counter on every Nth VM
 *      instruction (N = entry->instruction_hook_interval).
 *   2. Every MEMORY_SAMPLE_EVERY_N_HOOKS hook calls, samples Lua's
 *      GC memory usage with lua_gc(L, LUA_GCCOUNT, 0) and writes
 *      both counters to the job's scoreboard entry.
 *   3. If the job has enforce_limits=true and the sampled memory
 *      exceeds the soft limit, logs a one-shot warning and nudges
 *      the GC with lua_gc(L, LUA_GCSTEP, 0).
 *   4. If the sampled memory exceeds the hard limit, raises a Lua
 *      error via luaL_error, which longjmps to the worker's
 *      lua_pcall. The worker's existing FAILED path (worker_pool.c)
 *      copies the error out of Lua memory, logs it, marks the entry
 *      FAILED, and lua_close's the state.
 *
 * The hook reads its job_id, scoreboard pointer, and limits from the
 * per-state extraspace context (H_lua_job_context, defined in
 * lua_context.h). The caller MUST set that context with
 * H_lua_set_job_context() before calling H_lua_install_progress_hook().
 *
 * The hook is a leaf: it never calls H.* functions and never mutates
 * Lua state beyond a single GC step. It does not block on the
 * scoreboard for long - the critical section is just the two field
 * writes in scoreboard_update_progress.
 */
#ifndef HYDROGEN_SCRIPTING_LUA_HOOK_H
#define HYDROGEN_SCRIPTING_LUA_HOOK_H

// Third-party headers
#include <lua.h>

/*
 * Install the progress hook on a Lua state.
 *
 *   L - the state. Must have a job context already set via
 *       H_lua_set_job_context(). The hook interval is read from
 *       the context (which was populated from the scoreboard entry
 *       at submit time).
 *
 * No-op (with a log) if L is NULL, no context is set, or the
 * context's hook_interval is non-positive. Also no-op if the state
 * is NULL.
 */
void H_lua_install_progress_hook(lua_State* L);

/*
 * Remove the progress hook from a Lua state. Safe with a NULL
 * state. After this call, the state will not invoke the hook again
 * until/unless the hook is re-installed.
 */
void H_lua_uninstall_progress_hook(lua_State* L);

// Exposed for Unity tests (NOT part of the stable public API).
void H_lua_progress_hook_fn(lua_State* L, lua_Debug* ar);

#endif /* HYDROGEN_SCRIPTING_LUA_HOOK_H */
