/*
 * Landing Swagger Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the Swagger subsystem.
 * It provides functions for:
 * - Checking Swagger landing readiness
 * - Managing Swagger shutdown
 * 
 * Dependencies:
 * - Must wait for WebServer subsystem to be ready for shutdown
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern volatile sig_atomic_t swagger_system_shutdown;

// Check if the swagger subsystem is ready to land
LaunchReadiness check_swagger_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_SWAGGER;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_SWAGGER);
    
    // Check if swagger is actually running
    if (!is_subsystem_running_by_name(SR_SWAGGER)) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Swagger not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Swagger");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check WebServer dependency
    bool webserver_ready = is_subsystem_running_by_name("WebServer");
    if (!webserver_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebServer subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Swagger");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // All checks passed
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      Swagger ready for shutdown");
    readiness.messages[2] = strdup("  Go:      WebServer ready for shutdown");
    readiness.messages[3] = strdup("  Decide:  Go For Landing of Swagger");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Land the swagger subsystem
int land_swagger_subsystem(void) {
    log_this(SR_SWAGGER, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_SWAGGER, "LANDING: SWAGGER", LOG_LEVEL_STATE, 0);
    
    // Signal shutdown
    swagger_system_shutdown = 1;
    log_this(SR_SWAGGER, "Signaled Swagger system to stop", LOG_LEVEL_STATE, 0);
    
    // Cleanup resources
    // Additional cleanup will be added as needed
    
    log_this(SR_SWAGGER, "Swagger shutdown complete", LOG_LEVEL_STATE, 0);
    return 1; // Success
}
