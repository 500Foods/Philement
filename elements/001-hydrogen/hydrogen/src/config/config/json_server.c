/*
 * Server configuration JSON parsing
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
#include "json_server.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"

bool load_json_server(json_t* root, AppConfig* config, const char* config_path) {
    // Server Configuration
    json_t* server = json_object_get(root, "Server");
    log_config_section_header("Server");

    if (json_is_object(server)) {
        // Server Name
        json_t* server_name = json_object_get(server, "ServerName");
        config->server.server_name = get_config_string_with_env("ServerName", server_name, DEFAULT_SERVER_NAME);
        if (!config->server.server_name) {
            log_this("Config", "Failed to allocate server name", LOG_LEVEL_ERROR);
            return false;
        }
        log_config_section_item("ServerName", "%s", LOG_LEVEL_STATE, !server_name, 0, NULL, NULL, "Config",
            config->server.server_name);

        // Config File (always from filesystem)
        char real_path[PATH_MAX];
        if (realpath(config_path, real_path) != NULL) {
            config->server.config_file = strdup(real_path);
        } else {
            config->server.config_file = strdup(config_path);
        }
        
        if (!config->server.config_file) {
            log_this("Config", "Failed to allocate config file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            return false;
        }
        log_config_section_item("ConfigFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config",
            config->server.config_file);

        // Executable File (use default "./hydrogen")
        config->server.exec_file = strdup("./hydrogen");
        if (!config->server.exec_file) {
            log_this("Config", "Failed to allocate executable file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            return false;
        }
        log_config_section_item("ExecFile", "%s", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.exec_file);

        // Log File
        json_t* log_file = json_object_get(server, "LogFile");
        char* log_path = get_config_string_with_env("LogFile", log_file, DEFAULT_LOG_FILE_PATH);
        if (!log_path) {
            log_this("Config", "Failed to allocate log file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            return false;
        }
        
        // Store log file path, resolving if possible
        char log_real_path[PATH_MAX];
        if (realpath(log_path, log_real_path) != NULL) {
            config->server.log_file = strdup(log_real_path);
            free(log_path);
        } else {
            config->server.log_file = log_path;
        }
        log_config_section_item("LogFile", "%s", LOG_LEVEL_STATE, !log_file, 0, NULL, NULL, "Config",
            config->server.log_file);

        // Payload Key (for payload decryption)
        json_t* payload_key = json_object_get(server, "PayloadKey");
        config->server.payload_key = get_config_string_with_env("PayloadKey", payload_key, "Missing Key");
        if (!config->server.payload_key) {
            log_this("Config", "Failed to allocate payload key", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            free(config->server.log_file);
            return false;
        }
        log_config_section_item("PayloadKey", "%s", LOG_LEVEL_STATE, !payload_key, 0, NULL, NULL, "Config",
            config->server.payload_key);

        // Startup Delay (in milliseconds)
        json_t* startup_delay = json_object_get(server, "StartupDelay");
        if (json_is_string(startup_delay)) {
            char* delay_str = get_config_string_with_env("StartupDelay", startup_delay, "5");
            config->server.startup_delay = atoi(delay_str);
            free(delay_str);
        } else {
            config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
            log_config_section_item("StartupDelay", "%d", LOG_LEVEL_STATE, !startup_delay, 0, "ms", "ms", "Config",
                config->server.startup_delay);
        }
        if (config->server.startup_delay < 0) {
            log_this("Config", "StartupDelay cannot be negative", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            free(config->server.log_file);
            free(config->server.payload_key);
            return false;
        }
    } else {
        // Fallback to defaults if Server object is missing
        log_config_section_header("Server");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");

        config->server.server_name = strdup(DEFAULT_SERVER_NAME);
        config->server.config_file = strdup(DEFAULT_CONFIG_FILE);
        config->server.exec_file = strdup("./hydrogen");
        config->server.log_file = strdup(DEFAULT_LOG_FILE_PATH);
        config->server.payload_key = strdup("Missing Key");
        config->server.startup_delay = DEFAULT_STARTUP_DELAY;

        // Validate default allocation
        if (!config->server.server_name || !config->server.config_file || 
            !config->server.exec_file || !config->server.log_file || 
            !config->server.payload_key) {
            log_this("Config", "Failed to allocate default server configuration strings", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            free(config->server.log_file);
            free(config->server.payload_key);
            return false;
        }
    }

    return true;
}