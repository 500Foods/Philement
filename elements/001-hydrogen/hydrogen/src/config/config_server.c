/*
 * Server Configuration Implementation
 *
 * Implements the configuration handlers for the server subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_server.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Function prototypes for internal functions
static int config_server_validate(const ServerConfig* config);

// Load server configuration from JSON
bool load_server_config(json_t* root, AppConfig* config, const char* config_path) {
    ServerConfig* server = &config->server;

    // Initialize with defaults
    *server = (ServerConfig){
        .server_name = strdup("Philement/hydrogen"),
        .log_file = strdup("/var/log/hydrogen/hydrogen.log"),
        .config_file = strdup(config_path),  // Store the config path
        .payload_key = strdup("${env.PAYLOAD_KEY}"),
        .startup_delay = 5
    };

    // Verify string allocations
    if (!server->server_name || !server->log_file || !server->config_file || !server->payload_key) {
        log_this("Config", "Failed to allocate server configuration strings", LOG_LEVEL_ERROR);
        config_server_cleanup(server);
        return false;
    }

    // Process all config items in sequence
    bool success = true;

    // One line per key/value using macros
    success = PROCESS_SECTION(root, "Server");
    success = success && PROCESS_STRING(root, server, server_name, "Server.ServerName", "Server");
    success = success && PROCESS_STRING(root, server, log_file, "Server.LogFile", "Server");
    success = success && PROCESS_SENSITIVE(root, server, payload_key, "Server.PayloadKey", "Server");
    success = success && PROCESS_INT(root, server, startup_delay, "Server.StartupDelay", "Server");

    // Log config file path
    log_config_item("ConfigFile", config_path, false);

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_server_validate(&config->server) == 0);
    }

    return success;
}

// Free resources allocated for server configuration
void config_server_cleanup(ServerConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    if (config->server_name) {
        free(config->server_name);
        config->server_name = NULL;
    }

    if (config->log_file) {
        free(config->log_file);
        config->log_file = NULL;
    }

    if (config->config_file) {
        free(config->config_file);
        config->config_file = NULL;
    }

    if (config->payload_key) {
        free(config->payload_key);
        config->payload_key = NULL;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ServerConfig));
}

// Validate server configuration values
static int config_server_validate(const ServerConfig* config) {
    if (!config) {
        log_this("Config", "Server config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate server name
    if (!config->server_name || !config->server_name[0]) {
        log_this("Config", "Invalid server name (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate log file path
    if (!config->log_file || !config->log_file[0]) {
        log_this("Config", "Invalid log file path (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate config file path
    if (!config->config_file || !config->config_file[0]) {
        log_this("Config", "Invalid config file path (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate payload key
    if (!config->payload_key || !config->payload_key[0]) {
        log_this("Config", "Invalid payload key (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate startup delay
    if (config->startup_delay < 0) {
        log_this("Config", "Invalid startup delay (must be non-negative)", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}