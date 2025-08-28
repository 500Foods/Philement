/*
 * Launch mDNS Server Subsystem
 * 
 * This module handles the initialization of the mDNS server subsystem.
 * It provides functions for checking readiness and launching the mDNS server.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_mdns_server.c
 */

 // Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// External declarations
extern ServiceThreads mdns_server_threads;

// Check if the mDNS server subsystem is ready to launch
LaunchReadiness check_mdns_server_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup("mDNS Server"));

    // Register dependency on Network subsystem
    int mdns_id = get_subsystem_id_by_name("mDNS Server");
    if (mdns_id >= 0) {
        if (!add_dependency_from_launch(mdns_id, "Network")) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not running"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem running"));
    }

    // Check basic configuration
    if (!app_config || !(app_config->mdns_server.enable_ipv4 || app_config->mdns_server.enable_ipv6)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   mDNS server disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      mDNS server enabled in configuration"));

    // Validate required configuration fields
    const MDNSServerConfig* config = &app_config->mdns_server;

    if (!config->device_id || !config->device_id[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Device ID is required"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Device ID configured"));

    if (!config->friendly_name || !config->friendly_name[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Friendly name is required"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Friendly name configured"));

    if (!config->model || !config->model[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Model is required"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Model configured"));

    if (!config->manufacturer || !config->manufacturer[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Manufacturer is required"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Manufacturer configured"));

    if (!config->version || !config->version[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Version is required"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Version configured"));

    // Validate services array if present
    if (config->num_services > 0) {
        if (!config->services) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Services array is NULL but count is non-zero"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Services array allocated"));

        // Validate each service
        for (size_t i = 0; i < config->num_services; i++) {
            const mdns_server_service_t* service = &config->services[i];
            char msg_buffer[256];

            if (!service->name || !service->name[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu name is required", i + 1);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
            }

            if (!service->type || !service->type[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu type is required", i + 1);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
            }

            if (service->port <= 0 || service->port > 65535) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu has invalid port number", i + 1);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
            }

            if (service->num_txt_records > 0 && !service->txt_records) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu TXT records array is NULL but count is non-zero", i + 1);
                add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
                finalize_launch_messages(&messages, &count, &capacity);
                return (LaunchReadiness){ .subsystem = "mDNS Server", .ready = false, .messages = messages };
            }

            snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Service %zu validated", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
        }
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of mDNS Server Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = "mDNS Server",
        .ready = ready,
        .messages = messages
    };
}

// Launch the mDNS server subsystem
int launch_mdns_server_subsystem(void) {
    // Reset shutdown flag
    mdns_server_system_shutdown = 0;
    
    // Initialize mDNS server thread structure
    init_service_threads(&mdns_server_threads, "mDNS Server");
    
    // Additional initialization as needed
    return 1;
}
