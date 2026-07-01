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

#endif /* HYDROGEN_SCRIPTING_LUA_CONTEXT_H */
