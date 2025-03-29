/*
 * Landing API Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the API subsystem.
 * Note: API is part of the WebServer, not a standalone service.
 * It provides functions for:
 * - Checking API landing readiness
 * - Managing API route cleanup
 * 
 * Dependencies:
 * - Must coordinate with WebServer for shutdown
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Check if the API subsystem is ready to land
LandingReadiness check_api_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "API";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("API");
    
    // Check if API is actually running
    if (!is_subsystem_running_by_name("API")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   API not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of API");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check WebServer status since API depends on it
    bool webserver_ready = is_subsystem_running_by_name("WebServer");
    if (!webserver_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   WebServer subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of API");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // API is ready for landing when WebServer is ready
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      WebServer ready for shutdown");
    readiness.messages[2] = strdup("  Go:      API routes ready for cleanup");
    readiness.messages[3] = strdup("  Decide:  Go For Landing of API");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// No explicit shutdown needed as API is part of WebServer
// Function provided for consistency with other subsystems
void shutdown_api(void) {
    log_this("API", "API routes cleanup complete", LOG_LEVEL_STATE);
}