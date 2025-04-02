/*
 * Landing Network Subsystem
 * 
 * DESIGN PRINCIPLES:
 * - This file handles network-specific landing logic
 * - Follows standard landing patterns
 * - Manages clean network shutdown
 * - Reports detailed status
 *
 * This module provides two main functions:
 * 1. check_network_landing_readiness - Verifies if network can be landed
 * 2. land_network_subsystem - Performs actual network shutdown
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Project includes
#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../network/network.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"
#include "../utils/utils_logging.h"

/*
 * Land the network subsystem
 * Returns 1 on success, 0 on failure
 */
int land_network_subsystem(void) {
    log_this("Landing", "Landing network subsystem...", LOG_LEVEL_STATE);
    
    // Get subsystem ID
    int subsystem_id = get_subsystem_id_by_name("Network");
    if (subsystem_id < 0) {
        log_this("Landing", "Failed to get network subsystem ID", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Verify subsystem is in a state that can be landed
    if (!is_subsystem_running_by_name("Network")) {
        log_this("Landing", "Network subsystem is not running", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Close all network interfaces
    if (!network_shutdown()) {
        log_this("Landing", "Failed to shut down network interfaces", LOG_LEVEL_ERROR);
        return 0;
    }
    
    log_this("Landing", "Network subsystem landed successfully", LOG_LEVEL_STATE);
    return 1;
}

// Check if the network subsystem is ready to land
LaunchReadiness check_network_landing_readiness(void) {
    LaunchReadiness readiness = {0};
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