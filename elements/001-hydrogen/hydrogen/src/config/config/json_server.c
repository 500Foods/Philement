/*
 * Server configuration JSON parsing
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
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
    
    // Log header with asterisk if using defaults
    log_config_section_header(json_is_object(server) ? "Server" : "Server *");

    if (json_is_object(server)) {
        // Server Name
        json_t* server_name = json_object_get(server, "ServerName");
        config->server.server_name = get_config_string_with_env("ServerName", server_name, DEFAULT_SERVER_NAME);
        if (!config->server.server_name) {
            log_this("Config", "Failed to allocate server name", LOG_LEVEL_ERROR);
            return false;
        }
        const char* name_source = server_name ? "(from config)" : "(default)";
        log_config_section_item("ServerName", "%s %s", LOG_LEVEL_STATE, !server_name, 0, NULL, NULL, "Config",
            config->server.server_name, name_source);

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
        log_config_section_item("ConfigFile", "%s (filesystem)", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config",
            config->server.config_file);

        // Executable File
        config->server.exec_file = strdup("./hydrogen");
        if (!config->server.exec_file) {
            log_this("Config", "Failed to allocate executable file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            return false;
        }
        log_config_section_item("ExecFile", "%s (default)", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.exec_file);

        // Log File
        json_t* log_file = json_object_get(server, "LogFile");
        config->server.log_file = get_config_string_with_env("LogFile", log_file, DEFAULT_LOG_FILE_PATH);
        if (!config->server.log_file) {
            log_this("Config", "Failed to allocate log file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            return false;
        }
        const char* log_source = log_file ? "(from config)" : "(default)";
        log_config_section_item("LogFile", "%s %s", LOG_LEVEL_STATE, !log_file, 0, NULL, NULL, "Config",
            config->server.log_file, log_source);

        // Payload Key (for payload decryption)
        json_t* payload_key = json_object_get(server, "PayloadKey");
        char* raw_key = get_config_string_with_env("PayloadKey", payload_key, "${env.PAYLOAD_KEY}");
        if (!raw_key) {
            log_this("Config", "Failed to allocate payload key", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            free(config->server.log_file);
            return false;
        }

        // Process environment variable and store resolved value
        json_t* resolved = env_process_env_variable(raw_key);
        if (resolved && !json_is_null(resolved)) {
            // Store the resolved value
            const char* resolved_str = json_string_value(resolved);
            config->server.payload_key = strdup(resolved_str);
            free(raw_key);  // Free the raw key since we're using resolved value
            
            // Display first 5 chars of resolved key
            log_config_section_item("PayloadKey", "$PAYLOAD_KEY: %.5s...", LOG_LEVEL_STATE, 
                !payload_key, 0, NULL, NULL, "Config", resolved_str);
        } else {
            // If resolution fails, use raw key
            config->server.payload_key = raw_key;
            log_config_section_item("PayloadKey", "$PAYLOAD_KEY: not set", LOG_LEVEL_STATE,
                !payload_key, 0, NULL, NULL, "Config");
        }
        if (resolved) json_decref(resolved);

        // Startup Delay (in seconds)
        json_t* startup_delay = json_object_get(server, "StartupDelay");
        config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
        const char* delay_source = startup_delay ? "(from config)" : "(default)";
        log_config_section_item("StartupDelay", "%ds %s", LOG_LEVEL_STATE, !startup_delay, 0, NULL, "seconds", "Config",
            config->server.startup_delay, delay_source);
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

        // Set and log each default value
        config->server.server_name = strdup(DEFAULT_SERVER_NAME);
        log_config_section_item("ServerName", "%s (default)", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.server_name);

        config->server.config_file = strdup(DEFAULT_CONFIG_FILE);
        log_config_section_item("ConfigFile", "%s (default)", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.config_file);

        config->server.exec_file = strdup("./hydrogen");
        log_config_section_item("ExecFile", "%s (default)", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.exec_file);

        config->server.log_file = strdup(DEFAULT_LOG_FILE_PATH);
        log_config_section_item("LogFile", "%s (default)", LOG_LEVEL_STATE, true, 0, NULL, NULL, "Config",
            config->server.log_file);

        // Handle PayloadKey with potential environment variable
        char* raw_key = strdup("${env.PAYLOAD_KEY}");
        if (!raw_key) {
            log_this("Config", "Failed to allocate default payload key", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            free(config->server.exec_file);
            free(config->server.log_file);
            return false;
        }

        // Process environment variable and store resolved value
        json_t* resolved = env_process_env_variable(raw_key);
        if (resolved && !json_is_null(resolved)) {
            // Store the resolved value
            const char* resolved_str = json_string_value(resolved);
            config->server.payload_key = strdup(resolved_str);
            free(raw_key);  // Free the raw key since we're using resolved value
            
            // Display first 5 chars of resolved key
            log_config_section_item("PayloadKey", "$PAYLOAD_KEY: %.5s...", LOG_LEVEL_STATE, 
                true, 0, NULL, NULL, "Config", resolved_str);
        } else {
            // If resolution fails, use raw key
            config->server.payload_key = raw_key;
            log_config_section_item("PayloadKey", "$PAYLOAD_KEY: not set", LOG_LEVEL_STATE,
                true, 0, NULL, NULL, "Config");
        }
        if (resolved) json_decref(resolved);

        config->server.startup_delay = DEFAULT_STARTUP_DELAY;
        log_config_section_item("StartupDelay", "%ds (default)", LOG_LEVEL_STATE, true, 0, NULL, "seconds", "Config",
            config->server.startup_delay);

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