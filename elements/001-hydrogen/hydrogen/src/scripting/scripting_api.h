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

// Local headers
#include "scripting_handle.h"

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
 * Populate H.set_result with the C function backing it.
 *
 * Phase 26: artifact metadata declaration from a Lua script. Updates
 * the result_type and result_location fields on the scoreboard entry
 * for the current job.
 *
 *   H.set_result(type, location)
 *       Sets the artifact metadata for the current job. `type` is a
 *       label for the artifact kind (e.g. "json", "file", "db-ref").
 *       `location` is a path, URI, or reference to retrieve the artifact.
 *       Either may be NULL or empty to clear the field.
 *
 * Available only inside a worker job context (i.e. when the per-state
 * H_lua_job_context has a non-empty job_id and a scoreboard). The
 * Orchestrator is a no-op caller: it has no scoreboard entry, and
 * the function silently returns 0.
 *
 * Argument validation: takes exactly two string arguments. Missing
 * or non-string arguments are logged at LOG_LEVEL_ERROR and do not
 * raise a Lua error - the host path is a leaf, in keeping with the
 * Phase 6 rule that the host log path should never crash the host.
 *
 * Replaces the Phase 3 placeholder sub-table of the same name.
 */
void H_lua_install_set_result(lua_State* L);

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

/*
 * Populate H.query, H.altquery, H.authquery, and H.wait with the
 * C functions backing them.
 *
 * Phase 13: Conduit-equivalent query functions. Async-first: each
 * query function returns an opaque H_Handle userdata immediately.
 * H.wait(handle1, handle2, ...) blocks until all handles are ready
 * and returns the results + errors.
 *
 *   H.query(sql, params, opts?) -> handle
 *       Submits `sql` to the configured scripting.DefaultDatabase.
 *       `params` is an optional Lua table of named parameters,
 *       converted to Hydrogen's typed JSON format. `opts.timeout`
 *       defaults to config->scripting.DefaultQueryTimeout.
 *
 *   H.altquery(database_name, sql, params, opts?) -> handle
 *       Same as H.query but the database name is explicit.
 *
 *   H.authquery(token, sql, params, opts?) -> handle
 *       Validates the JWT, extracts the database from claims, and
 *       submits the query to that database. The token is also
 *       checked for revocation.
 *
 *   H.query_sync / H.altquery_sync / H.authquery_sync
 *       Convenience wrappers: submit + wait in one call.
 *       Return (result, err) directly.
 *
 *   H.wait(handle1, handle2, ...) -> r1, r2, ..., rN, e1, e2, ..., eN
 *       For N handles, returns 2N values: N results (each nil on
 *       error), then N errors (each nil on success). Handles are
 *       consumed after wait.
 *
 * Result table shape (from data_json + QueryResult):
 *   {
 *     rows = { ... },          -- array of row tables, empty if error
 *     column_names = { ... },  -- array of column name strings (Phase 14)
 *     affected_rows = 0,       -- from the engine (Phase 14 verifies)
 *     elapsed_ms = 0,          -- from the engine (Phase 14)
 *   }
 */
void H_lua_install_query(lua_State* L);

/*
 * Build and push a result table onto the Lua stack.
 *
 * data_json      - JSON array string of rows (the data_json from
 *                  QueryResult / database_queue_await_result). NULL
 *                  or empty -> empty rows.
 * affected_rows  - The engine-reported affected-rows count (from
 *                  QueryResult.affected_rows, propagated through
 *                  DatabaseQuery.affected_rows by
 *                  database_queue_await_result). 0 for SELECTs.
 *
 * The pushed table has:
 *   {
 *     rows = { ... },         -- array of row tables (may be empty)
 *     affected_rows = N,      -- int (0 for SELECT, N for writes)
 *   }
 *
 * Always pushes exactly 1 value. On JSON parse failure the rows
 * field is an empty table and a log line is emitted (the host log
 * path is a leaf per Phase 6).
 *
 * Exposed (non-static) so it can be tested directly.
 */
int H_lua_build_result_table(lua_State* L, const char* data_json, int affected_rows);

/*
 * Populate H.http with the C functions backing H.http.get,
 * H.http.post, H.http.get_sync, and H.http.post_sync.
 *
 * Phase 16 of the LUA_PLAN. H.http.get(url, headers?, opts?) and
 * H.http.post(url, body?, headers?, opts?) return an opaque
 * H_Handle of kind H_HK_HTTP. H.wait(handle) blocks until the
 * HTTP call completes and pushes a result table
 * { status, headers, body, elapsed_ms }.
 *
 * The "always return a handle" contract from Phase 13 is
 * preserved: any error before the network call (missing url, alloc
 * failure) results in a handle whose `error` field is set, so
 * H.wait returns (nil, error) without raising.
 *
 * The H.http sub-table replaces the Phase 3 placeholder sub-table
 * of the same name. Future phases (17+) may add more functions
 * (e.g. H.http.request for fully custom requests, or an
 * async-multiplexer entry point).
 *
 * Called automatically by H_lua_install_query (which is the single
 * install point that wires every H.* function on a fresh
 * lua_State).
 */
void H_lua_install_http(lua_State* L);

/*
 * Populate H.llm with the C functions backing H.llm.call,
 * H.llm.list, H.llm.call_sync, and H.llm.list_sync.
 *
 * Phase 18 of the LUA_PLAN. Provides LLM invocation from Lua scripts.
 *
 *   H.llm.call(model, prompt, opts?) -> handle
 *       Asynchronously invoke an LLM by model name. `model` is the
 *       engine name from the ChatEngineCache. `prompt` is the user
 *       prompt string. `opts` is an optional table with:
 *         max_tokens (int, optional)
 *         temperature (float, optional)
 *         database (string, optional - uses DefaultDatabase if not set)
 *         timeout (int, optional - seconds)
 *
 *   H.llm.list() -> handle
 *       Returns a handle that resolves to a result table:
 *         { models = { { name, model, provider, is_default }, ... } }
 *
 *   H.llm.call_sync / H.llm.list_sync
 *       Convenience wrappers that submit + wait in one call.
 *
 * H.wait on an LLM handle returns:
 *   On success: { status, response, elapsed_ms }
 *   On error: nil + error string
 *
 * The model name is resolved via the ChatEngineCache in the
 * configured database. The actual HTTP call is made via the
 * chat proxy helpers (OpenAI/Anthropic/Ollama compatible APIs).
 *
 * Called automatically by H_lua_install_query (which is the single
 * install point that wires every H.* function on a fresh
 * lua_State).
 */
void H_lua_install_llm(lua_State* L);

int H_lua_llm_wait_one(lua_State* L, H_Handle* h);

/*
 * Populate H.mail and H.notify host functions.
 *
 * Phase 7A: H.mail.send queues templated mail via mailrelay_send_template.
 * H.notify returns a stable deferred error (no channel→template map yet).
 *
 * H.mail.send(message, opts?) -> handle
 * H.mail.send_sync(message, opts?) -> result, err
 * H.notify.send(message, opts?) -> handle
 * H.notify.send_sync(message, opts?) -> result, err
 *
 * message shape for H.mail.send (template-first):
 *   { template|template_key = string, to = string|array, cc?, bcc?,
 *     params = { [string] = string }?, idempotency_key?, priority? }
 * message shape for H.notify.send:
 *   { channel = string, to = string|array, body = string }
 *
 * Called automatically by H_lua_install_api.
 */
int H_lua_mail_send(lua_State* L);
int H_lua_mail_send_sync(lua_State* L);
int H_lua_notify_send(lua_State* L);
int H_lua_notify_send_sync(lua_State* L);
void H_lua_install_mail_notify(lua_State* L);

/*
 * Wait on a MAIL or NOTIFY handle.
 *
 * MAIL success: { message_id, status = "queued" }, nil
 * MAIL/NOTIFY error: nil, error_string
 */
int H_lua_mail_notify_wait_one(lua_State* L, H_Handle* h);

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_H */
