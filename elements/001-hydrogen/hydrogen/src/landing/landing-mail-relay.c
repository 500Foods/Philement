/*
 * Landing Mail Relay Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the mail relay subsystem.
 * It provides functions for:
 * - Checking mail relay landing readiness
 * - Managing mail relay shutdown
 * - Cleaning up mail relay resources
 * 
 * Dependencies:
 * - Must coordinate with Network subsystem for clean shutdown
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations
extern volatile sig_atomic_t mail_relay_system_shutdown;

// Check if the mail relay subsystem is ready to land
LandingReadiness check_mail_relay_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Mail Relay";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Mail Relay");
    
    // Check if mail relay is actually running
    if (!is_subsystem_running_by_name("MailRelay")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Mail Relay not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Mail Relay");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check Network subsystem status
    bool network_ready = is_subsystem_running_by_name("Network");
    if (!network_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Network subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Mail Relay");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // All checks passed
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      Mail service ready for shutdown");
    readiness.messages[2] = strdup("  Go:      Network subsystem ready");
    readiness.messages[3] = strdup("  Decide:  Go For Landing of Mail Relay");
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Shutdown the mail relay subsystem
void shutdown_mail_relay(void) {
    log_this("Mail Relay", "Beginning Mail Relay shutdown sequence", LOG_LEVEL_STATE);
    
    // Signal shutdown
    mail_relay_system_shutdown = 1;
    log_this("Mail Relay", "Signaled Mail Relay to stop", LOG_LEVEL_STATE);
    
    // Cleanup resources
    // Additional cleanup will be added as needed
    
    log_this("Mail Relay", "Mail Relay shutdown complete", LOG_LEVEL_STATE);
}