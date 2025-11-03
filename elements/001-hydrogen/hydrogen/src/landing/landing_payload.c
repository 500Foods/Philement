/**
 * @file landing_payload.c
 * @brief Payload subsystem landing (shutdown) implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

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
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING: PAYLOAD", LOG_LEVEL_DEBUG, 0);
    
    // Free any resources allocated during payload launch
    log_this(SR_LANDING, "  Step 1: Freeing payload resources", LOG_LEVEL_DEBUG, 0);
    
    // Call the payload cleanup function for OpenSSL
    cleanup_openssl();
    log_this(SR_LANDING, "  Step 2: OpenSSL resources cleaned up", LOG_LEVEL_DEBUG, 0);
    
    // Update the registry that payload has been shut down
    update_subsystem_after_shutdown("Payload");
    log_this(SR_LANDING, "  Step 3: Payload subsystem marked as inactive", LOG_LEVEL_DEBUG, 0);
    
    log_this(SR_LANDING, "LANDING: PAYLOAD cleanup complete", LOG_LEVEL_DEBUG, 0);
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
    log_this(SR_PAYLOAD, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_PAYLOAD, "LANDING: " SR_PAYLOAD, LOG_LEVEL_DEBUG, 0);
    
    // Get current subsystem state through registry
    int subsys_id = get_subsystem_id_by_name("Payload");
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this(SR_PAYLOAD, "Payload not running, skipping shutdown", LOG_LEVEL_DEBUG, 0);
        return 1;  // Success - nothing to do
    }
    
    // Step 1: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this(SR_PAYLOAD, "LANDING: " SR_PAYLOAD " Beginning shutdown sequence", LOG_LEVEL_DEBUG, 0);
    
    // Step 2: Free resources and mark as inactive
    free_payload_resources();
    
    // Step 3: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this(SR_PAYLOAD, "LANDING: " SR_PAYLOAD " COMPLETE", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_PAYLOAD, "LANDING: " SR_PAYLOAD " Warning: Unexpected final state: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
    }
    return 1;  // Success
}
