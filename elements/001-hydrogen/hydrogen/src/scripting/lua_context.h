/*
 * Scripting Subsystem - Lua Context Management
 *
 * Phase 3 of the LUA_PLAN. Provides creation, sandboxing, and destruction of
 * Lua states. Two-tier architecture (Phase 1):
 *   - Workers: fresh lua_State per job, destroyed on completion
 *   - Orchestrator: long-lived lua_State, compiled once at launch
 *
 * Both tiers go through the same create/destroy pair. The mmap-based
 * allocator (lua_mmap_alloc, originally from the database migration engine)
 * is used as cheap insurance against process-heap contamination.
 *
 * Sandbox policy is read from config->scripting.Sandbox (set in Phase 2b) so
 * a single source of truth governs how restricted a Lua context is.
 */

#ifndef HYDROGEN_SCRIPTING_LUA_CONTEXT_H
#define HYDROGEN_SCRIPTING_LUA_CONTEXT_H

// System headers
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Third-party headers
#include <lua.h>

// Project headers
#include <src/config/config.h>
#include <src/globals.h>   // ID_LEN

// Forward declaration to avoid pulling scoreboard.h into every Lua
// include path. scoreboard.h includes this header's companions but
// here we only need the pointer type.
struct Scoreboard;

/*
 * Create a sandboxed Lua state.
 *
 * Reads config->scripting.Sandbox to decide which standard libraries to
 * open. Returns NULL on failure (with a SR_LUA log entry).
 *
 * Ownership: caller must release the state with H_lua_destroy_context().
 */
lua_State* H_lua_create_context(void);

/*
 * Destroy a Lua state previously returned by H_lua_create_context.
 *
 * Safe to call with a NULL pointer. After this call, the pointer must not
 * be used again.
 */
void H_lua_destroy_context(lua_State* L);

/*
 * Install the H host table into a Lua state.
 *
 * Phase 3 only installs the empty H table with its sub-table placeholders
 * (H.log, H.system, H.query, H.altquery, H.authquery, H.http, H.llm,
 * H.mail, H.notify, H.sleep, H.shutdown_requested, H.set_current_state,
 * H.scoreboard, H.wait). Real functions are filled in by later phases
 * (Phase 6 onward). Until then, the placeholders let scripts reference
 * the surface without raising an error during early bring-up.
 */
void H_lua_install_api(lua_State* L);

/*
 * Load and execute a Lua chunk from a NUL-terminated string.
 *
 * Phase 4: the minimal wrapper that turns a string buffer into a
 * successful (or failed) chunk execution. Compiles the source with
 * luaL_loadbuffer and runs it with lua_pcall.
 *
 * Parameters:
 *   L    - a sandboxed Lua state created by H_lua_create_context.
 *          May be NULL, in which case the call is a no-op and
 *          LUA_ERRRUN is returned.
 *   code - the Lua source code as a NUL-terminated string. May not
 *          be NULL; a NULL is treated as a runtime error.
 *   name - a chunk name used in error messages (e.g. "[orchestrator]"
 *          or "[job:42]"). May be NULL, in which case Lua uses "?" .
 *
 * Returns:
 *   LUA_OK on success; on success, the chunk's return values are left
 *   on the Lua stack for the caller to consume (lua_gettop tells how
 *   many, lua_pop removes them).
 *
 *   On failure, returns a non-zero Lua status (LUA_ERRSYNTAX, LUA_ERRRUN,
 *   LUA_ERRMEM, ...) and leaves a single error string on top of the
 *   stack. The caller is expected to copy that string out of Lua-owned
 *   memory (UAF discipline) before touching the state again and to
 *   pop it with lua_pop before resuming other work.
 *
 * Ownership: the caller owns L and the resulting stack values. This
 * function neither creates nor destroys the state and does not call
 * lua_close on failure (the migration engine's per-call-state pattern
 * already handles that at the worker level).
 */
int H_lua_run_string(lua_State* L, const char* code, const char* name);

/*
 * Per-lua_State job context. Stored in the state's "extraspace" (a
 * pointer-sized slot tied 1:1 to the state by Lua 5.4) so the
 * progress hook (Phase 8) can identify which job is running without
 * touching the Lua registry or globals.
 *
 * Two-tier usage:
 *   - Workers: filled by scripting_worker_process_one() before
 *     H_lua_install_progress_hook(). The hook reads scoreboard and
 *     job_id to write progress and enforce limits.
 *   - Orchestrator: filled by Phase 11 launch code with
 *     enforce_limits = false so the hook never kills it.
 *
 * String fields are owned by the C caller (the scoreboard entry
 * already strdup's script_name, so copying the job_id into this
 * fixed-size buffer is UAF-safe across the state lifetime).
 *
 * soft_warned is a per-state one-shot so we don't spam the log on
 * every hook tick once memory exceeds the soft limit.
 */
typedef struct {
    char             job_id[ID_LEN + 1];   // empty string means "no job context"
    struct Scoreboard* scoreboard;
    int              hook_interval;        // copy of entry->instruction_hook_interval
    size_t           soft_limit_kb;        // copy of entry->memory_soft_limit_kb
    size_t           hard_limit_kb;        // copy of entry->memory_hard_limit_kb
    bool             enforce_limits;       // copy of entry->enforce_limits
    bool             soft_warned;          // one-shot soft-limit warning flag
    uint64_t         local_instruction_count;  // bumped in-hook; flushed to scoreboard on sample ticks

    // Phase 10: per-job runtime cap. Copied from entry->max_runtime_seconds
    // at submit time. Positive value = enforce; 0 or INT_MAX = "no limit".
    // The hook checks (now - started_at) >= max_runtime_seconds on every
    // tick and trips a kill when the deadline is past.
    int              max_runtime_seconds;

    // Phase 10: snapshot of the job's started_at timestamp. Copied from
    // the entry in the worker so the hook can compute elapsed time
    // without re-reading the scoreboard on every tick. CLOCK_MONOTONIC-
    // style timespec; zero means "not yet started" (the hook skips the
    // time check in that case to avoid a spurious kill on the first
    // tick after a fast PENDING->RUNNING transition).
    struct timespec  started_at;
} H_lua_job_context;

/*
 * Set the per-state job context. Pass a NULL pointer to clear.
 * The context is stored in the state's extraspace, so it follows
 * the state, not the calling thread.
 */
void H_lua_set_job_context(lua_State* L, const H_lua_job_context* ctx);

/*
 * Get a pointer to the per-state job context. The pointer is owned
 * by the state (it points into the extraspace). Returns NULL if the
 * state is NULL or no context has been set.
 */
H_lua_job_context* H_lua_get_job_context(lua_State* L);

/*
 * Phase 24: Generate a sanitized Lua traceback.
 *
 * Collects up to 10 frames from the Lua stack and returns a
 * malloc'd multi-line string. The caller owns the returned string
 * and must free it. Returns NULL on allocation failure.
 *
 * Sanitization:
 *   - At most 10 frames are captured
 *   - Source snippets limited to 80 chars
 *   - No absolute paths included
 */
char* H_lua_build_traceback(lua_State* L);

/*
 * Exposed for Unity tests (NOT part of the stable public API).
 */
void H_lua_open_sandboxed_libraries(lua_State* L);
int H_lua_panic(lua_State* L);

#endif /* HYDROGEN_SCRIPTING_LUA_CONTEXT_H */
