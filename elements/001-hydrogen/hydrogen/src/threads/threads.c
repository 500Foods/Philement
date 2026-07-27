// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "threads.h"
#include <sys/syscall.h>

// // Public interface declarations
// void init_service_threads(ServiceThreads *threads);
// void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
// void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);
// void update_service_thread_metrics(ServiceThreads *threads);
// ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

// // External declarations
// extern ServiceThreads logging_threads;
// extern ServiceThreads webserver_threads;
// extern ServiceThreads websocket_threads;
// extern ServiceThreads mdns_server_threads;
// extern ServiceThreads print_threads;

// Flag to indicate we're in final shutdown mode - no more thread management logging
// This will be set just before logging the final "Shutdown complete" message
volatile sig_atomic_t final_shutdown_mode = 0;

// Note: All ServiceThreads structures are defined in state.c

// Internal state
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private function declarations
size_t get_thread_stack_size(pid_t tid);

// Initialize service thread tracking
void init_service_threads(ServiceThreads *threads, const char* subsystem_name) {
    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        threads->thread_count = 0;
        threads->virtual_memory = 0;
        threads->resident_memory = 0;
        threads->memory_percent = 0.0;
        memset(threads->thread_ids, 0, sizeof(pthread_t) * MAX_SERVICE_THREADS);
        memset(threads->thread_metrics, 0, sizeof(ThreadMemoryMetrics) * MAX_SERVICE_THREADS);
        memset(threads->thread_tids, 0, sizeof(pid_t) * MAX_SERVICE_THREADS);
        memset(threads->thread_descriptions, 0, sizeof(char) * MAX_SERVICE_THREADS * 32);

        if (subsystem_name) {
            strncpy(threads->subsystem, subsystem_name, 31); // Leave space for null terminator
            threads->subsystem[31] = '\0'; // Ensure null-termination
        } else {
            strncpy(threads->subsystem, "Unknown", 31);
            threads->subsystem[31] = '\0';
        }

        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }
}

// Add a thread to service tracking with custom subsystem and optional description
void add_service_thread_with_subsystem(ServiceThreads *threads, pthread_t thread_id, const char* subsystem, const char* description) {
    char msg[128];
    msg[0] = '\0';
    bool log_error_max = false;
    bool do_log = false;
    const char* log_subsystem = subsystem ? subsystem : SR_THREADS_LIB;
    char subsystem_copy[32];
    subsystem_copy[0] = '\0';

    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, log_subsystem);
    if (lock_result == MUTEX_SUCCESS) {
        if (threads->thread_count < MAX_SERVICE_THREADS) {
            pid_t tid = (pid_t)syscall(SYS_gettid);
            threads->thread_ids[threads->thread_count] = thread_id;
            threads->thread_tids[threads->thread_count] = tid;

            if (description) {
                strncpy(threads->thread_descriptions[threads->thread_count], description, 31);
                threads->thread_descriptions[threads->thread_count][31] = '\0';
            } else {
                threads->thread_descriptions[threads->thread_count][0] = '\0';
            }

            threads->thread_count++;

            if (!final_shutdown_mode) {
                strncpy(subsystem_copy, threads->subsystem, sizeof(subsystem_copy) - 1);
                subsystem_copy[sizeof(subsystem_copy) - 1] = '\0';
                if (description && strlen(description) > 0) {
                    snprintf(msg, sizeof(msg), "%s (%s): Thread %lu (tid: %d) added, count: %d",
                             subsystem_copy, description, (unsigned long)thread_id, tid, threads->thread_count);
                } else {
                    snprintf(msg, sizeof(msg), "%s: Thread %lu (tid: %d) added, count: %d",
                             subsystem_copy, (unsigned long)thread_id, tid, threads->thread_count);
                }
                do_log = true;
            }
        } else {
            log_error_max = true;
        }
        MUTEX_UNLOCK(&thread_mutex, log_subsystem);
    }

    if (do_log) {
        log_this(log_subsystem, msg, LOG_LEVEL_TRACE, 0);
    } else if (log_error_max) {
        log_this(log_subsystem, "Failed to add thread: MAX_SERVICE_THREADS reached", LOG_LEVEL_ERROR, 0);
    }
}

// Add a thread to service tracking with optional description
void add_service_thread_with_description(ServiceThreads *threads, pthread_t thread_id, const char* description) {
    add_service_thread_with_subsystem(threads, thread_id, SR_THREADS_LIB, description);
}

// Add a thread to service tracking
void add_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    add_service_thread_with_description(threads, thread_id, NULL);
}

// Remove a thread from service tracking
// If skip_logging is true, don't generate log messages (used during shutdown)
// Returns true when a removal log line should be emitted by the caller (outside thread_mutex).
bool remove_thread_internal(ServiceThreads *threads, int index, bool skip_logging,
                            char* log_msg, size_t log_msg_size) {
    pthread_t thread_id = threads->thread_ids[index];

    // Move last thread to this position to maintain array compaction
    if (index < threads->thread_count - 1) {  // Only move if not removing the last element
        threads->thread_ids[index] = threads->thread_ids[threads->thread_count - 1];
        threads->thread_tids[index] = threads->thread_tids[threads->thread_count - 1];
        threads->thread_metrics[index] = threads->thread_metrics[threads->thread_count - 1];
        strncpy(threads->thread_descriptions[index], threads->thread_descriptions[threads->thread_count - 1], 32);
    }

    // Always clear the last element (whether moved or not)
    int last_index = threads->thread_count - 1;
    threads->thread_ids[last_index] = 0;
    threads->thread_tids[last_index] = 0;
    threads->thread_metrics[last_index].virtual_bytes = 0;
    threads->thread_metrics[last_index].resident_bytes = 0;
    threads->thread_descriptions[last_index][0] = '\0';

    threads->thread_count--;

    if (!skip_logging && !final_shutdown_mode && log_msg && log_msg_size > 0) {
        snprintf(log_msg, log_msg_size, "%s: Thread %lu removed, count: %d",
                 threads->subsystem, (unsigned long)thread_id, threads->thread_count);
        return true;
    }
    return false;
}

void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    char msg[128];
    bool do_log = false;

    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        for (int i = 0; i < threads->thread_count; i++) {
            if (pthread_equal(threads->thread_ids[i], thread_id)) {
                do_log = remove_thread_internal(threads, i, false, msg, sizeof(msg));
                break;
            }
        }
        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }

    if (do_log) {
        log_this(SR_THREADS_LIB, msg, LOG_LEVEL_TRACE, 0);
    }
}

// Get thread stack size from /proc/[tid]/status
size_t get_thread_stack_size(pid_t tid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/task/%d/status", tid);
    
    FILE *status = fopen(path, "r");
    if (!status) return 0;
    
    char line[256];
    size_t stack_size = 0;
    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %20zu", &stack_size);
            break;
        }
    }
    
    fclose(status);
    return stack_size;
}

// Update memory metrics for all threads in a service
void update_service_thread_metrics(ServiceThreads *threads) {
    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        // Reset service totals
        threads->virtual_memory = 0;
        threads->resident_memory = 0;

        // Update each thread's metrics using stack size only
        for (int i = 0; i < threads->thread_count; i++) {
            pid_t tid = threads->thread_tids[i];

            if (kill(tid, 0) != 0) {
                (void)remove_thread_internal(threads, i, true, NULL, 0);
                i--; // Reprocess this index
                continue;
            }

            // Get thread stack size
            size_t stack_size = get_thread_stack_size(tid);

            // Update thread metrics with stack size
            ThreadMemoryMetrics *metrics = &threads->thread_metrics[i];
            metrics->virtual_bytes = stack_size * 1024; // Convert KB to bytes
            metrics->resident_bytes = stack_size * 1024; // Assume stack is resident

            // Add to service totals
            threads->virtual_memory += metrics->virtual_bytes;
            threads->resident_memory += metrics->resident_bytes;
        }

        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }
}

// Get memory metrics for a specific thread
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id) {
    ThreadMemoryMetrics metrics = {0, 0};

    // Check for NULL threads parameter
    if (!threads) {
        return metrics;
    }

    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        // Find the thread in our tracking array
        for (int i = 0; i < threads->thread_count; i++) {
            if (pthread_equal(threads->thread_ids[i], thread_id)) {
                metrics = threads->thread_metrics[i];
                break;
            }
        }

        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }

    return metrics;
}

// Report status of all service threads
void report_thread_status(void) {
    int logging_count = 0;
    int web_count = 0;
    int websocket_count = 0;
    int mdns_count = 0;
    int mail_count = 0;
    int print_count = 0;
    int database_count = 0;
    int total_threads = 0;
    bool have_counts = false;

    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        logging_count = logging_threads.thread_count;
        web_count = webserver_threads.thread_count;
        websocket_count = websocket_threads.thread_count;
        mdns_count = mdns_server_threads.thread_count;
        mail_count = mailrelay_threads.thread_count;
        print_count = print_threads.thread_count;
        database_count = database_threads.thread_count;
        total_threads = logging_count + web_count + websocket_count + mdns_count +
                        mail_count + print_count + database_count;
        have_counts = true;
        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }

    if (!have_counts) {
        return;
    }

    log_this(SR_THREADS, "Thread Status Report:", LOG_LEVEL_STATE, 0);
    log_this(SR_THREADS, "  Logging Threads: %d active", LOG_LEVEL_STATE, 1, logging_count);
    log_this(SR_THREADS, "  Web Threads: %d active", LOG_LEVEL_STATE, 1, web_count);
    log_this(SR_THREADS, "  WebSocket Threads: %d active", LOG_LEVEL_STATE, 1, websocket_count);
    log_this(SR_THREADS, "  mDNS Server Threads: %d active", LOG_LEVEL_STATE, 1, mdns_count);
    log_this(SR_THREADS, "  Mail Relay Threads: %d active", LOG_LEVEL_STATE, 1, mail_count);
    log_this(SR_THREADS, "  Print Threads: %d active", LOG_LEVEL_STATE, 1, print_count);
    log_this(SR_THREADS, "  Database Threads: %d active", LOG_LEVEL_STATE, 1, database_count);
    log_this(SR_THREADS, "Total Active Threads: %d", LOG_LEVEL_STATE, 1, total_threads);
}

// Free thread-related resources
void free_threads_resources(void) {
    MutexResult lock_result = MUTEX_LOCK(&thread_mutex, SR_THREADS_LIB);
    if (lock_result == MUTEX_SUCCESS) {
        // Set final shutdown mode to prevent excessive logging
        final_shutdown_mode = 1;

        MUTEX_UNLOCK(&thread_mutex, SR_THREADS_LIB);
    }

    // Now reset all thread structures without holding the lock
    // (init_service_threads acquires its own lock)
    init_service_threads(&logging_threads, "Unknown");
    init_service_threads(&webserver_threads, "Unknown");
    init_service_threads(&websocket_threads, "Unknown");
    init_service_threads(&mdns_server_threads, "Unknown");
    init_service_threads(&print_threads, "Unknown");
    init_service_threads(&database_threads, "Unknown");
}
