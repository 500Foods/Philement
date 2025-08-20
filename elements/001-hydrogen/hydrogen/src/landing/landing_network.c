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
#include "../hydrogen.h"

// Local includes
#include "landing.h"

/*
 * Land the network subsystem
 * Returns 1 on success, 0 on failure
 */
int land_network_subsystem(void) {
    log_this("Landing", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Landing", "LANDING: NETWORK", LOG_LEVEL_STATE);
    
    // Step 1: Verify subsystem state
    log_this("Landing", "  Step 1: Verifying subsystem state", LOG_LEVEL_STATE);
    int subsystem_id = get_subsystem_id_by_name("Network");
    if (subsystem_id < 0) {
        log_this("Landing", "Failed to get network subsystem ID", LOG_LEVEL_ALERT);
        log_this("Landing", "LANDING: NETWORK - Failed to land", LOG_LEVEL_STATE);
        return 0;
    }
    
    SubsystemState state = get_subsystem_state(subsystem_id);
    if (state != SUBSYSTEM_RUNNING && state != SUBSYSTEM_STOPPING) {
        log_this("Landing", "Network subsystem is in invalid state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(state));
        log_this("Landing", "LANDING: NETWORK - Failed to land", LOG_LEVEL_STATE);
        return 0;
    }
    
    // Step 2: Final interface tests
    log_this("Landing", "  Step 2: Performing final interface tests", LOG_LEVEL_STATE);
    network_info_t *info = get_network_info();
    if (info) {
        test_network_interfaces(info);
        free_network_info(info);
    }
    
    // Step 3: Shutdown network interfaces
    log_this("Landing", "  Step 3: Shutting down network interfaces", LOG_LEVEL_STATE);
    if (!network_shutdown()) {
        log_this("Landing", "Failed to shut down network interfaces", LOG_LEVEL_ALERT);
        log_this("Landing", "LANDING: NETWORK - Failed to land", LOG_LEVEL_STATE);
        return 0;
    }
    
    // Step 4: Update subsystem registry
    log_this("Landing", "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_after_shutdown("Network");
    
    log_this("Landing", "LANDING: NETWORK - Successfully landed", LOG_LEVEL_STATE);
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
