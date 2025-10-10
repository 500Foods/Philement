/*
 * Mutex Utility Library
 *
 * Provides consistent, timeout-aware mutex operations with deadlock detection
 * and comprehensive logging for the entire Hydrogen codebase.
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

// Default timeout values (in milliseconds)
#define MUTEX_DEFAULT_TIMEOUT_MS 500
#define MUTEX_HEARTBEAT_TIMEOUT_MS 500
#define MUTEX_INIT_TIMEOUT_MS 500

// Mutex operation results
typedef enum {
    MUTEX_SUCCESS = 0,
    MUTEX_TIMEOUT = 1,
    MUTEX_DEADLOCK_DETECTED = 2,
    MUTEX_ERROR = 3
} MutexResult;

// Mutex identification structure
typedef struct {
    const char* name;           // Human-readable name for logging
    const char* subsystem;      // Subsystem identifier (e.g., "DATABASE", "WEBSERVER")
    const char* function;       // Function name where mutex is used
    const char* file;           // Source file name
    int line;                   // Line number in source file
} MutexId;

// Lock attempt tracking for deadlock detection
typedef struct {
    MutexId id;
    pthread_t thread_id;
    time_t attempt_start;
    bool is_write_lock;         // For future rwlock support
    pthread_mutex_t* mutex_ptr; // Pointer to the actual mutex for unlock tracking
} MutexLockAttempt;

// Core mutex functions with timeout and identification
MutexResult mutex_lock_with_timeout(
    pthread_mutex_t* mutex,
    MutexId* id,
    int timeout_ms
);

MutexResult mutex_try_lock(
    pthread_mutex_t* mutex,
    MutexId* id
);

// Convenience macros for easy usage
#define MUTEX_LOCK(mutex_ptr, subsystem) \
    mutex_lock_with_timeout(mutex_ptr, \
        &(MutexId){#mutex_ptr, subsystem, __func__, __FILE__, __LINE__}, \
        MUTEX_DEFAULT_TIMEOUT_MS)

#define MUTEX_LOCK_TIMEOUT(mutex_ptr, subsystem, timeout_ms) \
    mutex_lock_with_timeout(mutex_ptr, \
        &(MutexId){#mutex_ptr, subsystem, __func__, __FILE__, __LINE__}, \
        timeout_ms)

#define MUTEX_TRY_LOCK(mutex_ptr, subsystem) \
    mutex_try_lock(mutex_ptr, \
        &(MutexId){#mutex_ptr, subsystem, __func__, __FILE__, __LINE__})

// Convenience macro for unlock with logging
#define MUTEX_UNLOCK(mutex_ptr, subsystem) \
    mutex_unlock_with_id(mutex_ptr, \
        &(MutexId){#mutex_ptr, subsystem, __func__, __FILE__, __LINE__})

// Unlock with error checking
MutexResult mutex_unlock(pthread_mutex_t* mutex);

// Unlock with identification for logging
MutexResult mutex_unlock_with_id(pthread_mutex_t* mutex, MutexId* id);

// Deadlock detection and monitoring
void mutex_enable_deadlock_detection(bool enable);
bool mutex_is_deadlock_detection_enabled(void);
void mutex_log_active_locks(void);

// Statistics and monitoring
typedef struct {
    unsigned long long total_locks;
    unsigned long long total_timeouts;
    unsigned long long total_deadlocks_detected;
    unsigned long long total_errors;
    time_t last_timeout_time;
    time_t last_deadlock_time;
} MutexStats;

void mutex_get_stats(MutexStats* stats);
void mutex_reset_stats(void);

// Initialization and cleanup
bool mutex_system_init(void);
void mutex_system_cleanup(void);

// Utility functions
const char* mutex_result_to_string(MutexResult result);
void mutex_log_result(MutexResult result, MutexId* id, int timeout_ms);

#endif // MUTEX_H