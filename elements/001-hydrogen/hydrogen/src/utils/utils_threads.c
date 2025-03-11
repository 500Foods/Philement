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
#include "../logging/logging.h"
#include "../state/state.h"

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
        char msg[128];
        // Only show thread ID details at debug level during shutdown
        if (server_running) {
            snprintf(msg, sizeof(msg), "Added thread %lu (tid: %d), new count: %d", 
                     (unsigned long)thread_id, tid, threads->thread_count);
            log_this("ThreadMgmt", msg, LOG_LEVEL_STATE);
        } else {
            snprintf(msg, sizeof(msg), "Thread added, count: %d", threads->thread_count);
            log_this("ThreadMgmt", msg, LOG_LEVEL_ALERT);
        }
    } else {
        log_this("ThreadMgmt", "Failed to add thread: MAX_SERVICE_THREADS reached", LOG_LEVEL_DEBUG);
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Remove a thread from service tracking
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            // Move last thread to this position
            threads->thread_count--;
            if (i < threads->thread_count) {
                threads->thread_ids[i] = threads->thread_ids[threads->thread_count];
                threads->thread_tids[i] = threads->thread_tids[threads->thread_count];
                threads->thread_metrics[i] = threads->thread_metrics[threads->thread_count];
            }
            char msg[128];
            // Only show thread ID details at debug level during shutdown
            if (server_running) {
                snprintf(msg, sizeof(msg), "Removed thread %lu, new count: %d", 
                         (unsigned long)thread_id, threads->thread_count);
                log_this("ThreadMgmt", msg, LOG_LEVEL_STATE);
            } else {
                snprintf(msg, sizeof(msg), "Thread removed, count: %d", threads->thread_count);
                log_this("ThreadMgmt", msg, LOG_LEVEL_ALERT);
            }
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
            sscanf(line, "VmStk: %zu", &stack_size);
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
            threads->thread_count--;
            if (i < threads->thread_count) {
                threads->thread_ids[i] = threads->thread_ids[threads->thread_count];
                threads->thread_tids[i] = threads->thread_tids[threads->thread_count];
                threads->thread_metrics[i] = threads->thread_metrics[threads->thread_count];
            }
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