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
 * Phase 1) before the scoreboard is touched, so the Lua value can be
 * garbage-collected right after the call.
 *
 * Argument validation: takes exactly one string argument. A missing
 * or non-string argument is logged at LOG_LEVEL_ERROR and does not
 * raise a Lua error - the host path is a leaf, in keeping with the
 * Phase 6 rule that the host log path should never crash the host.
 *
 * Replaces the Phase 3 placeholder sub-table of the same name.
 */
void H_lua_install_set_current_state(lua_State* L);

/*
 * Populate H.sleep and H.shutdown_requested with the C functions
 * backing them.
 *
 * Phase 11: cooperative shutdown primitives for the Orchestrator
 * (and any future long-running tier-2 script).
 *
 *   H.sleep(milliseconds)
 *       Blocks the calling Lua state for at most `milliseconds` ms,
 *       polling the scripting shutdown flag every 100 ms. Returns
 *       early when the flag is set, so a long H.sleep(60000) call
 *       wakes up within ~100 ms of landing_scripting_subsystem
 *       flipping the flag. Returns nothing. Non-positive ms is
 *       treated as "no sleep" and returns immediately.
 *
 *   H.shutdown_requested() -> boolean
 *       Returns true if the subsystem's shutdown flag is set, false
 *       otherwise. The Orchestrator's scheduling loop is expected
 *       to check this on every iteration and exit cleanly.
 *
 * Both functions are top-level on H, not sub-tables. The Phase 3
 * placeholder sub-tables of the same names are replaced.
 */
void H_lua_install_sleep_shutdown(lua_State* L);

/*
 * Populate H.scoreboard with the C functions backing
 * H.scoreboard.{list, get, submit, cancel}.
 *
 * Phase 11: gives the Orchestrator (and any future Lua code with
 * access to the scoreboard) a way to read and write job entries.
 *
 *   H.scoreboard.list() -> table
 *       Array of all jobs in the scoreboard. Each job is a table
 *       with at least: job_id, script_name, status (string),
 *       params_json (or nil), created_at (epoch seconds), started_at
 *       (or nil), finished_at (or nil), instruction_count,
 *       memory_used_kb, current_state (or nil), max_runtime_seconds,
 *       kill_requested. Empty scoreboard returns an empty array.
 *
 *   H.scoreboard.get(job_id) -> job | nil
 *       A single job, in the same shape as list()[i], or nil if the
 *       id is unknown.
 *
 *   H.scoreboard.submit(entry) -> job_id | nil
 *       Submits a job for a worker to run. `entry` is a Lua table
 *       with `script_name` (string, required) and optional
 *       `params_json` (string). Returns the new 5-char job_id, or
 *       nil on failure (e.g. unknown script_name).
 *
 *   H.scoreboard.cancel(job_id) -> boolean
 *       Requests that a job be killed. Returns true if the id was
 *       found and the kill flag was set, false otherwise. Idempotent.
 *
 * The scoreboard being operated on is the subsystem's scoreboard
 * (scripting_scoreboard), or the per-state H_lua_job_context's
 * scoreboard for a worker job. For a bare lua_State (no scoreboard
 * in scope) the list/get/cancel functions return empty / nil /
 * false; submit returns nil. This matches the Phase 9
 * H.set_current_state no-op contract for "no scoreboard context".
 */
void H_lua_install_scoreboard(lua_State* L);

/*
 * Install a DB-backed searcher in `package.searchers` so Lua scripts
 * can use `require("group.script")` to load source from the `scripts`
 * table.
 *
 * Phase 11g: the searcher looks up `(group_name, script_name)` first
 * in the process-wide source cache (scripting_source_cache), then on
 * cache miss fetches the `code` column via QueryRef #087 using
 * config->scripting.DefaultDatabase. The fetched source is cached for
 * the process lifetime so repeated requires do not hit the DB.
 *
 * Gated by app_config->scripting.AllowDBModuleLoad (default false).
 * When disabled, this function is a no-op and `require` retains its
 * default behavior (preload + file-based searchers only).
 *
 * The searcher honors Lua's package.searchers contract: on success it
 * returns the compiled chunk function; on failure it returns an error
 * string so Lua continues with the next searcher.
 */
void H_lua_install_package(lua_State* L);

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_H */
