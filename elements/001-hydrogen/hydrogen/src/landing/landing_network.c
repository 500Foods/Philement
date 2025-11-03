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

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

/*
 * Land the network subsystem
 * Returns 1 on success, 0 on failure
 */
int land_network_subsystem(void) {
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING: " SR_NETWORK, LOG_LEVEL_DEBUG, 0);
    
    // Step 1: Verify subsystem state
    log_this(SR_LANDING, "Verifying subsystem state", LOG_LEVEL_DEBUG, 0);
    int subsystem_id = get_subsystem_id_by_name(SR_NETWORK);
    if (subsystem_id < 0) {
        log_this(SR_LANDING, "LANDING: " SR_NETWORK " Failed to land", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    
    SubsystemState state = get_subsystem_state(subsystem_id);
    if (state != SUBSYSTEM_RUNNING && state != SUBSYSTEM_STOPPING) {
        log_this(SR_LANDING, "Network subsystem is in invalid state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(state));
        log_this(SR_LANDING, "LANDING: NETWORK - Failed to land", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    
    // Step 2: Final interface tests
    log_this(SR_LANDING, "Performing final interface tests", LOG_LEVEL_DEBUG, 0);
    network_info_t *info = get_network_info();
    if (info) {
        test_network_interfaces(info);
        free_network_info(info);
    }
    
    // Step 3: Shutdown network interfaces
    log_this(SR_LANDING, "Shutting down network interfaces", LOG_LEVEL_DEBUG, 0);
    if (!network_shutdown()) {
        log_this(SR_LANDING, "LANDING: NETWORK - Failed to land", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    
    // Step 4: Update subsystem registry
    log_this(SR_LANDING, "  Step 4: Updating subsystem registry", LOG_LEVEL_DEBUG, 0);
    update_subsystem_after_shutdown("Network");
    
    log_this(SR_LANDING, "LANDING: " SR_LANDING " COMPLETE", LOG_LEVEL_DEBUG, 0);
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

// Network subsystem shutdown function
void shutdown_network_subsystem(void) {
    log_this(SR_NETWORK, "Shutting down network subsystem", LOG_LEVEL_DEBUG, 0);
    // No specific shutdown actions needed for network subsystem
}