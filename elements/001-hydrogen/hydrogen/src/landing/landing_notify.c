/*
 * Notify Subsystem Landing (Shutdown) Implementation
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

/**
 * Check if the Notify subsystem is ready to land
 */
LaunchReadiness check_notify_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Notify";

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Notify");

    // For landing readiness, we only need to verify if the subsystem is running
    bool is_running = is_subsystem_running_by_name("Notify");
    readiness.ready = is_running;

    if (is_running) {
        readiness.messages[1] = strdup("  Go:      Notify subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of Notify");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   Notify not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Notify");
        readiness.messages[3] = NULL;
    }

    return readiness;
}

/**
 * Free resources allocated during notify launch
 */
static void free_notify_resources(void) {
    // Begin LANDING: NOTIFY section
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING: NOTIFY", LOG_LEVEL_DEBUG, 0);

    // Check if notify is enabled before attempting cleanup
    if (app_config && !app_config->notify.enabled) {
        log_this(SR_LANDING, "  Step 1: Notify disabled, skipping cleanup", LOG_LEVEL_DEBUG, 0);
        update_subsystem_after_shutdown("Notify");
        log_this(SR_LANDING, "  Step 2: Notify subsystem marked as inactive", LOG_LEVEL_DEBUG, 0);
        log_this(SR_LANDING, "LANDING: NOTIFY cleanup complete", LOG_LEVEL_DEBUG, 0);
        return;
    }

    // Clean up notification service resources
    log_this(SR_LANDING, "  Step 1: Stopping notification service", LOG_LEVEL_DEBUG, 0);

    // Note: If SMTP connections or notification queues were created,
    // they would be cleaned up here. Currently the notify subsystem
    // only initializes configuration, so minimal cleanup is needed.

    log_this(SR_LANDING, "  Step 2: Clearing notification templates", LOG_LEVEL_DEBUG, 0);
    // Any cached notification templates would be freed here

    log_this(SR_LANDING, "  Step 3: Closing notification connections", LOG_LEVEL_DEBUG, 0);
    // Any persistent connections (SMTP, etc.) would be closed here

    // Update the registry that notify has been shut down
    update_subsystem_after_shutdown("Notify");
    log_this(SR_LANDING, "  Step 4: Notify subsystem marked as inactive", LOG_LEVEL_DEBUG, 0);

    log_this(SR_LANDING, "LANDING: NOTIFY cleanup complete", LOG_LEVEL_DEBUG, 0);
}

/**
 * Land the notify subsystem
 */
int land_notify_subsystem(void) {
    // Begin LANDING: NOTIFY section
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING: NOTIFY", LOG_LEVEL_DEBUG, 0);

    // Get current subsystem state through registry
    int subsys_id = get_subsystem_id_by_name("Notify");
    if (subsys_id < 0 || !is_subsystem_running(subsys_id)) {
        log_this(SR_LANDING, "Notify not running, skipping shutdown", LOG_LEVEL_DEBUG, 0);
        return 1;  // Success - nothing to do
    }

    // Step 1: Mark as stopping
    update_subsystem_state(subsys_id, SUBSYSTEM_STOPPING);
    log_this(SR_LANDING, "LANDING: NOTIFY - Beginning shutdown sequence", LOG_LEVEL_DEBUG, 0);

    // Step 2: Free resources and mark as inactive
    free_notify_resources();

    // Step 3: Verify final state for restart capability
    SubsystemState final_state = get_subsystem_state(subsys_id);
    if (final_state == SUBSYSTEM_INACTIVE) {
        log_this(SR_LANDING, "LANDING: NOTIFY - Successfully landed and ready for future restart", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_LANDING, "LANDING: NOTIFY - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
    }

    return 1;  // Success
}
