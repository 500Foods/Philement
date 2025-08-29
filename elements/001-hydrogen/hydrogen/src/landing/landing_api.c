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

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "landing.h"
#include "../launch/launch.h"
#include "../api/api_service.h"

// Check if the API subsystem is ready to land
LaunchReadiness check_api_landing_readiness(void) {
    LaunchReadiness readiness = {0};
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

// Shutdown API subsystem
int land_api_subsystem(void) {
    log_this("API", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("API", "LANDING: API", LOG_LEVEL_STATE);

    bool success = true;

    // Step 1: Verify state
    log_this("API", "  Step 1: Verifying state", LOG_LEVEL_STATE);
    if (!is_api_running()) {
        log_this("API", "API already shut down", LOG_LEVEL_STATE);
        log_this("API", "LANDING: API - Already landed", LOG_LEVEL_STATE);
        return 1;
    }
    log_this("API", "    State verified", LOG_LEVEL_STATE);

    // Step 2: Clean up API resources
    log_this("API", "  Step 2: Cleaning up API resources", LOG_LEVEL_STATE);

    // Clean up API endpoints
    cleanup_api_endpoints();
    log_this("API", "    API endpoints cleaned up", LOG_LEVEL_STATE);

    // Step 3: Update registry state
    log_this("API", "  Step 3: Updating registry state", LOG_LEVEL_STATE);
    update_subsystem_on_shutdown("API");

    log_this("API", "LANDING: API - Successfully landed", LOG_LEVEL_STATE);

    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}
