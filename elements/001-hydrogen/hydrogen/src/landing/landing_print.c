/*
 * Landing Print Subsystem
 * 
 * This module handles the landing (shutdown) of the print subsystem.
 * It provides functions for checking landing readiness and shutting down
 * the print queue.
 * 
 * The print subsystem landing involves:
 * - Checking for active print jobs
 * - Stopping print queue thread
 * - Freeing print resources
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

#include <src/landing/landing.h>
#include <src/utils/utils_logging.h>
#include <src/threads/threads.h>
#include <src/config/config.h>
#include <src/registry/registry_integration.h>
#include <src/state/state_types.h>

// External declarations
extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;
extern volatile sig_atomic_t print_system_shutdown;
extern AppConfig* app_config;

// Check if the print subsystem is ready to land
LaunchReadiness check_print_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_PRINT;

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup(SR_PRINT);
    
    // Check if print subsystem is running
    if (!is_subsystem_running_by_name(SR_PRINT)) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Print subsystem not running");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Print subsystem running");
    
    // Check for active print jobs
    if (print_threads.thread_count > 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Active print jobs in progress");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      No active print jobs");
    
    // Check for dependent subsystems
    // Print is the last subsystem, so it has no dependents to check
    readiness.messages[msg_count++] = strdup("  Go:      No dependent subsystems");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Landing of Print Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Land the print subsystem
int land_print_subsystem(void) {
    log_this(SR_PRINT, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_PRINT, "LANDING: " SR_PRINT, LOG_LEVEL_STATE, 0);
    
    // Set shutdown flag
    print_system_shutdown = 1;
    
    // Wait for print queue thread to complete
    if (print_queue_thread) {
        pthread_join(print_queue_thread, NULL);
    }
    
    // Reset thread resources
    init_service_threads(&print_threads, SR_PRINT);
    
    // Additional cleanup as needed
    return 1;  // Success
}
