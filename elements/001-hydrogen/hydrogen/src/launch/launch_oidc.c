/*
 * Launch OIDC Subsystem
 *
 * This module handles the initialization of the OIDC subsystem.
 * It provides functions for checking readiness and launching OIDC services.
 */

 // Global includes
 #include <src/hydrogen.h>
 
 // Local includes
 #include "launch.h"
 #include <src/oidc/oidc_service.h>

// Registry ID and cached readiness state
static int oidc_subsystem_id = -1;

// Check if the OIDC subsystem is ready to launch
LaunchReadiness check_oidc_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_OIDC));

    // Register with registry if not already registered
    if (oidc_subsystem_id < 0) {
        oidc_subsystem_id = register_subsystem(SR_OIDC, NULL, NULL, NULL, NULL, NULL);
    }

    // Check configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_OIDC, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Configuration loaded"));

    // Check if registry subsystem is available (explicit dependency verification)
    if (is_subsystem_launchable_by_name("Registry")) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry dependency verified (launchable)"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not launchable (dependency)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_OIDC, .ready = false, .messages = messages };
    }

    // Skip validation if OIDC is disabled
    if (!app_config->oidc.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      OIDC disabled, skipping validation"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of OIDC Subsystem"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_OIDC, .ready = true, .messages = messages };
    }

    // Validate core settings
    if (!app_config->oidc.issuer || strlen(app_config->oidc.issuer) == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   OIDC issuer is required"));
        ready = false;
    } else if (strncmp(app_config->oidc.issuer, "http://", 7) != 0 && strncmp(app_config->oidc.issuer, "https://", 8) != 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid URL format for issuer"));
        ready = false;
    }

    if (!app_config->oidc.client_id || strlen(app_config->oidc.client_id) == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   OIDC client_id is required"));
        ready = false;
    }

    if (!app_config->oidc.client_secret || strlen(app_config->oidc.client_secret) == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   OIDC client_secret is required"));
        ready = false;
    }

    if (app_config->oidc.redirect_uri && (strlen(app_config->oidc.redirect_uri) == 0 ||
        (strncmp(app_config->oidc.redirect_uri, "http://", 7) != 0 && strncmp(app_config->oidc.redirect_uri, "https://", 8) != 0))) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid URL format for redirect_uri"));
        ready = false;
    }

    // Validate port
    if (app_config->oidc.port < MIN_OIDC_PORT || app_config->oidc.port > MAX_OIDC_PORT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid OIDC port %d (must be between %d and %d)",
                app_config->oidc.port, MIN_OIDC_PORT, MAX_OIDC_PORT);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    }

    if (ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Core settings valid"));
    }

    // Validate token settings
    if (app_config->oidc.tokens.access_token_lifetime < MIN_TOKEN_LIFETIME ||
        app_config->oidc.tokens.access_token_lifetime > MAX_TOKEN_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid access token lifetime %d (must be between %d and %d)",
                app_config->oidc.tokens.access_token_lifetime, MIN_TOKEN_LIFETIME, MAX_TOKEN_LIFETIME);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    }

    if (app_config->oidc.tokens.refresh_token_lifetime < MIN_REFRESH_LIFETIME ||
        app_config->oidc.tokens.refresh_token_lifetime > MAX_REFRESH_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid refresh token lifetime %d (must be between %d and %d)",
                app_config->oidc.tokens.refresh_token_lifetime, MIN_REFRESH_LIFETIME, MAX_REFRESH_LIFETIME);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    }

    if (app_config->oidc.tokens.id_token_lifetime < MIN_TOKEN_LIFETIME ||
        app_config->oidc.tokens.id_token_lifetime > MAX_TOKEN_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid ID token lifetime %d (must be between %d and %d)",
                app_config->oidc.tokens.id_token_lifetime, MIN_TOKEN_LIFETIME, MAX_TOKEN_LIFETIME);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    }

    if (ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Token settings valid"));
    }

    // Validate key settings
    if (app_config->oidc.keys.encryption_enabled) {
        if (!app_config->oidc.keys.encryption_key || strlen(app_config->oidc.keys.encryption_key) == 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Encryption key required when encryption is enabled"));
            ready = false;
        }
    }

    if (app_config->oidc.keys.rotation_interval_days < MIN_KEY_ROTATION_DAYS ||
        app_config->oidc.keys.rotation_interval_days > MAX_KEY_ROTATION_DAYS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid key rotation interval %d days (must be between %d and %d)",
                app_config->oidc.keys.rotation_interval_days, MIN_KEY_ROTATION_DAYS, MAX_KEY_ROTATION_DAYS);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        ready = false;
    }

    if (ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Key settings valid"));
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of OIDC Subsystem" :
        "  Decide:  No-Go For Launch of OIDC Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_OIDC,
        .ready = ready,
        .messages = messages
    };
}



/**
 * Launch the OIDC subsystem
 *
 * This function coordinates the launch of the OIDC subsystem by:
 * 1. Verifying explicit dependencies
 * 2. Using the standard launch formatting
 * 3. Initializing OIDC services (if enabled)
 * 4. Updating the subsystem registry with the result
 *
 * Dependencies:
 * - Registry subsystem must be running
 * - Network subsystem should be running for external connectivity
 *
 * @return 1 if OIDC subsystem was successfully launched, 0 on failure
 */
int launch_oidc_subsystem(void) {
    // Begin LAUNCH: OIDC section
    log_this(SR_OIDC, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_OIDC, "LAUNCH: " SR_OIDC, LOG_LEVEL_STATE, 0);

    // Step 1: Verify explicit dependencies
    log_this(SR_OIDC, "  Step 1: Verifying explicit dependencies", LOG_LEVEL_STATE, 0);

    // Check Registry dependency
    if (is_subsystem_running_by_name("Registry")) {
        log_this(SR_OIDC, "    Registry dependency verified (running)", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_OIDC, "    Registry dependency not met", LOG_LEVEL_ERROR, 0);
        log_this(SR_OIDC, "LAUNCH: OIDC - Failed: Registry dependency not met", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Check Network dependency (recommended for OIDC external connectivity)
    if (is_subsystem_running_by_name("Network")) {
        log_this(SR_OIDC, "    Network dependency verified (running)", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_OIDC, "    Network dependency not met - OIDC may have limited external connectivity", LOG_LEVEL_ALERT, 0);
    }

    log_this(SR_OIDC, "    All critical dependencies verified", LOG_LEVEL_STATE, 0);

    // Step 2: Check if OIDC is enabled
    log_this(SR_OIDC, "  Step 2: Checking OIDC configuration", LOG_LEVEL_STATE, 0);
    if (!app_config || !app_config->oidc.enabled) {
        log_this(SR_OIDC, "    OIDC is disabled - skipping service initialization", LOG_LEVEL_STATE, 0);
        log_this(SR_OIDC, "  Step 3: Updating subsystem registry", LOG_LEVEL_STATE, 0);
        update_subsystem_on_startup(SR_OIDC, true);

        SubsystemState final_state = get_subsystem_state(oidc_subsystem_id);
        if (final_state == SUBSYSTEM_RUNNING) {
            log_this(SR_OIDC, "LAUNCH: OIDC - Successfully launched (disabled state)", LOG_LEVEL_STATE, 0);
            return 1;
        } else {
            log_this(SR_OIDC, "LAUNCH: OIDC - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
            return 0;
        }
    }

    // Step 3: Initialize OIDC services
    log_this(SR_OIDC, "  Step 3: Initializing OIDC services", LOG_LEVEL_STATE, 0);

    // Initialize the OIDC service with configuration
    if (!init_oidc_service(&app_config->oidc)) {
        log_this(SR_OIDC, "    Failed to initialize OIDC service", LOG_LEVEL_ERROR, 0);
        log_this(SR_OIDC, "LAUNCH: OIDC - Failed: Service initialization failed", LOG_LEVEL_STATE, 0);
        return 0;
    }

    log_this(SR_OIDC, "    OIDC service initialized successfully", LOG_LEVEL_STATE, 0);
    log_this(SR_OIDC, "    OIDC server running on port %d", LOG_LEVEL_STATE, 1, app_config->oidc.port);
    log_this(SR_OIDC, "    OIDC issuer: %s", LOG_LEVEL_STATE, 1, app_config->oidc.issuer);

    // Step 4: Update registry and verify state
    log_this(SR_OIDC, "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE, 0);
    update_subsystem_on_startup(SR_OIDC, true);

    SubsystemState final_state = get_subsystem_state(oidc_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_OIDC, "LAUNCH: OIDC - Successfully launched and running", LOG_LEVEL_STATE, 0);
        return 1;
    } else {
        log_this(SR_OIDC, "LAUNCH: OIDC - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
