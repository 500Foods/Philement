/**
 * @file landing-threads.c
 * @brief Thread subsystem landing (shutdown) implementation
 */

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "landing-threads.h"
#include "../utils/utils_threads.h"
#include "../utils/utils_logging.h"
#include "../logging/logging.h"  // For LOG_LEVEL constants
#include "../state/state.h"
#include "../config/config.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations
extern AppConfig* app_config;
extern ServiceThreads system_threads;
extern pthread_t main_thread_id;
extern volatile sig_atomic_t threads_shutdown_flag;

// Free thread resources during shutdown
void free_threads_resources(void) {
    // Free thread tracking resources
    for (int i = 0; i < system_threads.thread_count; i++) {
        remove_service_thread(&system_threads, system_threads.thread_ids[i]);
    }
    
    // Reinitialize thread structure
    init_service_threads(&system_threads);
    
    log_this("Threads", "Thread resources freed", LOG_LEVEL_STATE);
}

// Report thread status during landing sequence
void report_landing_thread_status(void) {
    char msg[256];
    int non_main_threads = 0;
    
    // Count non-main threads
    for (int i = 0; i < system_threads.thread_count; i++) {
        if (!pthread_equal(system_threads.thread_ids[i], main_thread_id)) {
            non_main_threads++;
        }
    }
    
    // Update memory metrics
    update_service_thread_metrics(&system_threads);
    
    snprintf(msg, sizeof(msg), "  Remaining threads: %d total (%d service thread%s + main thread)", 
             system_threads.thread_count, non_main_threads,
             non_main_threads == 1 ? "" : "s");
    log_this("Threads", msg, LOG_LEVEL_STATE);
    
    // Report memory usage
    snprintf(msg, sizeof(msg), "  Memory usage: %.2f MB virtual, %.2f MB resident", 
             system_threads.virtual_memory / (1024.0 * 1024.0),
             system_threads.resident_memory / (1024.0 * 1024.0));
    log_this("Threads", msg, LOG_LEVEL_STATE);
}

// Check if the Threads subsystem is ready to land
LandingReadiness check_threads_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Threads";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Threads");
    
    // Check if threads subsystem is actually running
    if (!is_subsystem_running_by_name("Threads")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Threads subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Threads");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Count non-main threads
    int non_main_threads = 0;
    for (int i = 0; i < system_threads.thread_count; i++) {
        if (!pthread_equal(system_threads.thread_ids[i], main_thread_id)) {
            non_main_threads++;
        }
    }
    
    // Update memory metrics
    update_service_thread_metrics(&system_threads);
    
    // Check thread status
    char thread_msg[128];
    snprintf(thread_msg, sizeof(thread_msg), "  %s:      Active threads: %d (main + %d other%s)", 
             non_main_threads > 0 ? "No-Go" : "Go",
             system_threads.thread_count,
             non_main_threads,
             non_main_threads == 1 ? "" : "s");
    readiness.messages[1] = strdup(thread_msg);
    
    if (non_main_threads > 0) {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Service threads still running");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of Threads");
        readiness.messages[4] = NULL;
    } else {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      Only main thread remaining");
        readiness.messages[3] = strdup("  Go:      Ready for final cleanup");
        readiness.messages[4] = strdup("  Decide:  Go For Landing of Threads");
        readiness.messages[5] = NULL;
    }
    
    return readiness;
}

// Shutdown the threads subsystem
void shutdown_threads(void) {
    log_this("Threads", "Beginning Threads shutdown sequence", LOG_LEVEL_STATE);
    
    // Signal thread shutdown
    threads_shutdown_flag = 1;
    log_this("Threads", "Signaled thread shutdown", LOG_LEVEL_STATE);
    
    // Log current thread count before cleanup
    log_this("Threads", "Final thread count before cleanup: %d", LOG_LEVEL_STATE, 
             system_threads.thread_count);
    
    // Update metrics one last time
    update_service_thread_metrics(&system_threads);
    
    // First try to wait for non-main threads to complete naturally
    for (int i = 0; i < system_threads.thread_count; i++) {
        if (!pthread_equal(system_threads.thread_ids[i], main_thread_id)) {
            // Check if thread is still alive
            pid_t tid = system_threads.thread_tids[i];
            if (kill(tid, 0) == 0) {
                log_this("Threads", "Waiting for thread %d to complete", LOG_LEVEL_STATE, tid);
                pthread_join(system_threads.thread_ids[i], NULL);
                log_this("Threads", "Thread %d completed", LOG_LEVEL_STATE, tid);
            }
        }
    }
    
    // Remove main thread from tracking
    remove_service_thread(&system_threads, main_thread_id);
    
    // Report final status
    report_landing_thread_status();
    
    log_this("Threads", "Thread subsystem shutdown complete", LOG_LEVEL_STATE);
}