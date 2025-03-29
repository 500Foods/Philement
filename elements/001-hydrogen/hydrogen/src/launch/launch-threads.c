/**
 * @file launch-threads.c
 * @brief Thread subsystem launch readiness checks and initialization
 * 
 * Note: Shutdown functionality has been moved to landing/landing-threads.c
 */

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "launch-threads.h"
#include "../utils/utils_threads.h"
#include "../utils/utils_logging.h"
#include "../logging/logging.h"  // For LOG_LEVEL constants
#include "../state/state.h"
#include "../config/config.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations
extern AppConfig* app_config;

// Shutdown flag for threads subsystem
volatile sig_atomic_t threads_shutdown_flag = 0;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Thread tracking for the main thread and system threads
ServiceThreads system_threads = {0};  // Zero initialize all members

// External declarations from hydrogen.c
extern pthread_t main_thread_id;

// Function to report thread status
void report_thread_status(void) {
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
    
    snprintf(msg, sizeof(msg), "  Thread status: %d total (%d service thread%s + main thread)", 
             system_threads.thread_count, non_main_threads,
             non_main_threads == 1 ? "" : "s");
    log_this("Threads", msg, LOG_LEVEL_STATE);
    
    // Report memory usage
    snprintf(msg, sizeof(msg), "  Memory usage: %.2f MB virtual, %.2f MB resident", 
             system_threads.virtual_memory / (1024.0 * 1024.0),
             system_threads.resident_memory / (1024.0 * 1024.0));
    log_this("Threads", msg, LOG_LEVEL_STATE);
}

/**
 * @brief Check if the Threads subsystem is ready to launch
 * @returns LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_threads_launch_readiness(void) {
    LaunchReadiness result = {
        .subsystem = "Threads",
        .ready = false,
        .messages = NULL
    };

    // Allocate message array (NULL-terminated)
    result.messages = malloc(4 * sizeof(char*));  // Space for 3 messages + NULL terminator
    if (!result.messages) {
        return result;
    }

    // First message is just the subsystem name
    result.messages[0] = strdup("Threads");
    
    // Only check if system is shutting down
    if (server_stopping) {
        result.messages[1] = strdup("  No-Go:   System is shutting down");
        result.messages[2] = strdup("  Decide:  No-Go For Launch of Threads");
        result.messages[3] = NULL;
        return result;
    }
    
    result.messages[1] = strdup("  Go:      Ready for launch");
    result.messages[2] = strdup("  Decide:  Go For Launch of Threads");
    result.messages[3] = NULL;
    
    result.ready = true;
    return result;
}

/**
 * @brief Initialize the Threads subsystem
 * @returns 1 if initialization successful, 0 otherwise
 */
int launch_threads_subsystem(void) {
    // Check if already running
    int threads_id = get_subsystem_id_by_name("Threads");
    if (threads_id >= 0 && get_subsystem_state(threads_id) == SUBSYSTEM_RUNNING) {
        log_this("Threads", "Thread subsystem already running", LOG_LEVEL_STATE);
        return 1;
    }
    
    // Initialize thread tracking
    init_service_threads(&system_threads);
    add_service_thread(&system_threads, main_thread_id);
    
    // Register and update subsystem state
    update_subsystem_on_startup("Threads", true);
    
    // Log initialization and status with clear thread monitoring information
    log_this("Threads", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Threads", "LAUNCH: THREADS", LOG_LEVEL_STATE);
    log_this("Threads", "  Thread management system initialized", LOG_LEVEL_STATE);
    log_this("Threads", "  Thread subsystem has been initialized", LOG_LEVEL_STATE);
    log_this("Threads", "  Currently monitoring 1 thread (main thread)", LOG_LEVEL_STATE);
    log_this("Threads", "  Thread mutex initialized and ready", LOG_LEVEL_STATE);
    log_this("Threads", "  Thread tracking capabilities:", LOG_LEVEL_STATE);
    log_this("Threads", "    - Service thread registration", LOG_LEVEL_STATE);
    log_this("Threads", "    - Memory metrics monitoring", LOG_LEVEL_STATE);
    log_this("Threads", "    - Thread status reporting", LOG_LEVEL_STATE);
    log_this("Threads", "    - Automatic cleanup on exit", LOG_LEVEL_STATE);
    report_thread_status();
    log_this("Threads", "  Threads subsystem running", LOG_LEVEL_STATE);
    
    return 1;
}