/*
 * Configuration management system with robust fallback handling
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "config.h"
#include "env/config_env.h"
#include "env/config_env_utils.h"
#include "config_utils.h"
#include "types/config_string.h"
#include "types/config_bool.h"
#include "types/config_int.h"
#include "types/config_size.h"
#include "types/config_double.h"
#include "files/config_filesystem.h"
#include "files/config_file_utils.h"
#include "security/config_sensitive.h"
#include "config_priority.h"
#include "config_defaults.h"

// Subsystem headers
#include "webserver/config_webserver.h"
#include "websocket/config_websocket.h"
#include "network/config_network.h"
#include "monitor/config_monitoring.h"
#include "print/config_print_queue.h"
#include "oidc/config_oidc.h"
#include "resources/config_resources.h"
#include "mdns/config_mdns.h"
#include "logging/config_logging.h"
#include "api/config_api.h"
#include "notify/config_notify.h"

// Core system headers
#include "../logging/logging.h"
#include "../utils/utils.h"

// JSON loading functions
#include "config/json_server.h"
#include "config/json_logging.h"
#include "config/json_webserver.h"
#include "config/json_websocket.h"
#include "config/json_api.h"
#include "config/json_swagger.h"
#include "config/json_mdns_server.h"
#include "config/json_resources.h"
#include "config/json_monitoring.h"
#include "config/json_network.h"
#include "config/json_notify.h"
#include "config/json_terminal.h"
#include "config/json_mdns_client.h"
#include "config/json_print_queue.h"
#include "config/json_mail_relay.h"
#include "config/json_databases.h"

// Global static configuration instance
static AppConfig *app_config = NULL;

// Standard system paths to check for configuration
static const char* const CONFIG_PATHS[] = {
    "hydrogen.json",
    "/etc/hydrogen/hydrogen.json",
    "/usr/local/etc/hydrogen/hydrogen.json"
};
static const int NUM_CONFIG_PATHS = sizeof(CONFIG_PATHS) / sizeof(CONFIG_PATHS[0]);

// Function forward declarations
const AppConfig* get_app_config(void);
AppConfig* load_config(const char* cmdline_path);

// Function declarations for other JSON configuration loading functions

/*
 * Load and validate configuration with comprehensive error handling
 * 
 * This function loads the configuration from a file specified by the command line,
 * environment variable, or standard locations. It validates the configuration and
 * returns a pointer to the configuration structure.
 * 
 * @param cmdline_path The path to the configuration file specified on the command line
 * @return Pointer to the loaded configuration, or NULL on error
 */
AppConfig* load_config(const char* cmdline_path) {
    log_this("Config", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Config", "CONFIGURATION", LOG_LEVEL_STATE);

    // Free previous configuration if it exists
    if (app_config) {
        free(app_config);
    }

    json_t* root = NULL;
    json_error_t error;
    const char* final_path = NULL;
    bool explicit_config = false;

    // First check HYDROGEN_CONFIG environment variable
    const char* env_path = getenv("HYDROGEN_CONFIG");
    if (env_path) {
        explicit_config = true;
        if (!is_file_readable(env_path)) {
            log_this("Config", "Environment-specified config file not found: %s", LOG_LEVEL_ERROR, env_path);
            return NULL;
        }
        root = json_load_file(env_path, 0, &error);
        if (!root) {
            log_this("Config", "Failed to load environment-specified config: %s (line %d, column %d)",
                     LOG_LEVEL_ERROR, error.text, error.line, error.column);
            return NULL;
        }
        final_path = env_path;
        log_this("Config", "Using configuration from environment variable: %s", LOG_LEVEL_STATE, env_path);
    }

    // Then try command line path if provided
    if (!root && cmdline_path) {
        explicit_config = true;
        if (!is_file_readable(cmdline_path)) {
            log_this("Config", "Command-line specified config file not found: %s", LOG_LEVEL_ERROR, cmdline_path);
            return NULL;
        }
        root = json_load_file(cmdline_path, 0, &error);
        if (!root) {
            log_this("Config", "Failed to load command-line specified config: %s (line %d, column %d)",
                     LOG_LEVEL_ERROR, error.text, error.line, error.column);
            return NULL;
        }
        final_path = cmdline_path;
        log_this("Config", "Using configuration from command line: %s", LOG_LEVEL_STATE, cmdline_path);
    }

    // If no explicit config was provided, try standard locations
    if (!root && !explicit_config) {
        for (int i = 0; i < NUM_CONFIG_PATHS; i++) {
            if (is_file_readable(CONFIG_PATHS[i])) {
                root = json_load_file(CONFIG_PATHS[i], 0, &error);
                if (root) {
                    final_path = CONFIG_PATHS[i];
                    log_this("Config", "Using configuration from: %s", LOG_LEVEL_STATE, final_path);
                    break;
                }
                // If file exists but has errors, try next location
                log_this("Config", "Skipping %s due to parse errors", LOG_LEVEL_ALERT, CONFIG_PATHS[i]);
            }
        }
    }

    // Allocate config structure
    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Config", "Failed to allocate memory for config", LOG_LEVEL_ERROR);
        if (root) json_decref(root);
        return NULL;
    }
    
    app_config = config;

    // Set up config path for use in server section
    const char* config_path = final_path;
    if (!config_path) {
        config_path = "Missing... using defaults";
    }

    // If no config file was found, log the checked locations
    if (!root) {
        log_this("Config", "No configuration file found, using defaults", LOG_LEVEL_ALERT);
        log_this("Config", "Checked locations:", LOG_LEVEL_STATE);
        if (env_path) {
            log_this("Config", "  - $HYDROGEN_CONFIG: %s", LOG_LEVEL_STATE, env_path);
        }
        if (cmdline_path) {
            log_this("Config", "  - Command line path: %s", LOG_LEVEL_STATE, cmdline_path);
        }
        for (int i = 0; i < NUM_CONFIG_PATHS; i++) {
            log_this("Config", "  - %s", LOG_LEVEL_STATE, CONFIG_PATHS[i]);
        }
    }

    // Server Configuration
    // #include "config/json_server.inc"
    if (!load_json_server(root, config, config_path)) {
        if (root) json_decref(root);
        return NULL;
    }

    // Logging Configuration
    // #include "config/json_logging.inc"
    if (!load_json_logging(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // System Resources Configuration
    if (!load_json_resources(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }
    
    // System Monitoring Configuration
    if (!load_json_monitoring(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }
    
    // Network Configuration
    if (!load_json_network(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // WebServer Configuration
    if (!load_json_webserver(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // API Configuration
    if (!load_json_api(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // Notify Configuration
    if (!load_json_notify(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }
    
    // Swagger Configuration
    if (!load_json_swagger(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }
    
    // WebSocket Configuration
    if (!load_json_websocket(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // Terminal Configuration
    if (!load_json_terminal(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // mDNS Server Configuration
    if (!load_json_mdns_server(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // mDNSClient Configuration
    if (!load_json_mdns_client(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // MailRelay Configuration
    if (!load_json_mail_relay(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // Databases Configuration
    if (!load_json_databases(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    // Print Queue Configuration
    if (!load_json_print_queue(root, config)) {
        if (root) json_decref(root);
        return NULL;
    }

    if (root) json_decref(root);
    
    return config;
}

/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 * 
 * @return Pointer to the current application configuration
 */
const AppConfig* get_app_config(void) {
    return app_config;
}