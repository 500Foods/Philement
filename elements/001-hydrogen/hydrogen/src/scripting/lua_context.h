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

// Third-party headers
#include <lua.h>

// Project headers
#include <src/config/config.h>

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

#endif /* HYDROGEN_SCRIPTING_LUA_CONTEXT_H */
