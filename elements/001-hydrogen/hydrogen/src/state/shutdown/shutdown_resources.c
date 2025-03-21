/*
 * Resource Cleanup for Hydrogen Shutdown
 * 
 * This module handles the cleanup of various system resources
 * during the shutdown process, including network interfaces
 * and configuration structures.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <stdlib.h>

// Project headers
#include "shutdown_internal.h"
#include "../../logging/logging.h"
#include "../../network/network.h"

// Clean up network resources
// Called after all network-using components are stopped
// Frees network interface information
void shutdown_network(void) {
    // Use different subsystem name based on operation for clarity throughout the process
    const char* subsystem = restart_requested ? "Restart" : "Shutdown";
    log_this(subsystem, "Freeing network info", LOG_LEVEL_STATE);
    if (net_info) {
        free_network_info(net_info);
        net_info = NULL;
    }
}

// Free all configuration resources
// Must be called last as other components may need config during shutdown
// Recursively frees all allocated configuration structures
void free_app_config(void) {
    if (app_config) {
        free(app_config->server.server_name);
        free(app_config->server.payload_key);
        free(app_config->server.config_file);
        free(app_config->server.exec_file);
        free(app_config->server.log_file);

        // Free Web config
        free(app_config->web.web_root);
        free(app_config->web.upload_path);
        free(app_config->web.upload_dir);

        // Free WebSocket config
        free(app_config->websocket.protocol);
        free(app_config->websocket.key);

        // Free mDNS Server config
        free(app_config->mdns_server.device_id);
        free(app_config->mdns_server.friendly_name);
        free(app_config->mdns_server.model);
        free(app_config->mdns_server.manufacturer);
        free(app_config->mdns_server.version);
        for (size_t i = 0; i < app_config->mdns_server.num_services; i++) {
            free(app_config->mdns_server.services[i].name);
            free(app_config->mdns_server.services[i].type);
            for (size_t j = 0; j < app_config->mdns_server.services[i].num_txt_records; j++) {
                free(app_config->mdns_server.services[i].txt_records[j]);
            }
            free(app_config->mdns_server.services[i].txt_records);
        }
        free(app_config->mdns_server.services);

        // Free logging config
        if (app_config->logging.levels) {
            for (size_t i = 0; i < app_config->logging.level_count; i++) {
                free((void*)app_config->logging.levels[i].name);
            }
            free(app_config->logging.levels);
        }

        free(app_config);
        app_config = NULL;
    }
}