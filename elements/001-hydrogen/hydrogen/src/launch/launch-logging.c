/*
 * Launch Logging Subsystem
 * 
 * This module handles the initialization of the logging subsystem.
 * It provides functions for checking readiness and launching the logging system.
 * 
 * Note: The logging subsystem launch is currently disabled to prevent crashes.
 * This needs careful handling and testing before re-enabling.
 * 
 * Note: Shutdown functionality has been moved to landing/landing-logging.c
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"
#include "../utils/utils_threads.h"

// External declarations
extern ServiceThreads logging_threads;
extern pthread_t log_thread;
extern volatile sig_atomic_t log_queue_shutdown;

// Check if the logging subsystem is ready to launch
LaunchReadiness check_logging_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // For now, mark as not ready to prevent crashes
    readiness.ready = false;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Logging");
    readiness.messages[1] = strdup("  No-Go:   Logging System Launch Disabled");
    readiness.messages[2] = strdup("  Reason:  Preventing Potential Crashes");
    readiness.messages[3] = strdup("  Decide:  No-Go For Launch of Logging");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Initialize the logging subsystem
int init_logging_subsystem(void) {
    // Reset shutdown flag
    log_queue_shutdown = 0;
    
    // Initialize logging thread structure
    init_service_threads(&logging_threads);
    
    // Currently disabled to prevent crashes
    // Additional initialization will be added when re-enabled
    return 0;
}