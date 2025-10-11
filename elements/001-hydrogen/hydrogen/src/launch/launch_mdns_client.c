/*
 * Launch mDNS Client Subsystem
 * 
 * This module handles the initialization of the mDNS client subsystem.
 * It provides functions for checking readiness and launching the mDNS client.
 * 
 * The mDNS Client system enables discovery of network services:
 * 1. Discover other printers on the network
 * 2. Find available print services
 * 3. Locate network resources
 * 4. Enable auto-configuration
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * - Logging system must be operational
 * 
 * Note: Shutdown functionality has been moved to landing/landing_mdns_client.c
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Registry ID and cached readiness state
int mdns_client_subsystem_id = -1;

// Forward declarations
static void register_mdns_client_for_launch(void);

// Register the mDNS client subsystem with the registry (for launch)
static void register_mdns_client_for_launch(void) {
    // Always register during readiness check if not already registered
    if (mdns_client_subsystem_id < 0) {
        // Register with the launch system
        mdns_client_subsystem_id = register_subsystem_from_launch(SR_MDNS_CLIENT, NULL, NULL,
                                                                &mdns_client_system_shutdown,
                                                                (int (*)(void))launch_mdns_client_subsystem,
                                                                NULL);  // Client has no shutdown function yet
    }
}

// Check if the mDNS client subsystem is ready to launch
LaunchReadiness check_mdns_client_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_MDNS_CLIENT));

    // Register with registry for launch
    register_mdns_client_for_launch();

    // Register dependency on Network subsystem
    int mdns_id = get_subsystem_id_by_name(SR_MDNS_CLIENT);
    if (mdns_id >= 0) {
        if (!add_dependency_from_launch(mdns_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is ready to launch
        if (!is_subsystem_launchable_by_name(SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not launchable"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem will be available"));
    }

    // Check basic configuration
    if (!app_config || !(app_config->mdns_client.enable_ipv4 || app_config->mdns_client.enable_ipv6)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   mDNS client disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      mDNS client enabled in configuration"));

    // Validate configuration values
    const MDNSClientConfig* config = &app_config->mdns_client;

    // Validate intervals
    if (config->scan_interval <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid scan interval (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Scan interval valid"));

    if (config->health_check_enabled && config->health_check_interval <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid health check interval (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Health check interval valid"));

    // Validate service types if configured
    if (config->num_service_types > 0) {
        if (!config->service_types) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Service types array is NULL but count is non-zero"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Service types array allocated"));

        // Validate each service type
        for (size_t i = 0; i < config->num_service_types; i++) {
            char msg_buffer[256];

            if (!config->service_types[i].type || !config->service_types[i].type[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Invalid service type at index %zu (must not be empty)", i);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = SR_MDNS_CLIENT, .ready = false, .messages = messages };
            }

            snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Service type %zu validated", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
        }
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of mDNS Client Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_MDNS_CLIENT,
        .ready = ready,
        .messages = messages
    };
}

// Launch the mDNS client subsystem
int launch_mdns_client_subsystem(void) {

    mdns_client_system_shutdown = 0;
    
    log_this(SR_MDNS_CLIENT, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MDNS_CLIENT, "LAUNCH: " SR_MDNS_CLIENT, LOG_LEVEL_STATE, 0);

    // Register with subsystem registry (from launch) - critical for dependencies to work
    if (mdns_client_subsystem_id < 0) {
        mdns_client_subsystem_id = register_subsystem_from_launch(SR_MDNS_CLIENT, NULL, NULL,
                                                                &mdns_client_system_shutdown,
                                                                (int (*)(void))launch_mdns_client_subsystem,
                                                                NULL);  // Client has no shutdown function yet
        if (mdns_client_subsystem_id < 0) {
            log_this(SR_MDNS_CLIENT, "Failed to register mDNS Client subsystem for launch", LOG_LEVEL_ERROR, 0);
            log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Failed: Registration error", LOG_LEVEL_STATE, 0);
            return 0;
        }
    }
    
    // Initialize mDNS client system
    // TODO: Add proper initialization when system is ready - for now just mark as successful
    // This will be implemented when the actual mDNS client functionality is added

    log_this(SR_MDNS_CLIENT, "Initializing mDNS client system", LOG_LEVEL_STATE, 0);

    // Step 1: Verify system state
    if (server_stopping || server_starting != 1) {
        log_this(SR_MDNS_CLIENT, "Cannot initialize mDNS client outside startup phase", LOG_LEVEL_STATE, 0);
        log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Failed: Not in startup phase", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!app_config) {
        log_this(SR_MDNS_CLIENT, "Configuration not loaded", LOG_LEVEL_ERROR, 0);
        log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Failed: No configuration", LOG_LEVEL_STATE, 0);
        return 0;
    }

    if (!app_config->mdns_client.enable_ipv4 && !app_config->mdns_client.enable_ipv6) {
        log_this(SR_MDNS_CLIENT, "mDNS client disabled in configuration (no protocols enabled)", LOG_LEVEL_STATE, 0);
        log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Disabled by configuration", LOG_LEVEL_STATE, 0);
        return 1; // Not an error if disabled
    }

    // Step 2: Initialize mDNS client
    log_this(SR_MDNS_CLIENT, "Starting mDNS client service discovery", LOG_LEVEL_STATE, 0);

    // TODO: Implement actual mDNS client initialization when functionality is available
    // For now, just register success

    // Step 3: Update subsystem registry
    update_subsystem_on_startup(SR_MDNS_CLIENT, true);

    // Step 4: Verify final state
    SubsystemState final_state = get_subsystem_state(mdns_client_subsystem_id);

    log_this(SR_MDNS_CLIENT, "mDNS Client subsystem launched successfully (placeholder)", LOG_LEVEL_STATE, 0);

    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Successfully launched and running", LOG_LEVEL_STATE, 0);
        return 1;
    } else {
        log_this(SR_MDNS_CLIENT, "LAUNCH: MDNS CLIENT - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
