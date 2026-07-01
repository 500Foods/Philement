/*
 * Scripting Subsystem - Host API Implementation
 *
 * Phase 6 of the LUA_PLAN. Implements the C-side host functions exposed
 * to Lua via the H table.
 *
 *   H.log.{trace, debug, info, warn, error, fatal}
 *   H.system.{uptime, now, now_iso, instance_id, version}
 *
 * Each H.* function is a thin wrapper around an existing Hydrogen
 * primitive (log_this, time(NULL), the VERSION macro, etc.). Phase 6
 * does not introduce any new logging, timing, or identity machinery;
 * it just bridges the Lua C API to what is already there.
 *
 * The host table is installed by H_lua_install_api() in lua_context.c,
 * which calls the H_lua_install_log / H_lua_install_system functions
 * below to populate the (empty) H.log and H.system sub-tables that
 * were created during Phase 3.
 */

#ifndef HYDROGEN_SCRIPTING_SCRIPTING_API_H
#define HYDROGEN_SCRIPTING_SCRIPTING_API_H

// Third-party headers
#include <lua.h>

/*
 * Populate H.log with the six C functions that back
 * H.log.{trace, debug, info, warn, error, fatal}.
 *
 * Expects H and H.log to already exist on the Lua stack's globals.
 * Idempotent: re-calling replaces the existing fields.
 */
void H_lua_install_log(lua_State* L);

/*
 * Populate H.system with the C functions that back
 * H.system.{uptime, now, now_iso, instance_id, version}.
 *
 * Expects H and H.system to already exist on the Lua stack's globals.
 * Idempotent: re-calling replaces the existing fields.
 */
void H_lua_install_system(lua_State* L);

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_H */
