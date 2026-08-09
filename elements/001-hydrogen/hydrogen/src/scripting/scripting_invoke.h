/*
 * Scripting Subsystem - Client invoke submit path (LUA_CLIENT Phase 2)
 *
 * Production entry used by REST /api/conduit/script: resolve a named
 * DB script (Group.Name), load source (cache + QueryRef), register on
 * the worker registry, enqueue a job, return job_id.
 *
 * Allowlist: only scripts.invokable rows load (QueryRef #149). Missing and
 * non-invokable both map to SCRIPTING_INVOKE_ERR_NOT_FOUND (HTTP 404).
 */

#ifndef HYDROGEN_SCRIPTING_SCRIPTING_INVOKE_H
#define HYDROGEN_SCRIPTING_SCRIPTING_INVOKE_H

#include <stdbool.h>

#include "scoreboard.h"

/*
 * Why submit_from_db failed (or NULL args). HTTP maps these later.
 * SCRIPTING_INVOKE_OK is only used when job_id_out is non-NULL.
 */
typedef enum ScriptingInvokeError {
    SCRIPTING_INVOKE_OK = 0,
    SCRIPTING_INVOKE_ERR_DISABLED,       /* Scripting.Enabled false or subsystem down */
    SCRIPTING_INVOKE_ERR_INVALID_NAME,   /* not Group.Name */
    SCRIPTING_INVOKE_ERR_NO_DATABASE,    /* no DefaultDatabase / single-DB fallback */
    SCRIPTING_INVOKE_ERR_NOT_FOUND,      /* no scripts row or empty code */
    SCRIPTING_INVOKE_ERR_DB_TIMEOUT,     /* fetch timed out / queue failure */
    SCRIPTING_INVOKE_ERR_SUBMIT_FAILED,  /* registry/scoreboard/enqueue failed */
    SCRIPTING_INVOKE_ERR_INTERNAL
} ScriptingInvokeError;

/*
 * Stable short codes for logs and future HTTP JSON "error_code".
 * Never NULL for known enum values.
 */
const char* scripting_invoke_error_name(ScriptingInvokeError err);

/*
 * Parse canonical client script id "Group.Name" into heap strings.
 * Returns true and sets *group_out / *script_out (caller frees both).
 * Rejects slash form, empty segments, missing dot.
 */
bool scripting_invoke_parse_script_name(const char* script_name,
                                        char** group_out,
                                        char** script_out);

/*
 * Load source for Group.Name for client REST: QueryRef #149
 * (invokable only) + optional cache put. Does not read source_cache
 * for the allowlist decision. Caller frees returned string on success.
 * On failure returns NULL and sets *err_out (NOT_FOUND for missing
 * or non-invokable).
 *
 *   fetch_timeout_seconds - passed to DB fetch; must be > 0
 */
char* scripting_invoke_load_source(const char* group_name,
                                   const char* script_name,
                                   int fetch_timeout_seconds,
                                   ScriptingInvokeError* err_out);

/*
 * Production REST submit path.
 *
 *   script_name  - "Group.Name" (required)
 *   params_json  - opaque JSON or NULL
 *   limits       - optional ScoreboardJobLimits (NULL = defaults)
 *   fetch_timeout_seconds - DB source fetch wait (use >= 1)
 *   job_id_out   - on OK, set to heap job_id (caller frees); else NULL
 *
 * Returns SCRIPTING_INVOKE_OK and non-NULL *job_id_out on success.
 */
ScriptingInvokeError scripting_submit_job_from_db(
    const char* script_name,
    const char* params_json,
    const ScoreboardJobLimits* limits,
    int fetch_timeout_seconds,
    char** job_id_out);

/*
 * Test seam: when non-NULL, scripting_invoke_load_source uses this
 * instead of DB/cache. Signature matches load (group, script, timeout,
 * err). Set NULL to restore production. Unity only.
 */
typedef char* (*scripting_invoke_load_source_fn)(const char* group_name,
                                                 const char* script_name,
                                                 int fetch_timeout_seconds,
                                                 ScriptingInvokeError* err_out);

void scripting_invoke_set_load_source_hook(scripting_invoke_load_source_fn fn);

/* --- LUA_CLIENT Phase 3: sync wait -------------------------------------- */

typedef enum ScriptingWaitResult {
    SCRIPTING_WAIT_COMPLETED = 0,
    SCRIPTING_WAIT_FAILED,
    SCRIPTING_WAIT_KILLED,
    SCRIPTING_WAIT_TIMEOUT,
    SCRIPTING_WAIT_NOT_FOUND,
    SCRIPTING_WAIT_SHUTDOWN,   /* subsystem landing mid-wait */
    SCRIPTING_WAIT_INTERNAL
} ScriptingWaitResult;

const char* scripting_wait_result_name(ScriptingWaitResult r);

/*
 * Block until job reaches a terminal status or timeout_seconds elapses.
 *
 *   job_id           - scoreboard id from submit
 *   timeout_seconds  - >0 wait budget; clamped behavior is caller's job
 *   out_entry        - on COMPLETED/FAILED/KILLED/TIMEOUT (if job still
 *                      exists), set to scoreboard_find copy (caller
 *                      scoreboard_entry_free). May be NULL to skip copy.
 *
 * On TIMEOUT: requests kill on the job (Phase 0); returns TIMEOUT even
 * if the job later completes (caller may GET). Uses timed poll of
 * scoreboard_find (condvar wake not required for correctness).
 *
 * Must re-read live scoreboard (never trust a pre-wait snapshot alone).
 */
ScriptingWaitResult scripting_wait_job(const char* job_id,
                                       int timeout_seconds,
                                       ScoreboardEntry** out_entry);

/* Exposed for Unity (non-static helpers). */
ScriptingWaitResult scripting_wait_result_from_status(ScoreboardJobStatus st);
bool scripting_wait_status_is_terminal(ScoreboardJobStatus st);

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_INVOKE_H */
