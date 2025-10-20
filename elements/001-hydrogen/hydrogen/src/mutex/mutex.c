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

// Thread-local storage for current mutex operation (now via keys)
static pthread_key_t mutex_op_id_key;
static pthread_key_t mutex_op_ptr_key;
static bool mutex_tls_keys_initialized = false;

// Destructor for MutexId* (free on thread exit)
void free_mutex_id(void *ptr) {
    MutexId *id = (MutexId *)ptr;
    if (id) free(id);
}

// Lazy init mutex TLS keys
void init_mutex_tls_keys(void) {
    if (!mutex_tls_keys_initialized) {
        pthread_key_create(&mutex_op_id_key, free_mutex_id);
        pthread_key_create(&mutex_op_ptr_key, NULL);  // No free for mutex ptr
        mutex_tls_keys_initialized = true;
    }
}

// Accessors for current_mutex_operation_id
MutexId* get_current_mutex_op_id(void) {
    init_mutex_tls_keys();
    return pthread_getspecific(mutex_op_id_key);
}

void set_current_mutex_op_id(const MutexId *id) {
    init_mutex_tls_keys();
    // Free any existing MutexId before setting new one
    MutexId *existing = pthread_getspecific(mutex_op_id_key);
    if (existing) {
        free(existing);
    }
    if (id == NULL) {
        pthread_setspecific(mutex_op_id_key, NULL);
        return;
    }
    MutexId *copy = malloc(sizeof(MutexId));
    if (copy) {
        *copy = *id;  // Copy contents
        pthread_setspecific(mutex_op_id_key, copy);
    }
}

// Accessors for current_mutex_operation_ptr
pthread_mutex_t* get_current_mutex_op_ptr(void) {
    init_mutex_tls_keys();
    return pthread_getspecific(mutex_op_ptr_key);
}

void set_current_mutex_op_ptr(pthread_mutex_t *ptr) {
    init_mutex_tls_keys();
    pthread_setspecific(mutex_op_ptr_key, ptr);
}

// Forward declarations
void detect_potential_deadlock(MutexId* current_id);

// Statistics
static MutexStats global_stats = {0};
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    // DEBUG: Log lock attempt (only if logging system is initialized and not already in a logging operation to avoid circular dependency)
    if (queue_system_initialized && !log_is_in_logging_operation()) {
        log_this(id->subsystem, "MUTEX REQ: %X as %s in %s()", LOG_LEVEL_TRACE, 3, (unsigned int)(uintptr_t)mutex, id->name, id->function);
    }

    // Record lock attempt for deadlock detection
    if (deadlock_detection_enabled) {
        pthread_mutex_lock(&deadlock_detection_mutex);
        // Expand capacity if needed
        if (active_lock_count >= active_lock_capacity) {
            active_lock_capacity = active_lock_capacity == 0 ? 16 : active_lock_capacity * 2;
            MutexLockAttempt* new_attempts = realloc(active_lock_attempts,
                active_lock_capacity * sizeof(MutexLockAttempt));
            if (!new_attempts) {
                pthread_mutex_unlock(&deadlock_detection_mutex);
                return MUTEX_ERROR;
            }
            active_lock_attempts = new_attempts;
        }
        // Add lock attempt
        active_lock_attempts[active_lock_count].id = *id;
        active_lock_attempts[active_lock_count].thread_id = pthread_self();
        active_lock_attempts[active_lock_count].attempt_start = time(NULL);
        active_lock_attempts[active_lock_count].is_write_lock = false;
        active_lock_count++;
        pthread_mutex_unlock(&deadlock_detection_mutex);
    }

    // Attempt to lock with timeout
    int result = pthread_mutex_timedlock(mutex, &timeout);

    // Remove from active attempts
    if (deadlock_detection_enabled) {
        pthread_mutex_lock(&deadlock_detection_mutex);
        // Find and remove this attempt
        for (size_t i = 0; i < active_lock_count; i++) {
            if (pthread_equal(active_lock_attempts[i].thread_id, pthread_self()) &&
                strcmp(active_lock_attempts[i].id.name, id->name) == 0) {
                // Shift remaining elements
                memmove(&active_lock_attempts[i], &active_lock_attempts[i + 1],
                    (active_lock_count - i - 1) * sizeof(MutexLockAttempt));
                active_lock_count--;
                break;
            }
        }
        pthread_mutex_unlock(&deadlock_detection_mutex);
    }

    // Update statistics
    pthread_mutex_lock(&stats_mutex);
    global_stats.total_locks++;
    if (result == 0) {
        // Success - record the locked mutex
        if (deadlock_detection_enabled) {
            pthread_mutex_lock(&deadlock_detection_mutex);
            // Expand capacity if needed
            if (locked_mutex_count >= locked_mutex_capacity) {
                locked_mutex_capacity = locked_mutex_capacity == 0 ? 16 : locked_mutex_capacity * 2;
                MutexLockAttempt* new_locked = realloc(locked_mutexes,
                    locked_mutex_capacity * sizeof(MutexLockAttempt));
                if (!new_locked) {
                    pthread_mutex_unlock(&deadlock_detection_mutex);
                    pthread_mutex_unlock(&stats_mutex);
                    return MUTEX_ERROR;
                }
                locked_mutexes = new_locked;
            }
            // Add locked mutex
            locked_mutexes[locked_mutex_count].id = *id;
            locked_mutexes[locked_mutex_count].thread_id = pthread_self();
            locked_mutexes[locked_mutex_count].mutex_ptr = mutex;
            locked_mutex_count++;
            // Set thread-local storage for unlock logging
            set_current_mutex_op_id(id);
            set_current_mutex_op_ptr(mutex);
            pthread_mutex_unlock(&deadlock_detection_mutex);
        }
        pthread_mutex_unlock(&stats_mutex);
        return MUTEX_SUCCESS;
    } else if (result == ETIMEDOUT) {
        // Timeout - potential deadlock
        global_stats.total_timeouts++;
        global_stats.last_timeout_time = time(NULL);
        pthread_mutex_unlock(&stats_mutex);
        // Clear thread-local storage on failure
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        // Log timeout with full context (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX EXP: %X as %s in %s() [%s:%d] - timeout after %dms", LOG_LEVEL_ERROR, 6, (unsigned int)(uintptr_t)mutex, id->name, id->function, id->file, id->line, timeout_ms);
        }
        // Check for potential deadlock
        if (deadlock_detection_enabled) {
            detect_potential_deadlock(id);
        }
        return MUTEX_TIMEOUT;
    } else {
        // Other error
        global_stats.total_errors++;
        pthread_mutex_unlock(&stats_mutex);
        // Clear thread-local storage on failure
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        // Log error (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX ERR: %X as %s in %s() [%s:%d] - error %d (%s)", LOG_LEVEL_ERROR, 6, (unsigned int)(uintptr_t)mutex, id->name, id->function, id->file, id->line, result, strerror(result));
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
        // Clear thread-local storage
        set_current_mutex_op_id(NULL);
        set_current_mutex_op_ptr(NULL);
        return MUTEX_SUCCESS;
    } else {
        // This is a serious error - log it (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(SR_MUTEXES, "MUTEX ERR: %X unlock failed - error %d (%s)", LOG_LEVEL_ERROR, 3, (unsigned int)(uintptr_t)mutex, result, strerror(result));
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
        // DEBUG: Log unlock operation (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX REL: %X as %s in %s()", LOG_LEVEL_TRACE, 3,
                     (unsigned int)(uintptr_t)mutex, id->name, id->function);
        }
        return MUTEX_SUCCESS;
    } else {
        // This is a serious error - log it (only if not already in a logging operation to avoid circular dependency)
        if (!log_is_in_logging_operation()) {
            log_this(id->subsystem, "MUTEX ERR: %X unlock failed - error %d (%s)", LOG_LEVEL_ERROR, 3, (unsigned int)(uintptr_t)mutex, result, strerror(result));
        }
        return MUTEX_ERROR;
    }
}

/*
 * Deadlock detection helper
 */
void detect_potential_deadlock(MutexId* current_id) {
    pthread_mutex_lock(&deadlock_detection_mutex);
    // Look for threads waiting for mutexes we might hold
    for (size_t i = 0; i < active_lock_count; i++) {
        MutexLockAttempt* attempt = &active_lock_attempts[i];
        // Check if this thread is waiting for a mutex
        // In a real deadlock detection algorithm, we'd check the actual
        // mutex ownership, but this is a simplified version
        if (strcmp(attempt->id.subsystem, current_id->subsystem) == 0) {
            log_this(SR_MUTEXES, "DEADLOCK: Thread waiting for %s while we wait for %s", LOG_LEVEL_ERROR, 2, attempt->id.name, current_id->name);
            global_stats.total_deadlocks_detected++;
            global_stats.last_deadlock_time = time(NULL);
        }
    }
    pthread_mutex_unlock(&deadlock_detection_mutex);
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
    pthread_mutex_lock(&deadlock_detection_mutex);
    if (active_lock_count == 0) {
        log_this(SR_MUTEXES, "No active mutex lock attempts", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(SR_MUTEXES, "Active mutex lock attempts: %zu", LOG_LEVEL_TRACE, 1, active_lock_count);
        for (size_t i = 0; i < active_lock_count; i++) {
            MutexLockAttempt* attempt = &active_lock_attempts[i];
            time_t duration = time(NULL) - attempt->attempt_start;
            log_this(SR_MUTEXES, " [%zu] %s in %s() [%s:%d] - waiting %ld seconds", LOG_LEVEL_TRACE, 6, i, attempt->id.name, attempt->id.function, attempt->id.file, attempt->id.line, duration);
        }
    }
    pthread_mutex_unlock(&deadlock_detection_mutex);
}

/*
 * Statistics functions
 */
void mutex_get_stats(MutexStats* stats) {
    if (!stats) return;
    pthread_mutex_lock(&stats_mutex);
    *stats = global_stats;
    pthread_mutex_unlock(&stats_mutex);
}

void mutex_reset_stats(void) {
    pthread_mutex_lock(&stats_mutex);
    memset(&global_stats, 0, sizeof(MutexStats));
    pthread_mutex_unlock(&stats_mutex);
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
    // Clean up deadlock detection structures
    pthread_mutex_lock(&deadlock_detection_mutex);
    free(active_lock_attempts);
    active_lock_attempts = NULL;
    active_lock_count = 0;
    active_lock_capacity = 0;
    // Clean up locked mutex tracking structures
    free(locked_mutexes);
    locked_mutexes = NULL;
    locked_mutex_count = 0;
    locked_mutex_capacity = 0;
    // Clear thread-local storage (keys auto-free on delete)
    pthread_key_delete(mutex_op_id_key);
    pthread_key_delete(mutex_op_ptr_key);
    pthread_mutex_unlock(&deadlock_detection_mutex);
    log_this(SR_MUTEXES, "Mutex system cleanup completed", LOG_LEVEL_TRACE, 0);
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
