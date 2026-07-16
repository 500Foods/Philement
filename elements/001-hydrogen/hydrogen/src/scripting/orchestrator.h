/*
 * Scripting Subsystem - Orchestrator
 *
 * Phase 11d of the LUA_PLAN. The Orchestrator is the second tier of
 * the two-tier architecture: a long-lived Lua script that runs a
 * scheduling loop, polls the scoreboard, and submits work for the
 * worker pool. It is not a worker (it does not execute scoreboard
 * entries), and it is not a special long-running job (it is not an
 * entry in the scoreboard). It is the subsystem's own private
 * lua_State, loaded from a `(group_name, script_name)` row in the
 * `scripts` table.
 *
 * Threading model:
 *   - One dedicated pthread runs the orchestrator's lua_State. The
 *     pthread creates the state via H_lua_create_context, compiles
 *     the source with luaL_loadbufferx, and runs it with lua_pcall.
 *     The script is compiled exactly once.
 *   - The script typically runs an infinite H.sleep-driven loop.
 *     The loop polls H.shutdown_requested() to exit cleanly.
 *   - The pthread is joined in scripting_orchestrator_destroy(),
 *     which is called from land_scripting_subsystem() BEFORE
 *     scripting_workers_destroy() (so the orchestrator stops
 *     submitting new work before workers drain).
 *   - The pthread is registered with ServiceThreads as "orchestrator"
 *     so it shows up in thread/memory reporting alongside workers.
 *
 * Two public entry points:
 *   - scripting_orchestrator_start_with_source: test/REPL path that
 *     takes a Lua source string directly. Unit tests use this.
 *   - scripting_orchestrator_start_from_db: production path that
 *     looks up the configured (group_name, script_name) via
 *     QueryRef #087, then delegates to _with_source. Currently
 *     implements the QTC + database-queue plumbing; the actual
 *     sync DB call is the next deliverable.
 *
 * Idempotency: both start functions are no-ops the second time and
 * return true. scripting_orchestrator_destroy() is safe to call with
 * no orchestrator running, and is idempotent.
 */

#ifndef HYDROGEN_SCRIPTING_ORCHESTRATOR_H
#define HYDROGEN_SCRIPTING_ORCHESTRATOR_H

// System headers
#include <stdbool.h>

/*
 * Start the Orchestrator from a Lua source string. The source is
 * copied into a heap buffer owned by the orchestrator module and
 * freed when the orchestrator's pthread exits.
 *
 *   source - non-NULL Lua source. May be empty (a no-op script that
 *            returns immediately).
 *   name   - non-NULL human-readable name for log lines and the
 *            Lua chunk name in error messages. Typically the
 *            script_name portion of the `(group, name)` lookup key
 *            (e.g. "Orchestrator.lua").
 *
 * Returns true on a successful start. Returns false on any failure
 * (NULL args, allocation failure, H_lua_create_context failure,
 * pthread_create failure); in that case no orchestrator is running
 * and scripting_orchestrator_destroy() is a no-op.
 *
 * Idempotent: a second call while an orchestrator is already
 * running is a no-op and returns true.
 */
bool scripting_orchestrator_start_with_source(const char* source,
                                              const char* name);

/*
 * Fetch a script source from the `scripts` table via QueryRef #087.
 * Returns a heap-allocated copy of the `code` column, or NULL if the
 * row is missing or the query fails. Caller frees the returned string.
 *
 *   database        - non-NULL database name used to resolve the queue
 *   timeout_seconds - maximum time to wait for the query result
 *
 * Shared by scripting_orchestrator_start_from_db (Phase 11f) and the
 * DB-backed `require` searcher (Phase 11g).
 */
char* scripting_fetch_script_source(const char* group_name,
                                    const char* script_name,
                                    const char* database,
                                    int timeout_seconds);

/*
 * Start the Orchestrator by looking up the (group_name, script_name)
 * row in the `scripts` table via QueryRef #087, then delegating to
 * scripting_orchestrator_start_with_source with the loaded code.
 *
 *   group_name  - non-NULL group (e.g. "Orchestrators")
 *   script_name - non-NULL script name within the group
 *                 (e.g. "Orchestrator.lua")
 *
 * Returns true on a successful start. Returns false if the lookup
 * failed (no row, DB unavailable, QTC miss, allocation failure,
 * pthread_create failure).
 *
 * Idempotent: a second call while an orchestrator is already
 * running is a no-op and returns true (the existing instance is
 * kept, the new args are ignored).
 */
bool scripting_orchestrator_start_from_db(const char* group_name,
                                          const char* script_name);

/*
 * Parse config->scripting.Orchestrator (a "group/script" string) and
 * start the configured Orchestrator. Called by the server_ready
 * hook at both emission sites:
 *   - database_signal_ready_if_complete() in database.c
 *   - the no-DB path in launch.c
 *
 * No-op when scripting is disabled or the configured name is empty.
 * Logs a warning and returns false when the lookup fails; the
 * subsystem still launches and lands cleanly in that case.
 */
void scripting_orchestrator_load_configured(void);

/*
 * Signal the orchestrator to stop, join the pthread, and tear down
 * the lua_State. Safe to call with no orchestrator running
 * (no-op). Idempotent.
 *
 * Sets scripting_system_shutdown = 1 (the same flag H.sleep and
 * H.shutdown_requested poll) BEFORE joining, so a long H.sleep in
 * the orchestrator's loop wakes up within ~100 ms. After this call
 * the orchestrator's lua_State is destroyed and the module is reset
 * for a future start.
 */
void scripting_orchestrator_destroy(void);

/*
 * Returns true if an Orchestrator is currently running. Cheap read;
 * used by readiness checks and tests.
 */
bool scripting_orchestrator_is_running(void);

/*
 * Exposed for Unity tests (NOT part of the stable public API).
 */
void* orchestrator_thread_main(void* arg);
void orchestrator_set_shutdown_and_join(void);
void orchestrator_load_configured_blocking(void);
void* orchestrator_loader_main(void* arg);
const char* orchestrator_resolve_database(void);
char* orchestrator_build_params_json(const char* group_name,
                                     const char* script_name);
char* orchestrator_extract_code_from_result(const char* data_json);

#endif /* HYDROGEN_SCRIPTING_ORCHESTRATOR_H */
