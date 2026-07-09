/*
 * Scripting Subsystem - Scoreboard
 *
 * Phase 5 of the LUA_PLAN. Thread-safe in-memory scoreboard that tracks
 * Lua jobs. The scoreboard is the single shared, mutex-protected
 * structure between the worker pool, the Orchestrator, and any REST
 * waiters (see LUA_PLAN_COMPLETE.md Risks section).
 *
 * v1 surface is intentionally minimal:
 *   - submit a job, getting a 5-char Hydrogen ID back
 *   - look up a job by ID (returns a heap copy; caller frees)
 *   - update a job's status (stamps started_at / finished_at)
 *   - count entries
 *   - create / destroy
 *
 * Fields added by later phases (Phase 8: instruction_count, memory;
 * Phase 9: current_state - IMPLEMENTED; Phase 10: max_runtime, kill
 * flag - IMPLEMENTED; Phase 12: has_waiter, waiter_handle, result_ref
 * - IMPLEMENTED; Phase 24: error_message, error_traceback - IMPLEMENTED;
 * Phase 25: result_type, result_location - IMPLEMENTED) are NOT in v1
 * unless called out. The struct reserves placeholder comments for the
 * not-yet-implemented ones so the growth path is obvious.
 *
 * Concurrency model:
 *   - One pthread_mutex_t protects the entries array.
 *   - find() returns a copy so callers can read fields without holding
 *     the mutex (and so the data survives a realloc inside submit()).
 *   - The mutex is held only for short critical sections: appending
 *     during submit, copying during find, status transitions during
 *     update. Operations that may block (DB, HTTP) are NOT done under
 *     the mutex - those happen against the copy returned by find().
 *
 * Phase 10 adds:
 *   - max_runtime_seconds on the entry (snapshotted at submit, default
 *     from config->scripting.DefaultMaxRuntime, INT_MAX means no limit).
 *   - kill_requested on the entry (mutable, defaults false; set via
 *     scoreboard_request_kill). The progress hook (lua_hook.c) polls
 *     this on every tick and trips the kill when the job is running.
 *   - scoreboard_request_kill / scoreboard_is_kill_requested C API.
 *
 * v1 does not have:
 *   - iteration (Phase 11's H.scoreboard.list) - IMPLEMENTED
 *   - waiting/condition variables (Phase 12) - PARTIALLY IMPLEMENTED
 *     (the scoreboard-side primitive is in place; the H_Handle / H.wait
 *     mechanism that consumes it is Phase 13)
 *   - persistence or restart-survival (out of scope for this plan)
 *   - capacity upper bound; the array grows on demand.
 */

#ifndef HYDROGEN_SCRIPTING_SCOREBOARD_H
#define HYDROGEN_SCRIPTING_SCOREBOARD_H

// System headers
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>   // INT_MAX (Phase 10: "no max runtime" sentinel)
#include <time.h>

// Project headers
#include <src/globals.h>   // ID_LEN, ID_CHARS

// Forward declaration; defined in this header below.
struct Scoreboard;

/*
 * Status values for a scoreboard entry. Ordered so the natural
 * progression is monotonic (PENDING -> RUNNING -> terminal), which
 * makes status comparisons meaningful.
 */
typedef enum {
    SCOREBOARD_JOB_PENDING   = 0,   // Submitted, not yet claimed by a worker
    SCOREBOARD_JOB_RUNNING   = 1,   // A worker has claimed and is executing
    SCOREBOARD_JOB_COMPLETED = 2,   // Worker finished successfully
    SCOREBOARD_JOB_FAILED    = 3,   // Worker reported an error
    SCOREBOARD_JOB_KILLED    = 4    // Killed (Phase 10) or shutdown-forced
} ScoreboardJobStatus;

/*
 * One job tracked by the scoreboard.
 *
 * String fields (script_name, params_json) are owned by the entry
 * (allocated with strdup on submit, freed on entry removal or on
 * scoreboard_destroy). Timestamps with zero tv_sec are "not set".
 *
 * Reserved for later phases (NOT populated by v1):
 *   - has_waiter, waiter_context, result_ref (Phase 12)
 *   - result_type, result_location           (Phase 25)
 */
typedef struct {
    char                job_id[ID_LEN + 1];
    char*               script_name;     // strdup'd; freed by entry destroy
    char*               params_json;     // strdup'd; freed by entry destroy; may be NULL
    ScoreboardJobStatus status;
    struct timespec     created_at;
    struct timespec     started_at;      // 0,0 until status -> RUNNING
    struct timespec     finished_at;     // 0,0 until status -> terminal

    // Phase 8: per-job resource limits (snapshotted at submit time so
    // later config edits don't change a running job's contract) and
    // progress (updated by the lua_sethook-driven progress hook).
    int                 instruction_hook_interval;  // frequency of hook ticks; 0 = use config default
    size_t              memory_soft_limit_kb;      // 0 = no soft limit / use config default
    size_t              memory_hard_limit_kb;      // 0 = no hard limit; LONG_MAX = use config default
    bool                enforce_limits;            // false for the Orchestrator (and any opt-out job)

    uint64_t            instruction_count;         // 0 until first hook tick
    size_t              memory_used_kb;            // 0 until first periodic GC sample

    // Phase 9: voluntary progress report from the Lua script. Set by
    // H.set_current_state. strdup'd; freed by entry destroy; NULL or
    // "" means "no progress report". Always safe to overwrite; the
    // previous value is freed before the new one is installed.
    char*               current_state;

    // Phase 10: per-job runtime cap. 0 = "use config default"
    // (config->scripting.DefaultMaxRuntime); INT_MAX = "no limit"
    // (the Orchestrator and any opt-out job). The hook reads this on
    // every tick and trips the kill when (now - started_at) >=
    // max_runtime_seconds.
    int                 max_runtime_seconds;

    // Phase 10: kill request flag. Set by scoreboard_request_kill;
    // read by the hook (via scoreboard_is_kill_requested) on every
    // tick. Once true, the next hook tick raises luaL_error, the
    // worker's pcall catches it, and the worker marks the job
    // KILLED (not FAILED). Idempotent: setting twice is a no-op.
    bool                kill_requested;

    // Phase 12: completion signaling. POD pointers; the scoreboard
    // does not own them. The intended (and only v1) consumer is the
    // H_Handle type that Phase 13 introduces; the worker calls the
    // H_Handle signal function with waiter_handle and writes the
    // final result into *result_ref. Until Phase 13 the worker only
    // logs a "would signal waiter" marker (see worker_pool.c).
    //
    //   has_waiter      - true once scoreboard_attach_waiter has run.
    //   waiter_handle   - opaque pointer to a waiter object. NULL when
    //                     no waiter is attached.
    //   result_ref      - opaque pointer to a result storage the C side
    //                     populates on completion. NULL when not in use.
    //   waiter_signaled - true once scoreboard_claim_waiter has handed
    //                     the waiter fields to a completion path. Prevents
    //                     double-signal if claim is invoked more than once.
    //
    // Concurrency: attach may race with the worker. The worker MUST
    // re-read waiter fields at terminal status via scoreboard_claim_waiter
    // (live scoreboard state), not from an early scoreboard_find snapshot
    // taken at job start. Callers that attach after the job is already
    // terminal will not receive a completion signal; they must check
    // status after attach and treat terminal as done (Phase 13 H.wait).
    // Subsequent scoreboard_attach_waiter calls are idempotent (first
    // writer wins). scoreboard_clear_waiter resets all waiter fields.
    bool                has_waiter;
    void*               waiter_handle;
    void*               result_ref;
    bool                waiter_signaled;

    // Phase 24: structured error info. strdup'd; owned; freed on
    // entry destroy. NULL for jobs that succeeded or haven't run yet.
    // error_message - the Lua error string (e.g. "attempt to index a nil value")
    // error_traceback - sanitized Lua traceback (multi-line string, limited frames)
    char*               error_message;
    char*               error_traceback;

    // Phase 25: artifact metadata. strdup'd; owned; freed on entry destroy.
    // NULL or empty means "no artifact".
    // result_type - a label for the artifact kind (e.g. "json", "file", "db-ref")
    // result_location - a path, URI, or reference to retrieve the artifact
    char*               result_type;
    char*               result_location;
} ScoreboardEntry;

/*
 * The scoreboard itself.
 *
 * v1 has a single growable array, protected by a single mutex. List
 * operations (Phase 11) and waiter condvars (Phase 12) are additive
 * and do not change this layout.
 */
typedef struct Scoreboard {
    ScoreboardEntry*    entries;
    size_t              count;
    size_t              capacity;
    pthread_mutex_t     mutex;
} Scoreboard;

/*
 * Create a new, empty scoreboard. Returns NULL on allocation failure.
 * Destroy with scoreboard_destroy().
 */
Scoreboard* scoreboard_create(void);

/*
 * Free a scoreboard and all of its entries. Safe to call with NULL.
 * Idempotent (calling twice is a no-op the second time).
 */
void scoreboard_destroy(Scoreboard* sb);

/*
 * Per-job resource limits. Snapshotted from the scoreboard entry into
 * the per-lua_State extraspace context by the worker, so the hook
 * reads a stable contract even if the global config changes mid-job.
 *
 * A zero value means "use the corresponding config default" (filled in
 * by scoreboard_submit_with_limits at submit time, not by the hook).
 *
 * enforce_limits is a per-job opt-out: false means the hook will still
 * update instruction_count and memory_used_kb for observability, but
 * will never warn or abort the job. This is how the Orchestrator (and
 * any future long-running tier-2 script) keeps the same observability
 * path without risking being killed by its own growth.
 */
typedef struct {
    int    instruction_hook_interval;  // 0 = use config default
    size_t memory_soft_limit_kb;       // 0 = use config default
    size_t memory_hard_limit_kb;       // 0 = use config default; SIZE_MAX explicitly = no limit
    bool   enforce_limits;             // default true
    int    max_runtime_seconds;        // 0 = use config default; INT_MAX explicitly = no limit
} ScoreboardJobLimits;

/*
 * Append a new job to the scoreboard.
 *
 *   sb          - the scoreboard (non-NULL)
 *   script_name - non-NULL script identifier (e.g. "orchestrator.submit")
 *   params_json - opaque JSON string for the job's parameters, or NULL.
 *                 Ownership: scoreboard strdup's the string, so the
 *                 caller may free its own copy after the call returns.
 *
 * Returns a pointer to a heap-allocated C string containing the new
 * job_id (5-char Hydrogen ID). The caller is responsible for free()'ing
 * the returned string. Returns NULL on allocation failure.
 *
 * Concurrency: thread-safe; the entries array is protected by sb->mutex.
 * The capacity grows on demand (realloc) when full.
 *
 * Phase 8: this is a thin wrapper around scoreboard_submit_with_limits
 * passing limits=NULL (which resolves all zero fields to config defaults
 * and sets enforce_limits=true).
 */
char* scoreboard_submit(Scoreboard* sb, const char* script_name, const char* params_json);

/*
 * Submit with explicit per-job resource limits. Limits==NULL is
 * equivalent to scoreboard_submit() (config defaults, enforce_limits=true).
 *
 * A zero field in `limits` means "use the config default". A field set
 * to SIZE_MAX for memory_hard_limit_kb means "no hard limit". A
 * non-NULL limits pointer with all fields zero is therefore identical
 * to NULL.
 */
char* scoreboard_submit_with_limits(Scoreboard* sb,
                                    const char* script_name,
                                    const char* params_json,
                                    const ScoreboardJobLimits* limits);

/*
 * Look up a job by its 5-char ID.
 *
 * Returns a heap-allocated copy of the entry (caller frees with
 * scoreboard_entry_free). Returns NULL if the ID is unknown or if
 * memory cannot be allocated.
 *
 * The copy is independent of the scoreboard - subsequent submit,
 * update_status, or destroy calls do not invalidate it.
 */
ScoreboardEntry* scoreboard_find(Scoreboard* sb, const char* job_id);

/*
 * Free a ScoreboardEntry returned by scoreboard_find. Safe with NULL.
 */
void scoreboard_entry_free(ScoreboardEntry* entry);

/*
 * Update the status of a job.
 *
 *   sb        - the scoreboard
 *   job_id    - the 5-char ID to update
 *   new_status - the new status
 *
 * Returns true if a matching entry was found and updated. Returns
 * false if the ID is unknown (no change made).
 *
 * Side effects on timestamps:
 *   - Any transition *into* RUNNING (from PENDING) stamps started_at
 *     to "now".
 *   - Any transition into a terminal state (COMPLETED, FAILED, KILLED)
 *     stamps finished_at to "now".
 *   - Idempotent: re-setting the same status is a no-op (returns true
 *     but does not re-stamp timestamps).
 */
bool scoreboard_update_status(Scoreboard* sb, const char* job_id, ScoreboardJobStatus new_status);

/*
 * Number of entries currently in the scoreboard. Thread-safe (briefly
 * takes the mutex). Returns 0 for a NULL scoreboard.
 */
size_t scoreboard_count(Scoreboard* sb);

/*
 * Human-readable name for a status value (for logging). Returns
 * "unknown" for out-of-range values; never returns NULL.
 */
const char* scoreboard_status_name(ScoreboardJobStatus status);

/*
 * Update the per-job progress fields (instruction_count, memory_used_kb).
 * Thread-safe. Returns true if a matching entry was found and updated,
 * false if the ID is unknown or memory could not be allocated.
 *
 * This is a pure data update: it does NOT change the job's status and
 * does NOT stamp started_at/finished_at. It is called from the
 * lua_sethook-driven progress hook (Phase 8) on every memory sample
 * tick, so it must be cheap and lock-bounded.
 */
bool scoreboard_update_progress(Scoreboard* sb, const char* job_id,
                                uint64_t instruction_count,
                                size_t memory_used_kb);

/*
 * Update the voluntary progress report (current_state). Thread-safe.
 *
 *   sb     - the scoreboard
 *   job_id - the 5-char ID to update
 *   state  - the new state string (strdup'd by the scoreboard).
 *           NULL or "" clears the field. The caller may free its own
 *           copy after the call returns.
 *
 * Returns true if a matching entry was found and updated, false if the
 * ID is unknown or memory could not be allocated.
 *
 * Phase 9: this is the backing store for H.set_current_state. Any
 * prior value of current_state is freed before the new one is set.
 * The update is data-only: it does NOT change the job's status and
 * does NOT stamp started_at/finished_at.
 */
bool scoreboard_update_current_state(Scoreboard* sb, const char* job_id, const char* state);

/*
 * Request that a job be killed. Thread-safe.
 *
 *   sb     - the scoreboard
 *   job_id - the 5-char ID to mark for kill
 *
 * Returns true if a matching entry was found and its kill_requested
 * flag was set (or was already set); false if the ID is unknown.
 *
 * Idempotent: setting kill_requested=true on an entry that is already
 * kill_requested is a no-op and still returns true.
 *
 * Effect on running jobs: the lua_sethook-driven progress hook polls
 * scoreboard_is_kill_requested on every tick. When the flag flips to
 * true, the next tick raises luaL_error, the worker's pcall catches
 * it, and the worker marks the job KILLED (not FAILED). Kill latency
 * is therefore bounded by the per-state hook_interval (default 5,000
 * VM instructions, a few microseconds at typical run rates).
 *
 * Effect on PENDING jobs: workers that dequeue a PENDING entry with
 * kill_requested=true skip the Lua execution entirely and mark the
 * entry KILLED immediately. This avoids burning a lua_State on a job
 * nobody wants.
 *
 * Effect on terminal jobs: setting kill_requested on a job that has
 * already reached COMPLETED/FAILED/KILLED is harmless (the hook
 * only runs while the state is RUNNING). Returns true if the entry
 * exists, regardless of its current status.
 *
 * Phase 10: this is the C API. The Lua-side wrapper H.scoreboard.cancel
 * is added in Phase 11.
 */
bool scoreboard_request_kill(Scoreboard* sb, const char* job_id);

/*
 * Read the current kill_requested flag for a job. Thread-safe.
 *
 *   sb     - the scoreboard
 *   job_id - the 5-char ID to look up
 *   out    - non-NULL; *out is set to true if the entry exists and
 *            has kill_requested=true, false otherwise.
 *
 * Returns true if a matching entry was found; false if the ID is
 * unknown (in which case *out is left as false). The hook calls
 * this on every tick so the kill flag is always read live from the
 * scoreboard (no risk of snapshotting a stale value).
 */
bool scoreboard_is_kill_requested(Scoreboard* sb, const char* job_id, bool* out);

/*
 * Snapshot the scoreboard's entries into a heap-allocated array of
 * heap-allocated copies. The caller is responsible for freeing the
 * array with scoreboard_list_free.
 *
 *   sb        - the scoreboard
 *   out_list  - non-NULL; *out_list is set to a heap-allocated array
 *               of *out_count heap-allocated ScoreboardEntry copies.
 *               Set to NULL on failure or when the scoreboard is empty.
 *   out_count - non-NULL; *out_count is set to the number of entries
 *               in the returned array.
 *
 * Returns true on success, false on allocation failure.
 *
 * Thread-safety: briefly takes the scoreboard's mutex, copies all
 * entries, releases. New entries that arrive after the snapshot are
 * not in the result (this is a snapshot, not a live view).
 *
 * Phase 11: backs H.scoreboard.list(). The Orchestrator calls this
 * each tick to decide what to do next. The returned array is a
 * plain array, not a live view; iterating it does not block the
 * scoreboard.
 */
bool scoreboard_list(Scoreboard* sb,
                     ScoreboardEntry*** out_list,
                     size_t* out_count);

/*
 * Free an array returned by scoreboard_list. Safe to call with NULL.
 * Frees each entry via scoreboard_entry_free and then the array
 * itself; idempotent (NULL list or zero count is a no-op).
 */
void scoreboard_list_free(ScoreboardEntry** list, size_t count);

/*
 * Phase 12: attach a waiter to a job.
 *
 *   sb            - the scoreboard
 *   job_id        - the 5-char ID to attach to
 *   waiter_handle - opaque pointer to a waiter object. In Phase 13
 *                   this will be an H_Handle* (the C-side object
 *                   that backs H.wait). For Phase 12 it is just
 *                   stored verbatim and is not dereferenced by the
 *                   scoreboard. May be NULL only if result_ref is
 *                   also NULL (a "tag only" attach is not useful
 *                   and is rejected).
 *   result_ref    - opaque pointer to a result storage the C side
 *                   populates on completion. May be NULL (e.g. a
 *                   waiter that only cares about completion and not
 *                   the result).
 *
 * Returns true if a matching entry was found and the waiter was
 * attached (or was already attached). Returns false if the ID is
 * unknown, if both pointers are NULL, or if memory could not be
 * allocated.
 *
 * Concurrency: thread-safe (briefly takes the scoreboard mutex).
 * Idempotent: a second attach with the same or different pointers
 * is a no-op and still returns true; the first attach wins. This
 * is the right semantic for a submitter that wants to ensure the
 * waiter is in place even if another thread raced ahead.
 *
 * Phase 12 sets only the C-side scoreboard primitive; the actual
 * wake-up is performed by the worker (worker_pool.c) once the job
 * reaches a terminal status. Phase 13 plugs the H_Handle signal
 * into that worker hook.
 */
bool scoreboard_attach_waiter(Scoreboard* sb,
                             const char* job_id,
                             void* waiter_handle,
                             void* result_ref);

/*
 * Read the current waiter fields for a job. Thread-safe.
 *
 *   sb            - the scoreboard
 *   job_id        - the 5-char ID to look up
 *   out_has_waiter- non-NULL; *out_has_waiter is set to the entry's
 *                   has_waiter flag.
 *   out_handle    - may be NULL; if non-NULL, *out_handle is set to
 *                   the entry's waiter_handle (or NULL if none).
 *   out_result    - may be NULL; if non-NULL, *out_result is set to
 *                   the entry's result_ref (or NULL if none).
 *
 * Returns true if a matching entry was found; false if the ID is
 * unknown (in which case *out_has_waiter is left as false and the
 * out_handle / out_result outputs are left as NULL).
 *
 * The returned pointers are owned by the scoreboard; the caller
 * must not free them. They remain valid until the entry is removed
 * (which currently never happens - the scoreboard is append-only -
 * but a future phase may add removal).
 */
bool scoreboard_get_waiter(Scoreboard* sb,
                           const char* job_id,
                           bool* out_has_waiter,
                           void** out_handle,
                           void** out_result);

/*
 * Atomically claim an attached waiter for completion signaling.
 * Thread-safe.
 *
 *   sb         - the scoreboard
 *   job_id     - the 5-char ID to look up
 *   out_handle - may be NULL; if non-NULL and claim succeeds, set to
 *                the entry's waiter_handle (may itself be NULL).
 *   out_result - may be NULL; if non-NULL and claim succeeds, set to
 *                the entry's result_ref (may itself be NULL).
 *
 * Returns true exactly once per attach lifecycle when has_waiter is
 * true and the waiter has not already been claimed. Returns false if
 * the ID is unknown, no waiter is attached, or the waiter was already
 * claimed. On false, out_handle / out_result are left unchanged when
 * non-NULL (callers should initialize them).
 *
 * The worker calls this after marking a job terminal so a waiter
 * attached after job start (but before claim) is still observed.
 * Phase 13 will use the claimed pointers for the real H_Handle signal.
 */
bool scoreboard_claim_waiter(Scoreboard* sb,
                             const char* job_id,
                             void** out_handle,
                             void** out_result);

/*
 * Clear the waiter fields for a job. Thread-safe.
 *
 *   sb     - the scoreboard
 *   job_id - the 5-char ID to clear
 *
 * Returns true if a matching entry was found (the fields are
 * cleared even if they were already clear). Returns false if the
 * ID is unknown.
 *
 * Effect: sets has_waiter=false, waiter_handle=NULL, result_ref=NULL,
 * waiter_signaled=false. After this call the worker's completion path
 * will see no waiter to signal. Use with care - a waiter that was
 * expecting a signal will block until its own timeout.
 */
bool scoreboard_clear_waiter(Scoreboard* sb, const char* job_id);

/*
 * Phase 24: store structured error info for a failed job.
 *
 *   sb            - the scoreboard
 *   job_id        - the 5-char ID to update
 *   error_message - the Lua error string (strdup'd by the scoreboard).
 *                   May be NULL to clear.
 *   error_traceback - the Lua traceback string (strdup'd by the scoreboard).
 *                     May be NULL to clear.
 *
 * Returns true if a matching entry was found and updated, false if the
 * ID is unknown or memory allocation failed.
 *
 * Thread-safe. The strings are copied into scoreboard-owned memory.
 * Either string may be NULL (to clear the field), but not both NULL.
 */
bool scoreboard_update_error(Scoreboard* sb, const char* job_id,
                              const char* error_message,
                              const char* error_traceback);

/*
 * Phase 25: store artifact metadata for a completed job.
 *
 *   sb            - the scoreboard
 *   job_id        - the 5-char ID to update
 *   result_type   - a label for the artifact kind (e.g. "json", "file", "db-ref").
 *                   May be NULL or empty to clear.
 *   result_location - a path, URI, or reference to retrieve the artifact.
 *                   May be NULL or empty to clear.
 *
 * Returns true if a matching entry was found and updated, false if the
 * ID is unknown or memory allocation failed.
 *
 * Thread-safe. The strings are copied into scoreboard-owned memory.
 * Either parameter may be NULL or empty (to clear the field).
 *
 * This is a data-only update: it does NOT change the job's status and
 * does NOT stamp started_at/finished_at. This matches the contract of
 * scoreboard_update_progress and scoreboard_update_current_state.
 */
bool scoreboard_update_result(Scoreboard* sb, const char* job_id,
                              const char* result_type,
                              const char* result_location);

#endif /* HYDROGEN_SCRIPTING_SCOREBOARD_H */
