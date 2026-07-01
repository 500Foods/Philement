/*
 * Scripting Subsystem - Scoreboard
 *
 * Phase 5 of the LUA_PLAN. Thread-safe in-memory scoreboard.
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

char* scoreboard_submit(Scoreboard* sb, const char* script_name, const char* params_json) {
    if (!sb) {
        return NULL;
    }
    if (!script_name) {
        log_this(SR_LUA, "scoreboard_submit: NULL script_name", LOG_LEVEL_ERROR, 0);
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
        log_this(SR_LUA, "scoreboard_submit: could not generate unique id after %d retries",
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
            // The match's script_name / params_json point at strings
            // owned by the scoreboard. The copy must own its own
            // strings so the caller can free it independently.
            copy->script_name = match->script_name ? strdup(match->script_name) : NULL;
            copy->params_json = match->params_json ? strdup(match->params_json) : NULL;
            if ((match->script_name && !copy->script_name)
                || (match->params_json && !copy->params_json)) {
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
