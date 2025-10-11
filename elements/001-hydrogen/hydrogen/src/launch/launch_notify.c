/*
 * Launch Notify Subsystem
 *
 * This module handles the initialization of the notify subsystem.
 * It provides functions for checking readiness and launching notification services.
 */

 // Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Registry ID and cached readiness state
static int notify_subsystem_id = -1;

// Check if the notify subsystem is ready to launch
LaunchReadiness check_notify_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_NOTIFY));

    // Register with registry if not already registered
    if (notify_subsystem_id < 0) {
        notify_subsystem_id = register_subsystem(SR_NOTIFY, NULL, NULL, NULL, NULL, NULL);
    }

    // Check configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NOTIFY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Configuration loaded"));

    // Check if registry subsystem is available (explicit dependency verification)
    if (is_subsystem_launchable_by_name(SR_REGISTRY)) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry dependency verified (launchable)"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not launchable (dependency)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NOTIFY, .ready = false, .messages = messages };
    }

    // Skip validation if notify is disabled
    if (!app_config->notify.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Notify disabled, skipping validation"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Notify Subsystem"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NOTIFY, .ready = true, .messages = messages };
    }

    // Validate notifier type
    if (!app_config->notify.notifier || (!app_config->notify.notifier[0])) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Notifier type is required when notify is enabled"));
        ready = false;
    } else if (strcmp(app_config->notify.notifier, "SMTP") != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Unsupported notifier type: %s", app_config->notify.notifier);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Notifier type valid"));
    }

    // If SMTP notifier is configured, validate SMTP settings
    if (app_config->notify.notifier && strcmp(app_config->notify.notifier, "SMTP") == 0) {
        // Validate SMTP host
        if (!app_config->notify.smtp.host || strlen(app_config->notify.smtp.host) == 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   SMTP host is required"));
            ready = false;
        }

        // Validate SMTP port
        if (app_config->notify.smtp.port < MIN_SMTP_PORT || app_config->notify.smtp.port > MAX_SMTP_PORT) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP port %d (must be between %d and %d)",
                    app_config->notify.smtp.port, MIN_SMTP_PORT, MAX_SMTP_PORT);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            ready = false;
        }

        // Validate SMTP timeout
        if (app_config->notify.smtp.timeout < MIN_SMTP_TIMEOUT || app_config->notify.smtp.timeout > MAX_SMTP_TIMEOUT) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP timeout %d (must be between %d and %d)",
                    app_config->notify.smtp.timeout, MIN_SMTP_TIMEOUT, MAX_SMTP_TIMEOUT);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            ready = false;
        }

        // Validate SMTP max retries
        if (app_config->notify.smtp.max_retries < MIN_SMTP_RETRIES || app_config->notify.smtp.max_retries > MAX_SMTP_RETRIES) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP max retries %d (must be between %d and %d)",
                    app_config->notify.smtp.max_retries, MIN_SMTP_RETRIES, MAX_SMTP_RETRIES);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            ready = false;
        }

        // Validate SMTP from address
        if (!app_config->notify.smtp.from_address || strlen(app_config->notify.smtp.from_address) == 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   SMTP from address is required"));
            ready = false;
        }

        if (ready) {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      SMTP settings valid"));
        }
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of Notify Subsystem" :
        "  Decide:  No-Go For Launch of Notify Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_NOTIFY,
        .ready = ready,
        .messages = messages
    };
}



/**
 * Launch the Notify subsystem
 *
 * This function coordinates the launch of the Notify subsystem by:
 * 1. Verifying explicit dependencies
 * 2. Using the standard launch formatting
 * 3. Initializing notification services (if enabled)
 * 4. Updating the subsystem registry with the result
 *
 * Dependencies:
 * - Registry subsystem must be running
 * - Network subsystem should be running for SMTP connectivity
 *
 * @return 1 if Notify subsystem was successfully launched, 0 on failure
 */

int launch_notify_subsystem(void) {
    // Begin LAUNCH: NOTIFY section
    log_this(SR_NOTIFY, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_NOTIFY, "LAUNCH: " SR_NOTIFY, LOG_LEVEL_STATE, 0);

    // Step 1: Verify explicit dependencies
    log_this(SR_NOTIFY, "  Step 1: Verifying explicit dependencies", LOG_LEVEL_STATE, 0);

    // Check Registry dependency
    if (is_subsystem_running_by_name("Registry")) {
        log_this(SR_NOTIFY, "    Registry dependency verified (running)", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_NOTIFY, "    Registry dependency not met", LOG_LEVEL_ERROR, 0);
        log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Failed: Registry dependency not met", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Check Network dependency (recommended for SMTP connectivity)
    if (is_subsystem_running_by_name("Network")) {
        log_this(SR_NOTIFY, "    Network dependency verified (running)", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_NOTIFY, "    Network dependency not met - SMTP notifications may not work", LOG_LEVEL_ALERT, 0);
    }

    log_this(SR_NOTIFY, "    All critical dependencies verified", LOG_LEVEL_STATE, 0);

    // Step 2: Check if Notify is enabled
    log_this(SR_NOTIFY, "  Step 2: Checking Notify configuration", LOG_LEVEL_STATE, 0);
    if (!app_config || !app_config->notify.enabled) {
        log_this(SR_NOTIFY, "    Notify is disabled - skipping service initialization", LOG_LEVEL_STATE, 0);
        log_this(SR_NOTIFY, "  Step 3: Updating subsystem registry", LOG_LEVEL_STATE, 0);
        update_subsystem_on_startup(SR_NOTIFY, true);

        SubsystemState final_state = get_subsystem_state(notify_subsystem_id);
        if (final_state == SUBSYSTEM_RUNNING) {
            log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Successfully launched (disabled state)", LOG_LEVEL_STATE, 0);
            return 1;
        } else {
            log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
            return 0;
        }
    }

    // Step 3: Initialize Notify services
    log_this(SR_NOTIFY, "  Step 3: Initializing Notify services", LOG_LEVEL_STATE, 0);

    // Validate notifier type
    if (!app_config->notify.notifier || !app_config->notify.notifier[0] || strcmp(app_config->notify.notifier, "SMTP") != 0) {
        log_this(SR_NOTIFY, "    Unsupported notifier type: %s", LOG_LEVEL_ERROR, 1, app_config->notify.notifier ? app_config->notify.notifier : "NULL");
        log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Failed: Unsupported notifier type", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // TODO: Add actual notification service initialization here when implementation is ready
    // For now, we log that the service would be initialized
    log_this(SR_NOTIFY, "    Notify service initialization placeholder", LOG_LEVEL_STATE, 0);
    log_this(SR_NOTIFY, "    SMTP notifier configured", LOG_LEVEL_STATE, 0);
    log_this(SR_NOTIFY, "    SMTP host: %s", LOG_LEVEL_STATE, 1, app_config->notify.smtp.host);
    log_this(SR_NOTIFY, "    SMTP port: %d", LOG_LEVEL_STATE, 1, app_config->notify.smtp.port);
    log_this(SR_NOTIFY, "    From address: %s", LOG_LEVEL_STATE, 1, app_config->notify.smtp.from_address);

    // Step 4: Update registry and verify state
    log_this(SR_NOTIFY, "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE, 0);
    update_subsystem_on_startup(SR_NOTIFY, true);

    SubsystemState final_state = get_subsystem_state(notify_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Successfully launched and running", LOG_LEVEL_STATE, 0);
        return 1;
    } else {
        log_this(SR_NOTIFY, "LAUNCH: NOTIFY - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
