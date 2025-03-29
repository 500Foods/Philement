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
    
    snprintf(msg, sizeof(msg), "- Threads status: %d total (%d service thread%s + main thread)", 
             system_threads.thread_count, non_main_threads,
             non_main_threads == 1 ? "" : "s");
    log_this("Threads", msg, LOG_LEVEL_STATE);
    
    // Report memory usage
    snprintf(msg, sizeof(msg), "- Memory usage: %.2f MB virtual, %.2f MB resident", 
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

    // Register with subsystem registry during readiness check if needed
    if (get_subsystem_id_by_name("Threads") < 0) {
        register_subsystem_from_launch(
            "Threads",
            &system_threads,
            &main_thread_id,
            &threads_shutdown_flag,
            NULL,
            NULL
        );
    }

    // Initialize thread tracking if not already done
    if (system_threads.thread_count == 0) {
        init_service_threads(&system_threads);
        add_service_thread(&system_threads, main_thread_id);
        result.messages[1] = strdup("  Go:      Thread tracking initialized");
    } else {
        result.messages[1] = strdup("  Go:      Thread tracking ready");
    }
    
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

    // Log initialization first - so we know an attempt was made
    log_this("Threads", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Threads", "LAUNCH: THREADS", LOG_LEVEL_STATE);
    
    // Verify thread tracking is initialized
    if (system_threads.thread_count == 0) {
        // Not initialized during readiness check, initialize now
        init_service_threads(&system_threads);
        add_service_thread(&system_threads, main_thread_id);
        
        if (system_threads.thread_count != 1) {
            log_this("Threads", "Failed to add main thread to tracking", LOG_LEVEL_ERROR);
            return 0;
        }
    }
    
    // Get subsystem ID and update state to running
    int threads_id = get_subsystem_id_by_name("Threads");
    update_subsystem_state(threads_id, SUBSYSTEM_RUNNING);
    
    // Log initialization status with proper format
    log_this("Threads", "- Threads initialized", LOG_LEVEL_STATE);
    log_this("Threads", "- Main thread registered for monitoring", LOG_LEVEL_STATE);
    log_this("Threads", "- Threads mutex initialized and ready", LOG_LEVEL_STATE);
    report_thread_status();
    
    return 1;
}