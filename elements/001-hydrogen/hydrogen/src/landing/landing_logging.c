/*
 * Landing Logging Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the logging subsystem.
 * It provides functions for:
 * - Checking logging landing readiness
 * - Managing logging shutdown
 * - Cleaning up logging resources
 * 
 * Critical Note:
 * - Logging must remain available until all other subsystems complete shutdown
 * - Final shutdown must ensure no pending messages are lost
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern ServiceThreads logging_threads;
extern pthread_t log_thread;
extern volatile sig_atomic_t log_queue_shutdown;
extern AppConfig* app_config;

// Local function prototypes
bool check_other_subsystems_complete(void);

// Check if all other subsystems have completed shutdown
bool check_other_subsystems_complete(void) {
    for (int i = 0; i < subsystem_registry.count; i++) {
        const SubsystemInfo* info = &subsystem_registry.subsystems[i];
        // Skip logging subsystem itself
        if (strcmp(info->name, SR_LOGGING) == 0) continue;
        
        // If any other subsystem is still active, we're not ready
        if (info->state != SUBSYSTEM_INACTIVE) {
            return false;
        }
    }
    return true;
}

// Check if the logging subsystem is ready to land
LaunchReadiness check_logging_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_LOGGING;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_LOGGING);
    
    // Check if logging is actually running
    if (!is_subsystem_running_by_name(SR_LOGGING)) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Logging not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Logging");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check if other subsystems are done
    bool others_complete = check_other_subsystems_complete();
    if (!others_complete) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Other subsystems still active");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Logging");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (log_thread && logging_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      Logging thread ready for shutdown");
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   Logging thread not accessible");
    }
    
    // Final decision
    if (threads_ready) {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All other subsystems inactive");
        readiness.messages[3] = strdup("  Go:      Ready for final cleanup");
        readiness.messages[4] = strdup("  Decide:  Go For Landing of Logging");
    } else {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of Logging");
    }
    readiness.messages[5] = NULL;
    
    return readiness;
}

// Land the logging subsystem
int land_logging_subsystem(void) {
    log_this(SR_LOGGING, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_LOGGING, "LANDING: " SR_LOGGING, LOG_LEVEL_STATE, 0);
    
    bool success = true;
    
    // Signal thread shutdown
    log_queue_shutdown = 1;
    log_this(SR_LOGGING, "Signaled " SR_LOGGING " thread to stop", LOG_LEVEL_STATE, 0);
    
    // Wait for thread to complete
    if (log_thread) {
        log_this(SR_LOGGING, "Waiting for " SR_LOGGING " thread to complete", LOG_LEVEL_STATE, 0);
        if (pthread_join(log_thread, NULL) != 0) {
            log_this(SR_LOGGING, "Error waiting for " SR_LOGGING " thread", LOG_LEVEL_ERROR, 0);
            success = false;
        } else {
            log_this(SR_LOGGING, SR_LOGGING " thread completed", LOG_LEVEL_STATE, 0);
        }
    }
    
    // Remove the logging thread from tracking
    remove_service_thread(&logging_threads, log_thread);
    
    // Reinitialize thread structure
    init_service_threads(&logging_threads, SR_LOGGING);
    
    // Clean up logging configuration
    if (app_config) {
        log_this(SR_LOGGING, "Cleaning up " SR_LOGGING " configuration", LOG_LEVEL_STATE, 0);
        
        // Clean up logging config - handles all components including file logging
        cleanup_logging_config(&app_config->logging);
    } else {
        log_this(SR_LOGGING, "Warning: app_config is NULL during " SR_LOGGING " cleanup", LOG_LEVEL_ALERT, 0);
    }
    
    log_this(SR_LOGGING, SR_LOGGING " shutdown complete", LOG_LEVEL_STATE, 0);
    
    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}
