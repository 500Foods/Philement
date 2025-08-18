/**
 * @file landing_payload.c
 * @brief Payload subsystem landing (shutdown) implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "landing_payload.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../payload/payload.h"
#include "../utils/utils.h"
#include "../state/state_types.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

// Check if the Payload subsystem is ready to land
LaunchReadiness check_payload_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Payload";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Payload");
    
    // For landing readiness, we only need to verify if the subsystem is running
    bool is_running = is_subsystem_running_by_name("Payload");
    readiness.ready = is_running;
    
    if (is_running) {
        readiness.messages[1] = strdup("  Go:      Payload subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of Payload");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   Payload not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Payload");
        readiness.messages[3] = NULL;
    }
    
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
    // Begin LANDING: PAYLOAD section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING: PAYLOAD", LOG_LEVEL_STATE);
    log_this("Landing", "Beginning payload resource cleanup", LOG_LEVEL_STATE);
    
    // Free any resources allocated during payload launch
    log_this("Landing", "  Step 1: Freeing payload resources", LOG_LEVEL_STATE);
    
    // Call the payload cleanup function for OpenSSL
    cleanup_openssl();
    log_this("Landing", "  Step 2: OpenSSL resources cleaned up", LOG_LEVEL_STATE);
    
    // Update the registry that payload has been shut down
    update_subsystem_after_shutdown("Payload");
    log_this("Landing", "  Step 3: Payload subsystem marked as inactive", LOG_LEVEL_STATE);
    
    log_this("Landing", "LANDING: PAYLOAD cleanup complete", LOG_LEVEL_STATE);
}

/**
 * Land the payload subsystem
 * 
 * This function handles the complete shutdown sequence for the payload subsystem.
 * It ensures proper cleanup of resources and updates the subsystem state.
 * 
 * @return int 1 on success, 0 on failure
 */
int land_payload_subsystem(void) {
    // Begin LANDING: PAYLOAD section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING: PAYLOAD", LOG_LEVEL_STATE);
    
    // Get current subsystem state through registry
    int subsys_id = get_subsystem_id_by_name("Payload");
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this("Landing", "Payload not running, skipping shutdown", LOG_LEVEL_STATE);
        return 1;  // Success - nothing to do
    }
    
    // Step 1: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this("Landing", "LANDING: PAYLOAD - Beginning shutdown sequence", LOG_LEVEL_STATE);
    
    // Step 2: Free resources and mark as inactive
    free_payload_resources();
    
    // Step 3: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this("Landing", "LANDING: PAYLOAD - Successfully landed and ready for future restart", LOG_LEVEL_STATE);
    } else {
        log_this("Landing", "LANDING: PAYLOAD - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
    }
    return 1;  // Success
}
