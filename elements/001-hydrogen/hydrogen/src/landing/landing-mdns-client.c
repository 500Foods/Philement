/*
 * Landing mDNS Client Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the mDNS client subsystem.
 * It provides functions for:
 * - Checking mDNS client landing readiness
 * - Managing service discovery shutdown
 * - Cleaning up mDNS client resources
 * 
 * Dependencies:
 * - Must coordinate with Network subsystem for clean shutdown
 * - Requires Logging system to be operational
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations
extern volatile sig_atomic_t mdns_client_system_shutdown;

// Check if the mDNS client subsystem is ready to land
LaunchReadiness check_mdns_client_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "mDNS Client";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("mDNS Client");
    
    // Check if mDNS client is actually running
    if (!is_subsystem_running_by_name("mDNSClient")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   mDNS Client not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of mDNS Client");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check Network subsystem status
    bool network_ready = is_subsystem_running_by_name("Network");
    if (!network_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Network subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of mDNS Client");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check Logging subsystem status
    bool logging_ready = is_subsystem_running_by_name("Logging");
    if (!logging_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Logging subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of mDNS Client");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // All checks passed
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      Service discovery ready for shutdown");
    readiness.messages[2] = strdup("  Go:      Network subsystem ready");
    readiness.messages[3] = strdup("  Go:      Logging subsystem ready");
    readiness.messages[4] = strdup("  Decide:  Go For Landing of mDNS Client");
    readiness.messages[5] = NULL;
    
    return readiness;
}

// Land the mDNS client subsystem
int land_mdns_client_subsystem(void) {
    log_this("mDNS Client", "Beginning mDNS Client shutdown sequence", LOG_LEVEL_STATE);
    bool success = true;
    
    // Signal shutdown
    mdns_client_system_shutdown = 1;
    log_this("mDNS Client", "Signaled mDNS Client to stop", LOG_LEVEL_STATE);
    
    // Stop service discovery
    log_this("mDNS Client", "Stopping service discovery", LOG_LEVEL_STATE);
    
    // Additional cleanup will be added as needed
    
    log_this("mDNS Client", "mDNS Client shutdown complete", LOG_LEVEL_STATE);
    
    return success ? 1 : 0;  // Return 1 for success, 0 for failure
}