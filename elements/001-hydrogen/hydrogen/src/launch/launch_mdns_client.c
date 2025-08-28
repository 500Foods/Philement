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
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Check if the mDNS client subsystem is ready to launch
LaunchReadiness check_mdns_client_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup("mDNS Client"));

    // Register dependency on Network subsystem
    int mdns_id = get_subsystem_id_by_name("mDNS Client");
    if (mdns_id >= 0) {
        if (!add_dependency_from_launch(mdns_id, "Network")) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not running"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem running"));
    }

    // Check basic configuration
    if (!app_config || !(app_config->mdns_client.enable_ipv4 || app_config->mdns_client.enable_ipv6)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   mDNS client disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      mDNS client enabled in configuration"));

    // Validate configuration values
    const MDNSClientConfig* config = &app_config->mdns_client;

    // Validate intervals
    if (config->scan_interval <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid scan interval (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Scan interval valid"));

    if (config->health_check_enabled && config->health_check_interval <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid health check interval (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Health check interval valid"));

    // Validate service types if configured
    if (config->num_service_types > 0) {
        if (!config->service_types) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Service types array is NULL but count is non-zero"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Service types array allocated"));

        // Validate each service type
        for (size_t i = 0; i < config->num_service_types; i++) {
            char msg_buffer[256];

            if (!config->service_types[i].type || !config->service_types[i].type[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Invalid service type at index %zu (must not be empty)", i);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = "mDNS Client", .ready = false, .messages = messages };
            }

            snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Service type %zu validated", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
        }
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of mDNS Client Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = "mDNS Client",
        .ready = ready,
        .messages = messages
    };
}

// Launch the mDNS client subsystem
int launch_mdns_client_subsystem(void) {
    // Reset shutdown flag
    mdns_client_system_shutdown = 0;
    
    // Initialize mDNS client system
    // TODO: Add proper initialization when system is ready
    return 1;
}
