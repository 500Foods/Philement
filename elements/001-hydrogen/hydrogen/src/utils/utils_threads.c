// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

// Project headers
#include "utils_threads.h"
#include "utils_logging.h"
#include "../logging/logging.h"  // For LOG_LEVEL constants
#include "../state/state.h"

// Log level constants from logging.h
#ifndef LOG_LEVEL_STATE
#error "LOG_LEVEL_STATE not defined - logging.h not properly included"
#endif

// Public interface declarations
void init_service_threads(ServiceThreads *threads);
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);
void update_service_thread_metrics(ServiceThreads *threads);
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

// External declarations
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Flag to indicate we're in final shutdown mode - no more thread management logging
// This will be set just before logging the final "Shutdown complete" message
volatile sig_atomic_t final_shutdown_mode = 0;

// Internal state
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private function declarations
static size_t get_thread_stack_size(pid_t tid);

// Initialize service thread tracking
void init_service_threads(ServiceThreads *threads) {
    pthread_mutex_lock(&thread_mutex);
    threads->thread_count = 0;
    memset(threads->thread_metrics, 0, sizeof(ThreadMemoryMetrics) * MAX_SERVICE_THREADS);
    memset(threads->thread_tids, 0, sizeof(pid_t) * MAX_SERVICE_THREADS);
    pthread_mutex_unlock(&thread_mutex);
}

// Add a thread to service tracking
void add_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    if (threads->thread_count < MAX_SERVICE_THREADS) {
        pid_t tid = syscall(SYS_gettid);
        threads->thread_ids[threads->thread_count] = thread_id;
        threads->thread_tids[threads->thread_count] = tid;
        threads->thread_count++;
        
        // Only log if not in final shutdown mode
        if (!final_shutdown_mode) {
            char msg[128];
            // Use log group to ensure consistent formatting
            log_group_begin();
            snprintf(msg, sizeof(msg), "Thread %lu (tid: %d) added, count: %d", 
                     (unsigned long)thread_id, tid, threads->thread_count);
            log_this("ThreadMgmt", msg, LOG_LEVEL_STATE);
            log_group_end();
        }
    } else {
        log_this("ThreadMgmt", "Failed to add thread: MAX_SERVICE_THREADS reached", LOG_LEVEL_DEBUG);
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Remove a thread from service tracking
// If skip_logging is true, don't generate log messages (used during shutdown)
static void remove_thread_internal(ServiceThreads *threads, int index, bool skip_logging) {
    pthread_t thread_id = threads->thread_ids[index];
    
    // Move last thread to this position
    threads->thread_count--;
    if (index < threads->thread_count) {
        threads->thread_ids[index] = threads->thread_ids[threads->thread_count];
        threads->thread_tids[index] = threads->thread_tids[threads->thread_count];
        threads->thread_metrics[index] = threads->thread_metrics[threads->thread_count];
    }

    // Only log if not skipping, not in final shutdown, and early in shutdown (when app_config still exists)
    if (!skip_logging && !final_shutdown_mode) {
        char msg[128];
        log_group_begin();
        snprintf(msg, sizeof(msg), "Thread %lu removed, count: %d", 
                 (unsigned long)thread_id, threads->thread_count);
        log_this("ThreadMgmt", msg, LOG_LEVEL_STATE);
        log_group_end();
    }
}

void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            remove_thread_internal(threads, i, false);
            break;
        }
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Get thread stack size from /proc/[tid]/status
static size_t get_thread_stack_size(pid_t tid) {
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
    pthread_mutex_lock(&thread_mutex);
    
    // Reset service totals
    threads->virtual_memory = 0;
    threads->resident_memory = 0;
    
    // Update each thread's metrics using stack size only
    for (int i = 0; i < threads->thread_count; i++) {
        pid_t tid = threads->thread_tids[i];
        
        // Check if thread is alive
        if (kill(tid, 0) != 0) {
            // Thread is dead, remove it
            remove_thread_internal(threads, i, true);  // Skip logging during metrics update
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
    
    pthread_mutex_unlock(&thread_mutex);
}

// Get memory metrics for a specific thread
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id) {
    ThreadMemoryMetrics metrics = {0, 0};
    
    pthread_mutex_lock(&thread_mutex);
    
    // Find the thread in our tracking array
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            metrics = threads->thread_metrics[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&thread_mutex);
    
    return metrics;
}