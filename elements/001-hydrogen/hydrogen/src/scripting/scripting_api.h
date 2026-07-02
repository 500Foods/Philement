/*
 * Scripting Subsystem - Host API Implementation
 *
 * Phase 6 of the LUA_PLAN. Implements the C-side host functions exposed
 * to Lua via the H table.
 *
 *   H.log.{trace, debug, info, warn, error, fatal}
 *   H.system.{uptime, now, now_iso, instance_id, version}
 *   H.set_current_state                            (Phase 9)
 *   H.gc.{collect, step, count, isrunning}         (Phase 8)
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

/*
 * Populate H.gc with the C functions that back
 * H.gc.{collect, step, count, isrunning}.
 *
 * Phase 8: Lua-side explicit GC control. The progress hook only
 * SAMPLES memory (LUA_GCCOUNT); it never COLLECTS. Long-running
 * scripts (the Orchestrator) should call H.gc.collect() on their
 * own cadence in their scheduling loop. Workers don't need to -
 * lua_close at job end handles cleanup.
 *
 * H.gc.collect() is a potentially expensive operation: it blocks
 * the calling Lua state until a full mark-and-sweep cycle
 * completes. The hook deliberately does not call it.
 */
void H_lua_install_gc(lua_State* L);

/*
 * Populate H.set_current_state with the C function backing it.
 *
 * Phase 9: voluntary progress report from a Lua script. Updates the
 * current job's current_state field in the scoreboard. Available
 * only inside a worker job context (i.e. when the per-state
 * H_lua_job_context has a non-empty job_id and a scoreboard). The
 * Orchestrator is a no-op caller: it has no scoreboard entry, and
 * the function silently returns 0.
 *
 * The string is copied into a C-owned buffer (UAF discipline from
 * Phase 1) before the scoreboard is touched, so the Lua value can
 * be garbage-collected right after the call.
 *
 * Argument validation: takes exactly one string argument. A missing
 * or non-string argument is logged at LOG_LEVEL_ERROR and does not
 * raise a Lua error - the host path is a leaf, in keeping with the
 * Phase 6 rule that the host log path should never crash the host.
 *
 * Replaces the Phase 3 placeholder sub-table of the same name.
 */
void H_lua_install_set_current_state(lua_State* L);

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_H */
