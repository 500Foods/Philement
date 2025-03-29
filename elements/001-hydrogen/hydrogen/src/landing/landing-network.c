/*
 * Landing Network Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the network subsystem.
 * It provides functions for:
 * - Checking network landing readiness
 * - Managing network interface shutdown
 * - Cleaning up network resources
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

// Check if the network subsystem is ready to land
LandingReadiness check_network_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Network";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Network");
    
    // Check if network subsystem is actually running
    if (!is_subsystem_running_by_name("Network")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Network subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Network");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Network subsystem can be shut down at any time
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      Network interfaces ready for shutdown");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Network");
    readiness.messages[3] = NULL;
    
    return readiness;
}