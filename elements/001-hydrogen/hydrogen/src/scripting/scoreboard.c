/*
 * Scripting Subsystem - Scoreboard
 *
 * Phase 5 of the LUA_PLAN. Thread-safe in-memory scoreboard.
 * Phase 8 added per-job resource limits and progress fields.
 * Phase 9 added the current_state field for H.set_current_state.
 * Phase 10 added max_runtime_seconds, kill_requested, and the
 * scoreboard_request_kill / scoreboard_is_kill_requested C API.
 * See scoreboard.h for the v1 surface and the design rationale
 * (mutex-protected array, copy-on-find, on-demand growth).
 *
 * ID generation reuses Hydrogen's 5-char ID generator
 * (generate_id in src/utils/utils_logging.c) to stay consistent with
 * the rest of the project. Collisions on a 12^5 ~ 248K-id space are
 * extremely unlikely in v1 (the scoreboard's lifetime is one process)
 * and submit() retries on a collision.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

// Local includes
#include "scoreboard.h"
#include <src/utils/utils_logging.h>   // generate_id

#define SCOREBOARD_INITIAL_CAPACITY 16

// Helper: is this status a terminal state?
static bool is_terminal_status(ScoreboardJobStatus status) {
    return status == SCOREBOARD_JOB_COMPLETED
        || status == SCOREBOARD_JOB_FAILED
        || status == SCOREBOARD_JOB_KILLED;
}

// Helper: free the strings owned by an entry and zero the struct.
// Does NOT free the entry pointer itself (caller owns the storage).
static void entry_clear_owned(ScoreboardEntry* entry) {
    if (!entry) {
        return;
    }
    if (entry->script_name) {
        free(entry->script_name);
        entry->script_name = NULL;
    }
    if (entry->params_json) {
        free(entry->params_json);
        entry->params_json = NULL;
    }
    // Phase 9: H.set_current_state report, if any.
    if (entry->current_state) {
        free(entry->current_state);
        entry->current_state = NULL;
    }
    // Phase 24: structured error info.
    if (entry->error_message) {
        free(entry->error_message);
        entry->error_message = NULL;
    }
    if (entry->error_traceback) {
        free(entry->error_traceback);
        entry->error_traceback = NULL;
    }
    // Phase 25: artifact metadata.
    if (entry->result_type) {
        free(entry->result_type);
        entry->result_type = NULL;
    }
    if (entry->result_location) {
        free(entry->result_location);
        entry->result_location = NULL;
    }
}

// Helper: zero a timespec to "not set".
static void timespec_clear(struct timespec* ts) {
    if (ts) {
        ts->tv_sec = 0;
        ts->tv_nsec = 0;
    }
}

// Helper: get current CLOCK_REALTIME.
static void timespec_now(struct timespec* ts) {
    if (!ts) {
        return;
    }
    clock_gettime(CLOCK_REALTIME, ts);
}

// Helper: append a new entry (already-populated entry) to the array.
// On capacity exhaustion, realloc doubles the capacity. Returns
// true on success, false on allocation failure.
static bool entries_grow_if_needed(Scoreboard* sb) {
    if (sb->count < sb->capacity) {
        return true;
    }
    size_t new_capacity = sb->capacity ? sb->capacity * 2 : SCOREBOARD_INITIAL_CAPACITY;
    ScoreboardEntry* new_entries = realloc(sb->entries, new_capacity * sizeof(ScoreboardEntry));
    if (!new_entries) {
        return false;
    }
    // Zero the new tail so any future find() of uninitialized slots
    // sees predictable contents.
    memset(new_entries + sb->capacity, 0,
           (new_capacity - sb->capacity) * sizeof(ScoreboardEntry));
    sb->entries = new_entries;
    sb->capacity = new_capacity;
    return true;
}

// Generate a fresh 5-char ID and check that no existing entry uses it.
// On a collision, retry up to a small bound, then return NULL so the
// caller sees "could not allocate" rather than blocking forever.
#define SCOREBOARD_ID_RETRY_LIMIT 8
static bool generate_unique_id(const Scoreboard* sb, char out[ID_LEN + 1]) {
    for (int attempt = 0; attempt < SCOREBOARD_ID_RETRY_LIMIT; attempt++) {
        generate_id(out, ID_LEN + 1);
        bool found = false;
        for (size_t i = 0; i < sb->count; i++) {
            if (strcmp(sb->entries[i].job_id, out) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return true;
        }
    }
    return false;
}

Scoreboard* scoreboard_create(void) {
    Scoreboard* sb = calloc(1, sizeof(Scoreboard));
    if (!sb) {
        return NULL;
    }
    sb->entries = NULL;
    sb->count = 0;
    sb->capacity = 0;
    if (pthread_mutex_init(&sb->mutex, NULL) != 0) {
        free(sb);
        return NULL;
    }
    return sb;
}

void scoreboard_destroy(Scoreboard* sb) {
    if (!sb) {
        return;
    }
    // Free any entry-owned strings; entries themselves are part of
    // the array and freed by free(entries) below.
    for (size_t i = 0; i < sb->count; i++) {
        entry_clear_owned(&sb->entries[i]);
    }
    free(sb->entries);
    sb->entries = NULL;
    sb->count = 0;
    sb->capacity = 0;
    pthread_mutex_destroy(&sb->mutex);
    free(sb);
}

char* scoreboard_submit_with_limits(Scoreboard* sb,
                                    const char* script_name,
                                    const char* params_json,
                                    const ScoreboardJobLimits* limits) {
    if (!sb) {
        return NULL;
    }
    if (!script_name) {
        log_this(SR_LUA, "scoreboard_submit_with_limits: NULL script_name",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    char* dup_script = strdup(script_name);
    if (!dup_script) {
        return NULL;
    }
    char* dup_params = NULL;
    if (params_json) {
        dup_params = strdup(params_json);
        if (!dup_params) {
            free(dup_script);
            return NULL;
        }
    }

    // Phase 8: resolve limits. We snapshot from app_config (if
    // available) at submit time so later config edits don't change a
    // running job's contract. The hook reads from the entry copy
    // (filled by scoreboard_find), not from app_config.
    //
    // Zero fields mean "use the config default" (or for hard_limit,
    // SIZE_MAX means "no limit"). Non-NULL limits with all-zero fields
    // is therefore identical to NULL.
    int    hook_interval = 0;
    size_t soft_kb = 0;
    size_t hard_kb = 0;
    bool   enforce = true;
    int    max_runtime = 0;
    if (limits) {
        hook_interval = limits->instruction_hook_interval;
        soft_kb = limits->memory_soft_limit_kb;
        hard_kb = limits->memory_hard_limit_kb;
        enforce = limits->enforce_limits;
        max_runtime = limits->max_runtime_seconds;
    }
    if (app_config) {
        if (hook_interval <= 0) {
            hook_interval = app_config->scripting.InstructionHookInterval;
        }
        if (soft_kb == 0) {
            soft_kb = (size_t)app_config->scripting.MemorySoftLimitKB;
        }
        if (hard_kb == 0) {
            int cfg_hard = app_config->scripting.MemoryHardLimitKB;
            hard_kb = (cfg_hard > 0) ? (size_t)cfg_hard : SIZE_MAX;
        }
        if (max_runtime == 0) {
            int cfg_runtime = app_config->scripting.DefaultMaxRuntime;
            max_runtime = (cfg_runtime > 0) ? cfg_runtime : INT_MAX;
        }
    } else {
        // No config: leave zero fields as zero. The hook handles
        // 0 soft / 0 hard by skipping the check (see lua_hook.c).
        // hook_interval was already left at 0 above if the caller
        // didn't provide a positive value. max_runtime stays 0,
        // which the hook treats as "no limit" (only positive values
        // are enforced).
    }

    pthread_mutex_lock(&sb->mutex);

    if (!entries_grow_if_needed(sb)) {
        pthread_mutex_unlock(&sb->mutex);
        free(dup_script);
        free(dup_params);
        return NULL;
    }

    char new_id[ID_LEN + 1];
    if (!generate_unique_id(sb, new_id)) {
        pthread_mutex_unlock(&sb->mutex);
        free(dup_script);
        free(dup_params);
        log_this(SR_LUA, "scoreboard_submit_with_limits: could not generate unique id after %d retries",
                 LOG_LEVEL_ERROR, 1, SCOREBOARD_ID_RETRY_LIMIT);
        return NULL;
    }

    ScoreboardEntry* entry = &sb->entries[sb->count];
    memset(entry, 0, sizeof(ScoreboardEntry));
    memcpy(entry->job_id, new_id, ID_LEN + 1);
    entry->script_name = dup_script;
    entry->params_json = dup_params;
    entry->status = SCOREBOARD_JOB_PENDING;
    timespec_now(&entry->created_at);
    timespec_clear(&entry->started_at);
    timespec_clear(&entry->finished_at);

    // Phase 8: per-job limits + initial progress = 0.
    entry->instruction_hook_interval = hook_interval;
    entry->memory_soft_limit_kb = soft_kb;
    entry->memory_hard_limit_kb = hard_kb;
    entry->enforce_limits = enforce;
    entry->instruction_count = 0;
    entry->memory_used_kb = 0;

    // Phase 10: per-job runtime cap + initial kill_requested = false.
    entry->max_runtime_seconds = max_runtime;
    entry->kill_requested = false;

    sb->count++;

    char* result = strdup(new_id);

    pthread_mutex_unlock(&sb->mutex);

    if (!result) {
        // We already inserted the entry under the lock; that's a
        // partial success but the caller asked for a return value
        // and we can't give them one. Roll back the entry so the
        // scoreboard doesn't grow a row the caller can't see.
        pthread_mutex_lock(&sb->mutex);
        entry_clear_owned(&sb->entries[sb->count - 1]);
        memset(&sb->entries[sb->count - 1], 0, sizeof(ScoreboardEntry));
        sb->count--;
        pthread_mutex_unlock(&sb->mutex);
        return NULL;
    }

    return result;
}

char* scoreboard_submit(Scoreboard* sb, const char* script_name, const char* params_json) {
    // Phase 8: limits=NULL means "config defaults, enforce_limits=true".
    return scoreboard_submit_with_limits(sb, script_name, params_json, NULL);
}

ScoreboardEntry* scoreboard_find(Scoreboard* sb, const char* job_id) {
    if (!sb || !job_id) {
        return NULL;
    }

    pthread_mutex_lock(&sb->mutex);
    ScoreboardEntry* match = NULL;
    for (size_t i = 0; i < sb->count; i++) {
        if (strcmp(sb->entries[i].job_id, job_id) == 0) {
            match = &sb->entries[i];
            break;
        }
    }

    ScoreboardEntry* copy = NULL;
    if (match) {
        copy = calloc(1, sizeof(ScoreboardEntry));
        if (copy) {
            *copy = *match;
            // The match's script_name / params_json / current_state
            // point at strings owned by the scoreboard. The copy must
            // own its own strings so the caller can free it
            // independently.
            copy->script_name = match->script_name ? strdup(match->script_name) : NULL;
            copy->params_json = match->params_json ? strdup(match->params_json) : NULL;
            copy->current_state = match->current_state ? strdup(match->current_state) : NULL;
            copy->error_message = match->error_message ? strdup(match->error_message) : NULL;
            copy->error_traceback = match->error_traceback ? strdup(match->error_traceback) : NULL;
            copy->result_type = match->result_type ? strdup(match->result_type) : NULL;
            copy->result_location = match->result_location ? strdup(match->result_location) : NULL;
            if ((match->script_name && !copy->script_name)
                || (match->params_json && !copy->params_json)
                || (match->current_state && !copy->current_state)
                || (match->error_message && !copy->error_message)
                || (match->error_traceback && !copy->error_traceback)
                || (match->result_type && !copy->result_type)
                || (match->result_location && !copy->result_location)) {
                // strdup failed; abandon the copy.
                entry_clear_owned(copy);
                free(copy);
                copy = NULL;
            }
        }
    }
    pthread_mutex_unlock(&sb->mutex);
    return copy;
}

void scoreboard_entry_free(ScoreboardEntry* entry) {
    if (!entry) {
        return;
    }
    entry_clear_owned(entry);
    free(entry);
}

bool scoreboard_update_status(Scoreboard* sb, const char* job_id, ScoreboardJobStatus new_status) {
    if (!sb || !job_id) {
        return false;
    }

    bool updated = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }

        ScoreboardJobStatus old_status = entry->status;
        if (old_status == new_status) {
            // Idempotent: same status, no timestamp change.
            updated = true;
            break;
        }

        entry->status = new_status;

        // Stamp started_at on the first transition into RUNNING.
        if (new_status == SCOREBOARD_JOB_RUNNING
            && old_status == SCOREBOARD_JOB_PENDING
            && entry->started_at.tv_sec == 0) {
            timespec_now(&entry->started_at);
        }

        // Stamp finished_at on the first transition into a terminal state.
        if (is_terminal_status(new_status)
            && !is_terminal_status(old_status)
            && entry->finished_at.tv_sec == 0) {
            timespec_now(&entry->finished_at);
        }

        updated = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return updated;
}

size_t scoreboard_count(Scoreboard* sb) {
    if (!sb) {
        return 0;
    }
    pthread_mutex_lock(&sb->mutex);
    size_t c = sb->count;
    pthread_mutex_unlock(&sb->mutex);
    return c;
}

bool scoreboard_update_progress(Scoreboard* sb, const char* job_id,
                                uint64_t instruction_count,
                                size_t memory_used_kb) {
    if (!sb || !job_id) {
        return false;
    }
    bool updated = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        // Monotonic: the hook is the only writer of these fields, but
        // belt-and-braces: never let progress go backwards.
        if (instruction_count > entry->instruction_count) {
            entry->instruction_count = instruction_count;
        }
        // Memory is non-monotonic (GC can free); record the latest sample.
        entry->memory_used_kb = memory_used_kb;
        updated = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return updated;
}

const char* scoreboard_status_name(ScoreboardJobStatus status) {
    switch (status) {
        case SCOREBOARD_JOB_PENDING:   return "pending";
        case SCOREBOARD_JOB_RUNNING:   return "running";
        case SCOREBOARD_JOB_COMPLETED: return "completed";
        case SCOREBOARD_JOB_FAILED:    return "failed";
        case SCOREBOARD_JOB_KILLED:    return "killed";
        default:                       return "unknown";
    }
}

bool scoreboard_update_current_state(Scoreboard* sb, const char* job_id, const char* state) {
    if (!sb || !job_id) {
        return false;
    }

    // Allocate the new string OUTSIDE the lock so the mutex critical
    // section stays short. NULL/empty state clears the field.
    char* new_state = NULL;
    if (state && state[0] != '\0') {
        new_state = strdup(state);
        if (!new_state) {
            log_this(SR_LUA, "scoreboard_update_current_state: strdup failed for job %s",
                     LOG_LEVEL_ERROR, 1, job_id);
            return false;
        }
    }

    bool updated = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        // Free any prior value, then install the new one (which may
        // be NULL to clear). The owned-string pattern is the same as
        // script_name / params_json: copy in, copy out.
        if (entry->current_state) {
            free(entry->current_state);
        }
        entry->current_state = new_state;
        updated = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);

    if (!updated) {
        // The id was unknown; free the string we allocated so it
        // doesn't leak.
        free(new_state);
    }
    return updated;
}

bool scoreboard_request_kill(Scoreboard* sb, const char* job_id) {
    if (!sb || !job_id) {
        return false;
    }
    bool found = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        // Idempotent: setting a flag that's already set is a no-op.
        entry->kill_requested = true;
        found = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return found;
}

bool scoreboard_is_kill_requested(Scoreboard* sb, const char* job_id, bool* out) {
    if (out) {
        *out = false;
    }
    if (!sb || !job_id || !out) {
        return false;
    }
    bool found = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) == 0) {
            *out = entry->kill_requested;
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&sb->mutex);
    return found;
}

/*
 * Snapshot all entries into a heap-allocated array of heap-allocated
 * copies. Phase 11 backs H.scoreboard.list() with this; the
 * Orchestrator and any future introspection code use it to see the
 * scoreboard at a point in time without holding the mutex.
 *
 * Empty scoreboard: returns true with *out_list = NULL and
 * *out_count = 0 (a valid snapshot, just empty).
 */
bool scoreboard_list(Scoreboard* sb,
                     ScoreboardEntry*** out_list,
                     size_t* out_count) {
    if (!out_list || !out_count) {
        return false;
    }
    *out_list = NULL;
    *out_count = 0;

    if (!sb) {
        // A NULL scoreboard is treated as an empty snapshot, not a
        // failure, so callers can use the same code path during
        // tests that don't allocate a real scoreboard.
        return true;
    }

    ScoreboardEntry** list = NULL;
    bool ok = false;
    pthread_mutex_lock(&sb->mutex);
    if (sb->count == 0) {
        // Empty: nothing to copy, no allocation needed.
        pthread_mutex_unlock(&sb->mutex);
        *out_list = NULL;
        *out_count = 0;
        return true;
    }
    list = calloc(sb->count, sizeof(ScoreboardEntry*));
    if (!list) {
        pthread_mutex_unlock(&sb->mutex);
        return false;
    }
    size_t made = 0;
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* copy = calloc(1, sizeof(ScoreboardEntry));
        if (!copy) {
            // Partial failure: free what we have so far so the
            // caller doesn't have to track how many were made.
            for (size_t j = 0; j < made; j++) {
                scoreboard_entry_free(list[j]);
            }
            free(list);
            pthread_mutex_unlock(&sb->mutex);
            return false;
        }
        *copy = sb->entries[i];
        // Strdup the owned strings, same as scoreboard_find, so the
        // caller can free each entry independently with
        // scoreboard_entry_free.
        copy->script_name = sb->entries[i].script_name
            ? strdup(sb->entries[i].script_name) : NULL;
        copy->params_json = sb->entries[i].params_json
            ? strdup(sb->entries[i].params_json) : NULL;
        copy->current_state = sb->entries[i].current_state
            ? strdup(sb->entries[i].current_state) : NULL;
        copy->error_message = sb->entries[i].error_message
            ? strdup(sb->entries[i].error_message) : NULL;
        copy->error_traceback = sb->entries[i].error_traceback
            ? strdup(sb->entries[i].error_traceback) : NULL;
        copy->result_type = sb->entries[i].result_type
            ? strdup(sb->entries[i].result_type) : NULL;
        copy->result_location = sb->entries[i].result_location
            ? strdup(sb->entries[i].result_location) : NULL;
        if ((sb->entries[i].script_name && !copy->script_name)
            || (sb->entries[i].params_json && !copy->params_json)
            || (sb->entries[i].current_state && !copy->current_state)
            || (sb->entries[i].error_message && !copy->error_message)
            || (sb->entries[i].error_traceback && !copy->error_traceback)
            || (sb->entries[i].result_type && !copy->result_type)
            || (sb->entries[i].result_location && !copy->result_location)) {
            // strdup failure: clean up this entry and everything
            // before it.
            scoreboard_entry_free(copy);
            for (size_t j = 0; j < made; j++) {
                scoreboard_entry_free(list[j]);
            }
            free(list);
            pthread_mutex_unlock(&sb->mutex);
            return false;
        }
        list[made++] = copy;
    }
    pthread_mutex_unlock(&sb->mutex);

    *out_list = list;
    *out_count = made;
    ok = true;
    return ok;
}

/*
 * Free an array of heap-allocated entry copies returned by
 * scoreboard_list. Frees each entry (via scoreboard_entry_free) and
 * then the array. Safe with NULL list or zero count.
 */
void scoreboard_list_free(ScoreboardEntry** list, size_t count) {
    if (!list) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        scoreboard_entry_free(list[i]);
    }
    free(list);
}

/*
 * Phase 12: attach a waiter to a job. See scoreboard.h for the
 * contract. POD pointers, so the critical section only sets three
 * fields; no allocations, no strdups.
 *
 * Idempotency: a second attach is a no-op (first writer wins). This
 * matches the submitter's typical pattern ("attach if not already
 * attached") and prevents a racing attach from clobbering a waiter
 * that is already in place.
 */
bool scoreboard_attach_waiter(Scoreboard* sb,
                             const char* job_id,
                             void* waiter_handle,
                             void* result_ref) {
    if (!sb || !job_id) {
        return false;
    }
    if (!waiter_handle && !result_ref) {
        // A "tag only" attach (no handle, no result) is not useful
        // and would surprise the worker (which would log a
        // "would signal waiter" marker with a NULL handle). Reject.
        return false;
    }

    bool found = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        if (!entry->has_waiter) {
            // First writer wins. We deliberately do not overwrite a
            // waiter that is already attached, even if the caller
            // passed different pointers - the submitter that raced
            // ahead is the authoritative owner of the waiter slot.
            entry->has_waiter = true;
            entry->waiter_handle = waiter_handle;
            entry->result_ref = result_ref;
            entry->waiter_signaled = false;
        }
        found = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return found;
}

/*
 * Phase 12: read the waiter fields. The scoreboard mutex is held
 * only long enough to copy three POD fields, so callers can read
 * the snapshot and then do their own blocking work (e.g. condvar
 * wait) without holding the scoreboard lock.
 */
bool scoreboard_get_waiter(Scoreboard* sb,
                           const char* job_id,
                           bool* out_has_waiter,
                           void** out_handle,
                           void** out_result) {
    if (out_has_waiter) {
        *out_has_waiter = false;
    }
    if (out_handle) {
        *out_handle = NULL;
    }
    if (out_result) {
        *out_result = NULL;
    }
    if (!sb || !job_id || !out_has_waiter) {
        return false;
    }

    bool found = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        *out_has_waiter = entry->has_waiter;
        if (out_handle) {
            *out_handle = entry->waiter_handle;
        }
        if (out_result) {
            *out_result = entry->result_ref;
        }
        found = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return found;
}

/*
 * Phase 12: claim the waiter for a one-shot completion signal.
 * Live scoreboard state is authoritative; workers must not rely on
 * an early scoreboard_find snapshot of has_waiter.
 */
bool scoreboard_claim_waiter(Scoreboard* sb,
                             const char* job_id,
                             void** out_handle,
                             void** out_result) {
    if (!sb || !job_id) {
        return false;
    }

    bool claimed = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        if (entry->has_waiter && !entry->waiter_signaled) {
            entry->waiter_signaled = true;
            if (out_handle) {
                *out_handle = entry->waiter_handle;
            }
            if (out_result) {
                *out_result = entry->result_ref;
            }
            claimed = true;
        }
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return claimed;
}

/*
 * Phase 12: clear the waiter fields. Use with care; in v1 there
 * is no caller that needs this (Phase 13's H_Handle lifecycle owns
 * its own cleanup), but the C API is in place for future REST
 * cancel-after-wait paths and for Unity tests that want to assert
 * the "no waiter" state directly.
 */
bool scoreboard_clear_waiter(Scoreboard* sb, const char* job_id) {
    if (!sb || !job_id) {
        return false;
    }
    bool found = false;
    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        entry->has_waiter = false;
        entry->waiter_handle = NULL;
        entry->result_ref = NULL;
        entry->waiter_signaled = false;
        found = true;
        break;
    }
    pthread_mutex_unlock(&sb->mutex);
    return found;
}

/*
 * Phase 24: store structured error info for a failed job.
 * Thread-safe. Strings are strdup'd into scoreboard-owned memory.
 */
bool scoreboard_update_error(Scoreboard* sb, const char* job_id,
                              const char* error_message,
                              const char* error_traceback) {
    if (!sb || !job_id) {
        return false;
    }

    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        // Free old strings before assigning new ones.
        if (entry->error_message) {
            free(entry->error_message);
            entry->error_message = NULL;
        }
        if (entry->error_traceback) {
            free(entry->error_traceback);
            entry->error_traceback = NULL;
        }
        // Only set if non-NULL, non-empty.
        if (error_message && error_message[0] != '\0') {
            entry->error_message = strdup(error_message);
            if (!entry->error_message) {
                pthread_mutex_unlock(&sb->mutex);
                return false;
            }
        }
        if (error_traceback && error_traceback[0] != '\0') {
            entry->error_traceback = strdup(error_traceback);
            if (!entry->error_traceback) {
                pthread_mutex_unlock(&sb->mutex);
                return false;
            }
        }
        pthread_mutex_unlock(&sb->mutex);
        return true;
    }
    pthread_mutex_unlock(&sb->mutex);
    return false;
}

/*
 * Phase 25: store artifact metadata for a completed job.
 * Thread-safe. Strings are strdup'd into scoreboard-owned memory.
 */
bool scoreboard_update_result(Scoreboard* sb, const char* job_id,
                              const char* result_type,
                              const char* result_location) {
    if (!sb || !job_id) {
        return false;
    }

    pthread_mutex_lock(&sb->mutex);
    for (size_t i = 0; i < sb->count; i++) {
        ScoreboardEntry* entry = &sb->entries[i];
        if (strcmp(entry->job_id, job_id) != 0) {
            continue;
        }
        // Free old strings before assigning new ones.
        if (entry->result_type) {
            free(entry->result_type);
            entry->result_type = NULL;
        }
        if (entry->result_location) {
            free(entry->result_location);
            entry->result_location = NULL;
        }
        // Only set if non-NULL, non-empty.
        if (result_type && result_type[0] != '\0') {
            entry->result_type = strdup(result_type);
            if (!entry->result_type) {
                pthread_mutex_unlock(&sb->mutex);
                return false;
            }
        }
        if (result_location && result_location[0] != '\0') {
            entry->result_location = strdup(result_location);
            if (!entry->result_location) {
                pthread_mutex_unlock(&sb->mutex);
                return false;
            }
        }
        pthread_mutex_unlock(&sb->mutex);
        return true;
    }
    pthread_mutex_unlock(&sb->mutex);
    return false;
}
