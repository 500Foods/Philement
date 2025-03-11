/*
 * mDNS Server Subsystem Startup Handler
 * 
 * This module handles the initialization of the mDNS server subsystem.
 * It implements dynamic service advertisement based on active components.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "../state/state.h"
#include "../logging/logging.h"
#include "../mdns/mdns_server.h"
#include "../network/network.h"
#include "../websocket/websocket_server.h"

// Forward declarations
extern int get_websocket_port(void);

// Initialize mDNS Server system
// Requires: Network info, Logging system
//
// The mDNS Server system implements dynamic service advertisement based on active components.
// This design choice serves several purposes:
// 1. Zero-configuration networking - Clients can discover the server without manual setup
// 2. Accurate service representation - Only advertises services that are actually available
// 3. Runtime port adaptation - Handles cases where preferred ports are unavailable
// 4. Security through obscurity - Services are only advertised when explicitly enabled
int init_mdns_server_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t mdns_server_system_shutdown;

    // Prevent initialization during any shutdown state
    if (server_stopping || mdns_server_system_shutdown) {
        log_this("Initialization", "Cannot initialize mDNS Server during shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    // Only proceed if we're in startup phase
    if (!server_starting) {
        log_this("Initialization", "Cannot initialize mDNS Server outside startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    // Initialize mDNS with validated configuration
    log_this("Initialization", "Starting mDNS Server initialization", LOG_LEVEL_STATE);

    // Check shutdown state again before proceeding with resource allocation
    if (server_stopping || mdns_server_system_shutdown) {
        log_this("Initialization", "Shutdown initiated, aborting mDNS Server initialization", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->mdns_server.enabled) {
        log_this("Initialization", "mDNS Server disabled in configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    // Create a filtered list of services based on what's enabled
    mdns_server_service_t *filtered_services = NULL;
    size_t filtered_count = 0;

    if (app_config->mdns_server.services && app_config->mdns_server.num_services > 0) {
        filtered_services = calloc(app_config->mdns_server.num_services, sizeof(mdns_server_service_t));
        if (!filtered_services) {
            log_this("Initialization", "Failed to allocate memory for filtered services", LOG_LEVEL_DEBUG);
            return 0;
        }

        for (size_t i = 0; i < app_config->mdns_server.num_services; i++) {
            // Only include web-related services if web server is enabled
            if (strstr(app_config->mdns_server.services[i].type, "_http._tcp") != NULL) {
                if (app_config->web.enabled) {
                    memcpy(&filtered_services[filtered_count], &app_config->mdns_server.services[i], sizeof(mdns_server_service_t));
                    filtered_count++;
                }
            }
            // Only include websocket services if websocket server is enabled
            else if (strstr(app_config->mdns_server.services[i].type, "_websocket._tcp") != NULL) {
                if (app_config->websocket.enabled) {
                    int actual_port = get_websocket_port();
                    if (actual_port > 0 && actual_port <= 65535) {
                        memcpy(&filtered_services[filtered_count], &app_config->mdns_server.services[i], sizeof(mdns_server_service_t));
                        filtered_services[filtered_count].port = (uint16_t)actual_port;
                        log_this("Initialization", "Setting WebSocket mDNS service port to %d", LOG_LEVEL_STATE, actual_port);
                        filtered_count++;
                    } else {
                        log_this("Initialization", "Invalid WebSocket port: %d, skipping mDNS service", LOG_LEVEL_DEBUG, actual_port);
                    }
                }
            }
            // Include any other services by default
            else {
                memcpy(&filtered_services[filtered_count], &app_config->mdns_server.services[i], sizeof(mdns_server_service_t));
                filtered_count++;
            }
        }
    }

    // Only create config URL if web server is enabled
    char config_url[128] = {0};
    if (app_config->web.enabled) {
        snprintf(config_url, sizeof(config_url), "http://localhost:%d", app_config->web.port);
    }

    mdns_server = mdns_server_init(app_config->server.server_name,
                                 app_config->mdns_server.device_id,
                                 app_config->mdns_server.friendly_name,
                                 app_config->mdns_server.model,
                                 app_config->mdns_server.manufacturer,
                                 app_config->mdns_server.version,
                                 "1.0", // Hardware version
                                 config_url,
                                 filtered_services,
                                 filtered_count,
                                 app_config->mdns_server.enable_ipv6);

    if (!mdns_server) {
        log_this("Initialization", "Failed to initialize mDNS Server", LOG_LEVEL_DEBUG);
        free(filtered_services);
        return 0;
    }

    // Start mDNS thread with heap-allocated arguments
    net_info = get_network_info();
    mdns_server_thread_arg_t *mdns_server_arg = malloc(sizeof(mdns_server_thread_arg_t));
    if (!mdns_server_arg) {
        log_this("Initialization", "Failed to allocate mDNS Server thread arguments", LOG_LEVEL_DEBUG);
        mdns_server_shutdown(mdns_server);
        free_network_info(net_info);
        free(filtered_services);
        return 0;
    }

    mdns_server_arg->mdns_server = mdns_server;
    mdns_server_arg->port = 0;  // Not used anymore, each service has its own port
    mdns_server_arg->net_info = net_info;
    mdns_server_arg->running = &server_running;

    if (pthread_create(&mdns_server_thread, NULL, mdns_server_announce_loop, mdns_server_arg) != 0) {
        log_this("Initialization", "Failed to start mDNS Server thread", LOG_LEVEL_DEBUG);
        free(mdns_server_arg);
        mdns_server_shutdown(mdns_server);
        free_network_info(net_info);
        free(filtered_services);
        return 0;
    }

    free(filtered_services);  // Safe to free after mdns_server_init has copied the data

    log_this("Initialization", "mDNS Server initialized successfully", LOG_LEVEL_STATE);
    return 1;
}