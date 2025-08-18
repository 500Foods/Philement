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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// External declarations
extern ServiceThreads mdns_server_threads;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern AppConfig* app_config;

// Check if the mDNS server subsystem is ready to launch
LaunchReadiness check_mdns_server_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("mDNS Server");
    
    // Register dependency on Network subsystem
    int mdns_id = get_subsystem_id_by_name("mDNS Server");
    if (mdns_id >= 0) {
        if (!add_dependency_from_launch(mdns_id, "Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Failed to register Network dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network dependency registered");
        
        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Network subsystem not running");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network subsystem running");
    }
    
    // Check basic configuration
    if (!app_config || !app_config->mdns_server.enabled) {
        readiness.messages[msg_count++] = strdup("  No-Go:   mDNS server disabled in configuration");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      mDNS server enabled in configuration");

    // Validate required configuration fields
    const MDNSServerConfig* config = &app_config->mdns_server;

    if (!config->device_id || !config->device_id[0]) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Device ID is required");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Device ID configured");

    if (!config->friendly_name || !config->friendly_name[0]) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Friendly name is required");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Friendly name configured");

    if (!config->model || !config->model[0]) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Model is required");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Model configured");

    if (!config->manufacturer || !config->manufacturer[0]) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Manufacturer is required");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Manufacturer configured");

    if (!config->version || !config->version[0]) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Version is required");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Version configured");

    // Validate services array if present
    if (config->num_services > 0) {
        if (!config->services) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Services array is NULL but count is non-zero");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Services array allocated");

        // Validate each service
        for (size_t i = 0; i < config->num_services; i++) {
            const mdns_server_service_t* service = &config->services[i];
            char msg_buffer[256];

            if (!service->name || !service->name[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu name is required", i + 1);
                readiness.messages[msg_count++] = strdup(msg_buffer);
                readiness.messages[msg_count] = NULL;
                readiness.ready = false;
                return readiness;
            }

            if (!service->type || !service->type[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu type is required", i + 1);
                readiness.messages[msg_count++] = strdup(msg_buffer);
                readiness.messages[msg_count] = NULL;
                readiness.ready = false;
                return readiness;
            }

            if (service->port <= 0 || service->port > 65535) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu has invalid port number", i + 1);
                readiness.messages[msg_count++] = strdup(msg_buffer);
                readiness.messages[msg_count] = NULL;
                readiness.ready = false;
                return readiness;
            }

            if (service->num_txt_records > 0 && !service->txt_records) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu TXT records array is NULL but count is non-zero", i + 1);
                readiness.messages[msg_count++] = strdup(msg_buffer);
                readiness.messages[msg_count] = NULL;
                readiness.ready = false;
                return readiness;
            }

            snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Service %zu validated", i + 1);
            readiness.messages[msg_count++] = strdup(msg_buffer);
        }
    }

    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of mDNS Server Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Launch the mDNS server subsystem
int launch_mdns_server_subsystem(void) {
    // Reset shutdown flag
    mdns_server_system_shutdown = 0;
    
    // Initialize mDNS server thread structure
    init_service_threads(&mdns_server_threads);
    
    // Additional initialization as needed
    return 1;
}
