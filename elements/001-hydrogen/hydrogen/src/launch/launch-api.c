/*
 * Launch API Subsystem
 * 
 * This module handles the initialization and shutdown of the API subsystem.
 * It provides functions for checking readiness and managing the API lifecycle.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "launch.h"
#include "../utils/utils_logging.h"

// Check if the API subsystem is ready to launch
LaunchReadiness check_api_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Always ready - API is part of the WebServer
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("API");
    readiness.messages[1] = strdup("  Go:      API Routes Configured");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of API");
    readiness.messages[3] = NULL;
    
    return readiness;
}