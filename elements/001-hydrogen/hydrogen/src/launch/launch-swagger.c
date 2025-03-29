/*
 * Launch Swagger Subsystem
 * 
 * This module handles the initialization of the Swagger subsystem.
 * It provides functions for checking readiness and launching Swagger.
 * 
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing-swagger.c
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"

// External declarations
extern volatile sig_atomic_t swagger_system_shutdown;

// Check if the swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // For now, mark as not ready as it's under development
    readiness.ready = false;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Swagger");
    readiness.messages[1] = strdup("  No-Go:   Swagger System Not Ready");
    readiness.messages[2] = strdup("  Reason:  Under Development");
    readiness.messages[3] = strdup("  Decide:  No-Go For Launch of Swagger");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Initialize the swagger subsystem
int init_swagger_subsystem(void) {
    // Reset shutdown flag
    swagger_system_shutdown = 0;
    
    // Initialize swagger system
    // Currently a placeholder as system is under development
    return 0;
}