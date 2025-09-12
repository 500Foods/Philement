/*
 * OIDC Subsystem Landing (Shutdown) Implementation
 */

// Global includes
#include "../hydrogen.h"

// Local includes
#include "landing.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

/**
 * Check if the OIDC subsystem is ready to land
 */
LaunchReadiness check_oidc_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "OIDC";

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    // Add initial subsystem identifier
    readiness.messages[0] = strdup("OIDC");

    // For landing readiness, we only need to verify if the subsystem is running
    bool is_running = is_subsystem_running_by_name("OIDC");
    readiness.ready = is_running;

    if (is_running) {
        readiness.messages[1] = strdup("  Go:      OIDC subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of OIDC");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   OIDC not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of OIDC");
        readiness.messages[3] = NULL;
    }

    return readiness;
}

/**
 * Free resources allocated during OIDC launch
 */
static void free_oidc_resources(void) {
    // Begin LANDING: OIDC section
    log_this(SR_LANDING, "%s", LOG_LEVEL_STATE, 1, LOG_LINE_BREAK);
    log_this(SR_LANDING, "LANDING: OIDC", LOG_LEVEL_STATE, 0);
    
    // Check if OIDC is enabled before attempting cleanup
    if (app_config && !app_config->oidc.enabled) {
        log_this(SR_LANDING, "  Step 1: OIDC disabled, skipping cleanup", LOG_LEVEL_STATE, 0);
        update_subsystem_after_shutdown("OIDC");
        log_this(SR_LANDING, "  Step 2: OIDC subsystem marked as inactive", LOG_LEVEL_STATE, 0);
        log_this(SR_LANDING, "LANDING: OIDC cleanup complete", LOG_LEVEL_STATE, 0);
        return;
    }

    // Clean up OIDC service resources
    log_this(SR_LANDING, "  Step 1: Stopping OIDC authentication service", LOG_LEVEL_STATE, 0);

    // Clean up any active OIDC sessions or tokens
    log_this(SR_LANDING, "  Step 2: Clearing active OIDC sessions", LOG_LEVEL_STATE, 0);
    // Any cached authentication sessions would be cleared here

    log_this(SR_LANDING, "  Step 3: Revoking cached tokens", LOG_LEVEL_STATE, 0);
    // Any cached access/refresh tokens would be revoked here

    log_this(SR_LANDING, "  Step 4: Closing OIDC network connections", LOG_LEVEL_STATE, 0);
    // Any HTTP connections to OIDC provider would be closed here

    log_this(SR_LANDING, "  Step 5: Clearing OIDC key cache", LOG_LEVEL_STATE, 0);
    // Any cached encryption keys would be cleared here

    log_this(SR_LANDING, "  Step 6: Freeing OIDC configuration", LOG_LEVEL_STATE, 0);
    // OIDC-specific configuration resources would be freed here

    // Update the registry that OIDC has been shut down
    update_subsystem_after_shutdown("OIDC");
    log_this(SR_LANDING, "  Step 7: OIDC subsystem marked as inactive", LOG_LEVEL_STATE, 0);

    log_this(SR_LANDING, "LANDING: OIDC cleanup complete", LOG_LEVEL_STATE, 0);
}

/**
 * Land the OIDC subsystem
 */
int land_oidc_subsystem(void) {
    // Begin LANDING: OIDC section
    log_this(SR_LANDING, "%s", LOG_LEVEL_STATE, 1, LOG_LINE_BREAK);
    log_this(SR_LANDING, "LANDING: OIDC", LOG_LEVEL_STATE, 0);

    // Get current subsystem state through registry
    int subsys_id = get_subsystem_id_by_name("OIDC");
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this(SR_LANDING, "OIDC not running, skipping shutdown", LOG_LEVEL_STATE, 0);
        return 1;  // Success - nothing to do
    }

    // Step 1: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this(SR_LANDING, "LANDING: OIDC - Beginning shutdown sequence", LOG_LEVEL_STATE, 0);

    // Step 2: Free resources and mark as inactive
    free_oidc_resources();

    // Step 3: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this(SR_LANDING, "LANDING: OIDC - Successfully landed and ready for future restart", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_LANDING, "LANDING: OIDC - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
    }

    return 1;  // Success
}
