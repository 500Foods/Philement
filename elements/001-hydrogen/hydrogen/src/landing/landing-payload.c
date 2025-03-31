/**
 * @file landing-payload.c
 * @brief Payload subsystem landing (shutdown) implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "landing-payload.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../payload/payload.h"
#include "../utils/utils.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

// Check if the Payload subsystem is ready to land
LandingReadiness check_payload_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Payload";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Payload");
    
    // Check if payload is actually running
    if (!is_subsystem_running_by_name("Payload")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Payload not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Payload");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check if system is in shutdown state
    if (!server_stopping) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   System not in shutdown state");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Payload");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // All checks passed
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      No active payload operations");
    readiness.messages[2] = strdup("  Go:      Ready for OpenSSL cleanup");
    readiness.messages[3] = strdup("  Decide:  Go For Landing of Payload");
    readiness.messages[4] = NULL;
    
    return readiness;
}

/**
 * Free resources allocated during payload launch
 * 
 * This function frees any resources allocated during the payload launch phase.
 * It should be called during the LANDING: PAYLOAD phase of the application.
 * After freeing resources, it unregisters the Payload subsystem to prevent
 * it from being stopped again during the LANDING: SUBSYSTEM REGISTRY phase.
 */
void free_payload_resources(void) {
    // Log the start of payload cleanup
    log_this("Payload", "Beginning payload resource cleanup", LOG_LEVEL_STATE);
    
    // Free any resources allocated during payload launch
    log_this("Payload", "Freeing payload resources", LOG_LEVEL_STATE);
    
    // Call the payload cleanup function for OpenSSL
    cleanup_openssl();
    log_this("Payload", "OpenSSL resources cleaned up", LOG_LEVEL_STATE);
    
    // Update the Payload subsystem state to inactive to prevent it from being stopped again
    // during the LANDING: SUBSYSTEM REGISTRY phase
    int subsys_id = get_subsystem_id_by_name("Payload");
    if (subsys_id >= 0) {
        update_subsystem_state(subsys_id, SUBSYSTEM_INACTIVE);
        log_this("Payload", "Payload subsystem marked as inactive", LOG_LEVEL_STATE);
    }
    
log_this("Payload", "Payload cleanup complete", LOG_LEVEL_STATE);
}

/**
 * Shutdown the payload subsystem
 * 
 * This function handles the complete shutdown sequence for the payload subsystem.
 * It ensures proper cleanup of resources and updates the subsystem state.
 */
void shutdown_payload(void) {
    log_this("Payload", "Beginning Payload shutdown sequence", LOG_LEVEL_STATE);
    
    // Check if payload is running
    if (!is_subsystem_running_by_name("Payload")) {
        log_this("Payload", "Payload not running, skipping shutdown", LOG_LEVEL_STATE);
        return;
    }
    
    // Free payload resources (includes OpenSSL cleanup)
    free_payload_resources();
    
    log_this("Payload", "Payload shutdown complete", LOG_LEVEL_STATE);
}