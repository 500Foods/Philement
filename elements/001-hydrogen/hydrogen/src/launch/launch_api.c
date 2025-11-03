/*
 * Launch API Subsystem
 *
 * This module handles the initialization of the API subsystem.
 * It provides functions for checking readiness and launching the API endpoints.
 *
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/api/api_service.h> // For API service initialization

// Registry ID and cached readiness state
int api_subsystem_id = -1;

/*
 * Check if the API subsystem is ready to launch
 *
 * This function performs readiness checks for the API subsystem by:
 * - Verifying system state and dependencies (Registry, WebServer)
 * - Checking API configuration (endpoints, prefix, JWT)
 *
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_api_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_API));

    // Register with registry first
    if (api_subsystem_id < 0) {
        api_subsystem_id = register_subsystem_from_launch(SR_API, NULL, NULL, NULL,
                                            (int (*)(void))launch_api_subsystem,
                                            NULL);  // No special shutdown needed
        if (api_subsystem_id < 0) {
            log_this(SR_API, "Failed to register " SR_API " subsystem", LOG_LEVEL_ERROR, 0);
        }
    }

    if (api_subsystem_id < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register with registry"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_API, .ready = false, .messages = messages };
    }

    // 1. Check Registry subsystem readiness (primary dependency)
    LaunchReadiness registry_readiness = check_registry_launch_readiness();
    if (!registry_readiness.ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not Go for Launch"));
        finalize_launch_messages(&messages, &count, &capacity);
        cleanup_readiness_messages(&registry_readiness);
        return (LaunchReadiness){ .subsystem = SR_API, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry subsystem Go for Launch"));
    cleanup_readiness_messages(&registry_readiness);

    // 4. Check our configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   API configuration not loaded"));
        ready = false;
    } else {
        // Check if API is enabled
        if (!app_config->api.enabled) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   API endpoints disabled in configuration"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      API endpoints enabled"));
        }

        // Check API prefix
        if (!app_config->api.prefix || app_config->api.prefix[0] != '/') {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid API prefix configuration"));
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid API prefix: %s", app_config->api.prefix);
                add_launch_message(&messages, &count, &capacity, msg);
            }
        }

        // Check JWT secret
        if (!app_config->api.jwt_secret || !app_config->api.jwt_secret[0]) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   JWT secret not configured"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      JWT secret configured"));
        }
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of API Subsystem" :
        "  Decide:  No-Go For Launch of API Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    LaunchReadiness readiness = {
        .subsystem = SR_API,
        .ready = ready,
        .messages = NULL
    };

    set_readiness_messages(&readiness, messages);

    return readiness;
}

// Launch API subsystem
int launch_api_subsystem(void) {

    // Step 1: Log Launch
    log_this(SR_API, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_API, "LAUNCH: " SR_API, LOG_LEVEL_DEBUG, 0);

    // Step 2: Check Registration
    if (api_subsystem_id < 0) {
        log_this(SR_API, "    Failed to register " SR_API " subsystem", LOG_LEVEL_ERROR, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: Registration failed", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Step 3: Register Dependencies
    add_subsystem_dependency(api_subsystem_id, "Registry");
    add_subsystem_dependency(api_subsystem_id, "WebServer");

    // Step 4: Verify system state
    if (server_stopping || server_starting != 1) {
        log_this(SR_API, "Cannot initialize " SR_API " subsystem outside startup phase", LOG_LEVEL_DEBUG, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: Not in startup phase", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Step 5: Verify config state
    if (!app_config) {
        log_this(SR_API, "Configuration not loaded", LOG_LEVEL_ERROR, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: No configuration", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this(SR_API, "    Registry not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: Registry dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_API, "    Registry dependency verified", LOG_LEVEL_DEBUG, 0);

    // Then check WebServer
    if (!is_subsystem_running_by_name(SR_WEBSERVER)) {
        log_this(SR_API, "    " SR_WEBSERVER " not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: " SR_WEBSERVER " subsystem dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_API, "    " SR_WEBSERVER " subsystem dependency verified", LOG_LEVEL_DEBUG, 0);
    log_this(SR_API, "    All dependencies verified", LOG_LEVEL_DEBUG, 0);

    // Step 4: Initialize API endpoints
    log_this(SR_API, "  Initializing " SR_API " endpoints", LOG_LEVEL_DEBUG, 0);

    // Initialize and register API endpoints
    if (!init_api_endpoints()) {
        log_this(SR_API, "    Failed to initialize " SR_API " endpoints", LOG_LEVEL_ERROR, 0);
        log_this(SR_API, "LAUNCH: " SR_API " Failed: Endpoint initialization failed", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    log_this(SR_API, "    " SR_API " endpoints initialized successfully", LOG_LEVEL_DEBUG, 0);

    // Step 5: Update registry and verify state
    log_this(SR_API, "  Updating " SR_REGISTRY " subsystem", LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_API, true);

    SubsystemState final_state = get_subsystem_state(api_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_API, "LAUNCH: " SR_API " Success: Launched and running", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_API, "LAUNCH: " SR_API " Warning: Unexpected final state: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
        return 0;
    }

    return 1;
}
