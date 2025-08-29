/*
 * Resources Subsystem Landing (Shutdown) Implementation
 */

// Global includes
#include "../hydrogen.h"

// Local includes
#include "landing.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

/**
 * Check if the Resources subsystem is ready to land
 */
LaunchReadiness check_resources_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Resources";

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Resources");

    // For landing readiness, we only need to verify if the subsystem is running
    bool is_running = is_subsystem_running_by_name("Resources");
    readiness.ready = is_running;

    if (is_running) {
        readiness.messages[1] = strdup("  Go:      Resources subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of Resources");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   Resources not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Resources");
        readiness.messages[3] = NULL;
    }

    return readiness;
}

/**
 * Free resources allocated during resources launch
 */
static void free_resources_resources(void) {
    // Begin LANDING: RESOURCES section
    log_this("Resources", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Resources", "LANDING: RESOURCES", LOG_LEVEL_STATE);
    
    // Clean up resource monitoring threads and data structures
    log_this("Landing", "  Step 1: Stopping resource monitoring threads", LOG_LEVEL_STATE);

    // Note: Resource monitoring cleanup is handled by the threads subsystem
    // as resource monitoring typically runs in a dedicated thread

    log_this("Landing", "  Step 2: Freeing resource configuration", LOG_LEVEL_STATE);
    // Resource configuration is part of app_config and will be cleaned up
    // when the main config is freed during final shutdown

    log_this("Landing", "  Step 3: Clearing resource limits", LOG_LEVEL_STATE);
    // Resource limits are configuration-based, no dynamic cleanup needed

    // Update the registry that resources has been shut down
    update_subsystem_after_shutdown("Resources");
    log_this("Landing", "  Step 4: Resources subsystem marked as inactive", LOG_LEVEL_STATE);

    log_this("Landing", "LANDING: RESOURCES cleanup complete", LOG_LEVEL_STATE);
}

/**
 * Land the resources subsystem
 */
int land_resources_subsystem(void) {
    // Begin LANDING: RESOURCES section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING: RESOURCES", LOG_LEVEL_STATE);

    // Get current subsystem state through registry
    int subsys_id = get_subsystem_id_by_name("Resources");
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this("Landing", "Resources not running, skipping shutdown", LOG_LEVEL_STATE);
        return 1;  // Success - nothing to do
    }

    // Step 1: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this("Landing", "LANDING: RESOURCES - Beginning shutdown sequence", LOG_LEVEL_STATE);

    // Step 2: Free resources and mark as inactive
    free_resources_resources();

    // Step 3: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this("Landing", "LANDING: RESOURCES - Successfully landed and ready for future restart", LOG_LEVEL_STATE);
    } else {
        log_this("Landing", "LANDING: RESOURCES - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
    }

    return 1;  // Success
}
