/**
 * @file launch-threads.c
 * @brief Thread subsystem launch readiness checks and initialization
 */

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "launch-threads.h"
#include "../../utils/utils_threads.h"
#include "../../utils/utils_logging.h"
#include "../../logging/logging.h"  // For LOG_LEVEL constants
#include "../../state/state.h"
#include "../../config/config.h"

// External declarations
extern AppConfig* app_config;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Thread tracking for the main thread and system threads
ServiceThreads system_threads = {0};  // Zero initialize all members

// External declarations from hydrogen.c
extern pthread_t main_thread_id;

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
    result.messages = malloc(5 * sizeof(char*));  // Space for 4 messages + NULL terminator
    if (!result.messages) {
        return result;
    }

    // Add subsystem name as first message
    result.messages[0] = strdup("Threads");
    
    // Check system state
    if (server_stopping) {
        result.messages[1] = strdup("  No-Go:   System is shutting down");
        result.messages[2] = strdup("  Decide:  No-Go For Launch of Threads Subsystem");
        result.messages[3] = NULL;
        return result;
    }

    // Count current threads (should be 1 for main thread at this point)
    char thread_msg[128];
    snprintf(thread_msg, sizeof(thread_msg), "  Go:      Current thread count: %d (main thread)", 
             system_threads.thread_count + 1);  // +1 for main thread
    
    result.messages[1] = strdup("  Go:      System check passed (not shutting down)");
    result.messages[2] = strdup(thread_msg);
    result.messages[3] = strdup("  Decide:  Launch Threads");
    result.messages[4] = NULL;
    
    // Always return Go if system is not shutting down
    result.ready = true;
    
    return result;
}

/**
 * @brief Initialize the Threads subsystem
 * @returns 1 if initialization successful, 0 otherwise
 */
int launch_threads_subsystem(void) {
    
    // Initialize thread tracking
    init_service_threads(&system_threads);
    
    // Add main thread to tracking
    add_service_thread(&system_threads, main_thread_id);
    
    // Log successful initialization
    log_this("Threads", "Thread subsystem initialized with main thread", LOG_LEVEL_STATE);
    
    return 1;
}

/**
 * @brief Clean up thread tracking resources during shutdown
 */
void free_threads_resources(void) {
    // Log current thread count before cleanup
    log_this("Threads", "Final thread count before cleanup: %d", LOG_LEVEL_STATE, 
             system_threads.thread_count);
    
    // Remove main thread from tracking
    remove_service_thread(&system_threads, main_thread_id);
    
    // Log successful cleanup
    log_this("Threads", "Thread subsystem resources freed", LOG_LEVEL_STATE);
}