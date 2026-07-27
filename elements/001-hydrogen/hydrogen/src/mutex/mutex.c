/*
 * Mutex Utility Library Implementation
 *
 * Provides consistent, timeout-aware mutex operations with deadlock detection
 * and comprehensive logging for the entire Hydrogen codebase.
 */
#include <src/hydrogen.h>
#include "mutex.h"
#include <src/logging/logging.h>  // Relative include for src/mutex/ to src/logging/
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

// Global state for deadlock detection
static bool deadlock_detection_enabled = true;
static MutexLockAttempt* active_lock_attempts = NULL;
static size_t active_lock_count = 0;
static size_t active_lock_capacity = 0;
static pthread_mutex_t deadlock_detection_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global state for tracking locked mutexes
static MutexLockAttempt* locked_mutexes = NULL;
static size_t locked_mutex_count = 0;
static size_t locked_mutex_capacity = 0;

// Per-thread current lock identity for REL logging. Use __thread (no malloc)
// so LSAN never sees orphaned MutexId copies after thread exit / key_delete.
static __thread MutexId tls_mutex_op_id;
static __thread bool tls_mutex_op_id_set = false;
static __thread pthread_mutex_t *tls_mutex_op_ptr = NULL;

void free_mutex_id(void *ptr) {
    (void)ptr;
}

void init_mutex_tls_keys(void) {
}

MutexId* get_current_mutex_op_id(void) {
    return tls_mutex_op_id_set ? &tls_mutex_op_id : NULL;
}

void set_current_mutex_op_id(const MutexId *id) {
    if (id == NULL) {
        tls_mutex_op_id_set = false;
        return;
    }
    tls_mutex_op_id = *id;
    tls_mutex_op_id_set = true;
}

pthread_mutex_t* get_current_mutex_op_ptr(void) {
    return tls_mutex_op_ptr;
}

void set_current_mutex_op_ptr(pthread_mutex_t *ptr) {
    tls_mutex_op_ptr = ptr;
}

// Forward declarations
void detect_potential_deadlock(MutexId* current_id);

// Statistics
static MutexStats global_stats = {0};
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

bool mutex_bookkeeping_lock(pthread_mutex_t *m) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 50000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_mutex_timedlock(m, &ts) == 0;
}

void mutex_bookkeeping_unlock(pthread_mutex_t *m) {
    pthread_mutex_unlock(m);
}

/*
 * Core mutex lock with timeout and identification
 */
MutexResult mutex_lock_with_timeout(
    pthread_mutex_t* mutex,
    MutexId* id,
    int timeout_ms
) {
    if (!mutex || !id) {
        return MUTEX_ERROR;
    }

    // Convert milliseconds to timespec
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
    // Normalize nanoseconds
    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec += timeout.tv_nsec / 1000000000;
        timeout.tv_nsec %= 1000000000;
    }

    // Do NOT log MUTEX REQ here. log_this takes log_mutex via MUTEX_LOCK which
    // re-enters this function (stats/deadlock bookkeeping) and under load caused
    // system-wide hangs (test_41 ASAN). REL/EXP still log after the critical path.

    // Record lock attempt for deadlock detection
    if (deadlock_detection_enabled && mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
        if (active_lock_count >= active_lock_capacity) {
            size_t new_cap = active_lock_capacity == 0 ? 16 : active_lock_capacity * 2;
            MutexLockAttempt* new_attempts = realloc(active_lock_attempts,
                new_cap * sizeof(MutexLockAttempt));
            if (new_attempts) {
                active_lock_attempts = new_attempts;
                active_lock_capacity = new_cap;
            }
        }
        if (active_lock_count < active_lock_capacity) {
            active_lock_attempts[active_lock_count].id = *id;
            active_lock_attempts[active_lock_count].thread_id = pthread_self();
            active_lock_attempts[active_lock_count].attempt_start = time(NULL);
            active_lock_attempts[active_lock_count].is_write_lock = false;
            active_lock_count++;
        }
        mutex_bookkeeping_unlock(&deadlock_detection_mutex);
    }

    // Attempt to lock with timeout
    int result = pthread_mutex_timedlock(mutex, &timeout);

    // Remove from active attempts
    if (deadlock_detection_enabled && mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
        for (size_t i = 0; i < active_lock_count; i++) {
            if (pthread_equal(active_lock_attempts[i].thread_id, pthread_self()) &&
                strcmp(active_lock_attempts[i].id.name, id->name) == 0) {
                memmove(&active_lock_attempts[i], &active_lock_attempts[i + 1],
                    (active_lock_count - i - 1) * sizeof(MutexLockAttempt));
                active_lock_count--;
                break;
            }
        }
        mutex_bookkeeping_unlock(&deadlock_detection_mutex);
    }

    if (result == 0) {
        if (mutex_bookkeeping_lock(&stats_mutex)) {
            global_stats.total_locks++;
            mutex_bookkeeping_unlock(&stats_mutex);
        } else {
            __atomic_fetch_add(&global_stats.total_locks, 1, __ATOMIC_RELAXED);
        }
        if (deadlock_detection_enabled && mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
            if (locked_mutex_count >= locked_mutex_capacity) {
                size_t new_cap = locked_mutex_capacity == 0 ? 16 : locked_mutex_capacity * 2;
                MutexLockAttempt* new_locked = realloc(locked_mutexes,
                    new_cap * sizeof(MutexLockAttempt));
                if (new_locked) {
                    locked_mutexes = new_locked;
                    locked_mutex_capacity = new_cap;
                }
            }
            if (locked_mutex_count < locked_mutex_capacity) {
                locked_mutexes[locked_mutex_count].id = *id;
                locked_mutexes[locked_mutex_count].thread_id = pthread_self();
                locked_mutexes[locked_mutex_count].mutex_ptr = mutex;
                locked_mutex_count++;
            }
            mutex_bookkeeping_unlock(&deadlock_detection_mutex);
        }
        set_current_mutex_op_id(id);
        set_current_mutex_op_ptr(mutex);
        return MUTEX_SUCCESS;
    } else if (result == ETIMEDOUT) {
        if (mutex_bookkeeping_lock(&stats_mutex)) {
            global_stats.total_timeouts++;
            global_stats.last_timeout_time = time(NULL);
            mutex_bookkeeping_unlock(&stats_mutex);
        }
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX EXP: %X as %s in %s() [%s:%d] - timeout after %dms", LOG_LEVEL_ERROR, 6, (unsigned int)(uintptr_t)mutex, id->name, id->function, id->file, id->line, timeout_ms);
        }
        if (deadlock_detection_enabled) {
            detect_potential_deadlock(id);
        }
        return MUTEX_TIMEOUT;
    } else {
        if (mutex_bookkeeping_lock(&stats_mutex)) {
            global_stats.total_errors++;
            mutex_bookkeeping_unlock(&stats_mutex);
        }
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX ERR 1: %X as %s in %s() [%s:%d] - error %d (%s)", LOG_LEVEL_ERROR, 7, (unsigned int)(uintptr_t)mutex, id->name, id->function, id->file, id->line, result, strerror(result));
        }
        return MUTEX_ERROR;
    }
}

/*
 * Try to lock mutex without blocking
 */
MutexResult mutex_try_lock(
    pthread_mutex_t* mutex,
    MutexId* id
) {
    if (!mutex || !id) {
        return MUTEX_ERROR;
    }
    int result = pthread_mutex_trylock(mutex);
    if (result == 0) {
        set_current_mutex_op_id(id);
        set_current_mutex_op_ptr(mutex);
        return MUTEX_SUCCESS;
    } else if (result == EBUSY) {
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        return MUTEX_TIMEOUT; // Treat as timeout for consistency
    } else {
        // Log error (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX TRY: %X as %s in %s() [%s:%d] - error %d (%s)", LOG_LEVEL_ERROR, 6, (unsigned int)(uintptr_t)mutex, id->name, id->function, id->file, id->line, result, strerror(result));
        }
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        return MUTEX_ERROR;
    }
}

/*
 * Unlock mutex with error checking
 */
MutexResult mutex_unlock(pthread_mutex_t* mutex) {
    if (!mutex) {
        return MUTEX_ERROR;
    }
    int result = pthread_mutex_unlock(mutex);
    if (result == 0) {
        // DEBUG: Log unlock operation (only if not already in a logging operation to avoid circular dependency)
        MutexId *op_id = get_current_mutex_op_id();
        pthread_mutex_t *op_ptr = get_current_mutex_op_ptr();
        if (!log_is_in_logging_operation() && op_id && op_ptr == mutex) {
            log_this(op_id->subsystem, "MUTEX REL: %X as %s in %s()", LOG_LEVEL_TRACE, 3,
                     (unsigned int)(uintptr_t)mutex, op_id->name, op_id->function);
        }
        
        if (deadlock_detection_enabled && mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
            for (size_t i = 0; i < locked_mutex_count; i++) {
                if (locked_mutexes[i].mutex_ptr == mutex &&
                    pthread_equal(locked_mutexes[i].thread_id, pthread_self())) {
                    memmove(&locked_mutexes[i], &locked_mutexes[i + 1],
                        (locked_mutex_count - i - 1) * sizeof(MutexLockAttempt));
                    locked_mutex_count--;
                    break;
                }
            }
            mutex_bookkeeping_unlock(&deadlock_detection_mutex);
        }

        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        return MUTEX_SUCCESS;
    } else {
        if (!log_is_in_logging_operation()) {
            log_this(SR_MUTEXES, "MUTEX ERR 2: %X unlock failed - error %d (%s)", LOG_LEVEL_ERROR, 3, (unsigned int)(uintptr_t)mutex, result, strerror(result));
        }
        return MUTEX_ERROR;
    }
}

/*
 * Unlock mutex with identification for logging
 */
MutexResult mutex_unlock_with_id(pthread_mutex_t* mutex, MutexId* id) {
    if (!mutex) {
        return MUTEX_ERROR;
    }
    int result = pthread_mutex_unlock(mutex);
    if (result == 0) {
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX REL: %X as %s in %s()", LOG_LEVEL_TRACE, 3,
                     (unsigned int)(uintptr_t)mutex, id->name, id->function);
        }

        if (deadlock_detection_enabled && mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
            for (size_t i = 0; i < locked_mutex_count; i++) {
                if (locked_mutexes[i].mutex_ptr == mutex &&
                    pthread_equal(locked_mutexes[i].thread_id, pthread_self())) {
                    memmove(&locked_mutexes[i], &locked_mutexes[i + 1],
                        (locked_mutex_count - i - 1) * sizeof(MutexLockAttempt));
                    locked_mutex_count--;
                    break;
                }
            }
            mutex_bookkeeping_unlock(&deadlock_detection_mutex);
        }

        return MUTEX_SUCCESS;
    } else {
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX ERR 3: %X unlock failed - error %d (%s)", LOG_LEVEL_ERROR, 3, (unsigned int)(uintptr_t)mutex, result, strerror(result));
        }
        return MUTEX_ERROR;
    }
}

/*
 * Deadlock detection helper
 */
void detect_potential_deadlock(MutexId* current_id) {
    char peer_name[128];
    peer_name[0] = '\0';
    bool found = false;

    if (!current_id || !mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
        return;
    }
    for (size_t i = 0; i < active_lock_count; i++) {
        MutexLockAttempt* attempt = &active_lock_attempts[i];
        if (strcmp(attempt->id.subsystem, current_id->subsystem) == 0) {
            strncpy(peer_name, attempt->id.name, sizeof(peer_name) - 1);
            peer_name[sizeof(peer_name) - 1] = '\0';
            found = true;
            global_stats.total_deadlocks_detected++;
            global_stats.last_deadlock_time = time(NULL);
            break;
        }
    }
    mutex_bookkeeping_unlock(&deadlock_detection_mutex);

    if (found && !log_is_in_logging_operation()) {
        log_this(SR_MUTEXES, "DEADLOCK: Thread waiting for %s while we wait for %s", LOG_LEVEL_ERROR, 2, peer_name, current_id->name);
    }
}

/*
 * Deadlock detection control
 */
void mutex_enable_deadlock_detection(bool enable) {
    deadlock_detection_enabled = enable;
}

bool mutex_is_deadlock_detection_enabled(void) {
    return deadlock_detection_enabled;
}

/*
 * Log all currently active lock attempts
 */
void mutex_log_active_locks(void) {
    if (!mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
        return;
    }
    size_t count = active_lock_count;
    mutex_bookkeeping_unlock(&deadlock_detection_mutex);
    if (count == 0) {
        log_this(SR_MUTEXES, "No active mutex lock attempts", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(SR_MUTEXES, "Active mutex lock attempts: %zu", LOG_LEVEL_TRACE, 1, count);
    }
}

/*
 * Statistics functions
 */
void mutex_get_stats(MutexStats* stats) {
    if (!stats) return;
    if (mutex_bookkeeping_lock(&stats_mutex)) {
        *stats = global_stats;
        mutex_bookkeeping_unlock(&stats_mutex);
    } else {
        *stats = global_stats;
    }
}

void mutex_reset_stats(void) {
    if (mutex_bookkeeping_lock(&stats_mutex)) {
        memset(&global_stats, 0, sizeof(MutexStats));
        mutex_bookkeeping_unlock(&stats_mutex);
    }
}

/*
 * System initialization and cleanup
 */
bool mutex_system_init(void) {
    // Initialize TLS keys
    init_mutex_tls_keys();
    // Initialize deadlock detection structures
    active_lock_attempts = calloc(16, sizeof(MutexLockAttempt));
    if (!active_lock_attempts) {
        return false;
    }
    active_lock_capacity = 16;
    active_lock_count = 0;
    // Initialize locked mutex tracking structures
    locked_mutexes = calloc(16, sizeof(MutexLockAttempt));
    if (!locked_mutexes) {
        free(active_lock_attempts);
        return false;
    }
    locked_mutex_capacity = 16;
    locked_mutex_count = 0;
    // Reset statistics
    memset(&global_stats, 0, sizeof(MutexStats));
    log_this(SR_MUTEXES, "Mutex system initialized", LOG_LEVEL_TRACE, 0);
    return true;
}

void mutex_system_cleanup(void) {
    log_this(SR_MUTEXES, "Mutex system cleanup started", LOG_LEVEL_TRACE, 0);

    if (mutex_bookkeeping_lock(&deadlock_detection_mutex)) {
        free(active_lock_attempts);
        active_lock_attempts = NULL;
        active_lock_count = 0;
        active_lock_capacity = 0;
        free(locked_mutexes);
        locked_mutexes = NULL;
        locked_mutex_count = 0;
        locked_mutex_capacity = 0;
        mutex_bookkeeping_unlock(&deadlock_detection_mutex);
    }

    set_current_mutex_op_id(NULL);
    set_current_mutex_op_ptr(NULL);
}

/*
 * Utility functions
 */
const char* mutex_result_to_string(MutexResult result) {
    switch (result) {
        case MUTEX_SUCCESS: return "SUCCESS";
        case MUTEX_TIMEOUT: return "TIMEOUT";
        case MUTEX_DEADLOCK_DETECTED: return "DEADLOCK_DETECTED";
        case MUTEX_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void mutex_log_result(MutexResult result, MutexId* id, int timeout_ms) {
    if (!id) {
        // Log without id information if id is NULL
        if (result == MUTEX_SUCCESS) {
            log_this(SR_MUTEXES, "MUTEX ADD: Mutex locked (no id info)", LOG_LEVEL_TRACE, 0);
        } else {
            log_this(SR_MUTEXES, "MUTEX %s: Mutex operation failed (no id info)", LOG_LEVEL_ERROR, 1, mutex_result_to_string(result));
        }
        return;
    }
    if (result == MUTEX_SUCCESS) {
        log_this(id->subsystem, "MUTEX ADD: %s locked in %s() [%s:%d]", LOG_LEVEL_TRACE, 4, id->name, id->function, id->file, id->line);
    } else {
        log_this(id->subsystem, "MUTEX %s: %s in %s() [%s:%d] timeout=%dms", LOG_LEVEL_ERROR, 6, mutex_result_to_string(result), id->name, id->function, id->file, id->line, timeout_ms);
    }
}
