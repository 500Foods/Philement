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
#include "config_env.h"
#include "config_string.h"
#include "config_bool.h"
#include "config_int.h"
#include "config_size.h"
#include "config_double.h"
#include "config_filesystem.h"
#include "config_priority.h"
#include "config_defaults.h"

// Subsystem headers
#include "config_webserver.h"
#include "config_websocket.h"
#include "config_network.h"
#include "config_monitoring.h"
#include "config_print_queue.h"
#include "config_oidc.h"
#include "config_resources.h"
#include "config_mdns.h"
#include "config_logging.h"
#include "config_api.h"

#include "../logging/logging.h"
#include "../utils/utils.h"

// Public interface declarations
const AppConfig* get_app_config(void);
AppConfig* load_config(const char* cmdline_path);

// Private interface declarations
static bool is_file_readable(const char* path);
static bool is_sensitive_value(const char* name);
static void log_config_section_header(const char* section_name);
static void log_config_section_item(const char* key, const char* format, int level,
                                  int is_default, int indent, const char* input_units,
                                  const char* output_units, ...);
static char* get_config_string_with_env(const char* json_key, json_t* value, const char* default_value);

/*
 * Helper function to handle environment variable substitution in config values
 * 
 * This function checks if a string value is in "${env.VAR}" format and if so,
 * processes it using the environment variable handling system. It handles:
 * - Environment variable resolution
 * - Type conversion
 * - Logging with Config-Env subsystem
 * - Sensitive value masking
 * 
 * @param json_key The configuration key name (for logging)
 * @param value The JSON value to check
 * @param default_value The default value to use if no value is provided
 * @return The resolved string value (caller must free)
 */
static char* get_config_string_with_env(const char* json_key, json_t* value, const char* default_value) {
    if (!json_is_string(value)) {
        return strdup(default_value);
    }

    const char* str_value = json_string_value(value);
    if (strncmp(str_value, "${env.", 6) == 0) {
        // Extract environment variable name
        const char* env_var = str_value + 6;
        size_t env_var_len = strlen(env_var);
        if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
            char var_name[256] = {0};
            strncpy(var_name, env_var, env_var_len - 1);
            var_name[env_var_len - 1] = '\0';

            const char* env_value = getenv(var_name);
            if (env_value) {
                // For sensitive values, truncate in log
                if (is_sensitive_value(json_key)) {
                    char safe_value[256];
                    strncpy(safe_value, env_value, 5);
                    safe_value[5] = '\0';
                    strcat(safe_value, "...");
                    log_this("Config-Env", "- %s: $%s: %s", LOG_LEVEL_STATE, json_key, var_name, safe_value);
                } else {
                    log_this("Config-Env", "- %s: $%s: %s", LOG_LEVEL_STATE, json_key, var_name, env_value);
                }
                return strdup(env_value);
            } else {
                log_this("Config-Env", "- %s: $%s: (not set)", LOG_LEVEL_STATE, json_key, var_name);
                return strdup(default_value);
            }
        }
    }

    // Not an environment variable, use the value directly
    return strdup(str_value);
}

// Global static configuration instance
static AppConfig *app_config = NULL;

/*
 * Helper function to detect sensitive configuration values
 * 
 * This function checks if a configuration key contains sensitive terms
 * like "key", "token", "pass", etc. that might contain secrets.
 * 
 * @param name The configuration key name
 * @return true if the name contains a sensitive term, false otherwise
 */
static bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    const char* sensitive_terms[] = {
        "key", "token", "pass", "seed", "jwt", "secret"
    };
    int num_terms = sizeof(sensitive_terms) / sizeof(sensitive_terms[0]);
    
    for (int i = 0; i < num_terms; i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    
    return false;
}
    


/*
 * Helper function to log a configuration section header
 * 
 * This function logs a section header with the name of the configuration section.
 * It's used to group related configuration items in the log.
 * 
 * @param section_name The name of the configuration section
 */
static void log_config_section_header(const char* section_name) {
    log_this("Config", "%s", LOG_LEVEL_STATE, section_name);
}

/*
 * Helper function to log a regular configuration item
 * 
 * This function logs a configuration item with proper indentation and unit conversion.
 * The format is "[indent]- key: value [units]" with an optional asterisk (*) to 
 * indicate when a default value is being used.
 * 
 * @param key The configuration key
 * @param format The format string for the value
 * @param level The log level
 * @param is_default Whether this is a default value (1) or from config (0)
 * @param indent Indentation level (0 = top level, 1+ = nested)
 * @param input_units The units of the input value (e.g., "B" for bytes, "ms" for milliseconds)
 * @param output_units The desired display units (e.g., "MB" for megabytes, "s" for seconds)
 * @param ... Variable arguments for the format string
 */
static void log_config_section_item(const char* key, const char* format, int level, int is_default,
                           int indent, const char* input_units, const char* output_units, ...) {
    if (!key || !format) {
        return;
    }
    
    va_list args;
    va_start(args, output_units);
    
    char value_buffer[1024] = {0};
    vsnprintf(value_buffer, sizeof(value_buffer), format, args);
    va_end(args);
    
    char message[2048] = {0};
    
    // Add indent prefix based on level
    switch (indent) {
        case 0:
            strncat(message, "― ", sizeof(message) - strlen(message) - 1);
            break;
        case 1:
            strncat(message, "――― ", sizeof(message) - strlen(message) - 1);
            break;
        case 2:
            strncat(message, "――――― ", sizeof(message) - strlen(message) - 1);
            break;
        case 3:
            strncat(message, "――――――― ", sizeof(message) - strlen(message) - 1);
            break;
        default:
            strncat(message, "――――――――― ", sizeof(message) - strlen(message) - 1);
            break;
    }
    
    strncat(message, key, sizeof(message) - strlen(message) - 1);
    strncat(message, ": ", sizeof(message) - strlen(message) - 1);
    
    // Handle unit conversion if units are specified
    if (input_units && output_units) {
        double value;
        if (sscanf(value_buffer, "%lf", &value) == 1) {
            // Convert based on unit combination
            if (strcmp(input_units, output_units) != 0) {  // Only convert if units differ
                switch (input_units[0] | (output_units[0] << 8)) {
                    case 'B' | ('M' << 8):  // B -> MB
                        value /= (1024.0 * 1024.0);
                        snprintf(value_buffer, sizeof(value_buffer), "%.2f", value);
                        break;
                    case 'm' | ('s' << 8):  // ms -> s
                        value /= 1000.0;
                        snprintf(value_buffer, sizeof(value_buffer), "%.2f", value);
                        break;
                }
            }
            // Add the value and units to the message
            strncat(message, value_buffer, sizeof(message) - strlen(message) - 1);
            strncat(message, " ", sizeof(message) - strlen(message) - 1);
            strncat(message, output_units, sizeof(message) - strlen(message) - 1);
        } else {
            // If value couldn't be converted, just display as is
            strncat(message, value_buffer, sizeof(message) - strlen(message) - 1);
        }
    } else {
        strncat(message, value_buffer, sizeof(message) - strlen(message) - 1);
    }
    
    // Add asterisk for default values
    if (is_default) {
        strncat(message, " *", sizeof(message) - strlen(message) - 1);
    }
    
    log_this("Config", "%s", level, message);
}



/*
 * Load and validate configuration with comprehensive error handling
 */
// Standard system paths to check for configuration
static const char* const CONFIG_PATHS[] = {
    "hydrogen.json",
    "/etc/hydrogen/hydrogen.json",
    "/usr/local/etc/hydrogen/hydrogen.json"
};
static const int NUM_CONFIG_PATHS = sizeof(CONFIG_PATHS) / sizeof(CONFIG_PATHS[0]);

// Helper function to check if a file exists and is readable
static bool is_file_readable(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode) && access(path, R_OK) == 0);
}

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

    // If we found a config file, process it
    if (root) {
        log_this("Config", "Using configuration from: %s", LOG_LEVEL_STATE, config_path);
    } else {
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
    json_t* server = json_object_get(root, "Server");
    if (json_is_object(server)) {
        log_config_section_header("Server");

        // Server Name
        json_t* server_name = json_object_get(server, "ServerName");
        config->server.server_name = get_config_string_with_env("ServerName", server_name, DEFAULT_SERVER_NAME);
        log_config_section_item("ServerName", "%s", LOG_LEVEL_STATE, !server_name, 0, NULL, NULL, config->server.server_name);
                
        // Store configuration paths
        char real_path[PATH_MAX];
        
        // Config File
        if (realpath(config_path, real_path) != NULL) {
            config->server.config_file = strdup(real_path);
        } else {
            config->server.config_file = strdup(config_path);
        }
        log_config_section_item("ConfigFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->server.config_file);
        
        // Exec File
        config->server.exec_file = get_executable_path();
        if (!config->server.exec_file) {
            log_this("Config", "Failed to get executable path, using default", LOG_LEVEL_STATE);
            config->server.exec_file = strdup("./hydrogen");
        }
        log_config_section_item("ExecFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->server.exec_file);

        // Log File
        json_t* log_file = json_object_get(server, "LogFile");
        char* log_path = get_config_string_with_env("LogFile", log_file, DEFAULT_LOG_FILE_PATH);
        if (realpath(log_path, real_path) != NULL) {
            config->server.log_file = strdup(real_path);
            free(log_path);
        } else {
            config->server.log_file = log_path;
        }
        log_config_section_item("LogFile", "%s", LOG_LEVEL_STATE, !log_file, 0, NULL, NULL, config->server.log_file);

        // Payload Key (for payload decryption)
        json_t* payload_key = json_object_get(server, "PayloadKey");
        config->server.payload_key = get_config_string_with_env("PayloadKey", payload_key, "${env.PAYLOAD_KEY}");

        // Startup Delay (in milliseconds)
        json_t* startup_delay = json_object_get(server, "StartupDelay");
        config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
        log_config_section_item("StartupDelay", "%d", LOG_LEVEL_STATE, !startup_delay, 0, "ms", "ms", config->server.startup_delay);
    } else {
        // Fallback to defaults if Server object is missing
        config->server.server_name = strdup(DEFAULT_SERVER_NAME);
        config->server.config_file = strdup(DEFAULT_CONFIG_FILE);
        config->server.exec_file = strdup("./hydrogen");
        config->server.log_file = strdup(DEFAULT_LOG_FILE_PATH);
        config->server.payload_key = strdup("MISSING");
        config->server.startup_delay = DEFAULT_STARTUP_DELAY;
        log_config_section_header("Server");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
        log_config_section_item("ConfigFile", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_CONFIG_FILE);
        log_config_section_item("ExecFile", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "./hydrogen");
        log_config_section_item("LogFile", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_LOG_FILE_PATH);
        log_config_section_item("ServerName", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_SERVER_NAME);
        log_config_section_item("PayloadKey", "MISSING", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
        log_config_section_item("StartupDelay", "%d", LOG_LEVEL_STATE, 1, 0, "ms", "ms", DEFAULT_STARTUP_DELAY);
    }

    // Logging Configuration
    json_t* logging = json_object_get(root, "Logging");
    if (json_is_object(logging)) {
        log_config_section_header("Logging");

        // Initialize logging configuration
        if (config_logging_init(&config->logging) != 0) {
            log_this("Config", "Failed to initialize logging configuration", LOG_LEVEL_ERROR);
            json_decref(root);
            return NULL;
        }

        // Log Levels
        json_t* levels = json_object_get(logging, "Levels");
        if (json_is_array(levels)) {
            config->logging.level_count = json_array_size(levels);
            config->logging.levels = calloc(config->logging.level_count, sizeof(*config->logging.levels));
            if (!config->logging.levels) {
                log_this("Config", "Failed to allocate memory for log levels", LOG_LEVEL_ERROR);
                json_decref(root);
                return NULL;
            }

            log_config_section_item("LogLevels", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->logging.level_count);
            
            for (size_t i = 0; i < config->logging.level_count; i++) {
                json_t* level = json_array_get(levels, i);
                if (json_is_array(level) && json_array_size(level) == 2) {
                    config->logging.levels[i].value = json_integer_value(json_array_get(level, 0));
                    config->logging.levels[i].name = strdup(json_string_value(json_array_get(level, 1)));
                    log_config_section_item("Level", "%s (%d)", LOG_LEVEL_STATE, 0, 1, NULL, NULL, 
                        config->logging.levels[i].name, config->logging.levels[i].value);
                }
            }
        } else {
            // Set default levels if not configured
            config->logging.level_count = DEFAULT_LOG_LEVEL_COUNT;
            config->logging.levels = calloc(DEFAULT_LOG_LEVEL_COUNT, sizeof(*config->logging.levels));
            if (!config->logging.levels) {
                log_this("Config", "Failed to allocate memory for default log levels", LOG_LEVEL_ERROR);
                json_decref(root);
                return NULL;
            }

            // Use DEFAULT_PRIORITY_LEVELS directly from config_priority.c
            for (size_t i = 0; i < DEFAULT_LOG_LEVEL_COUNT; i++) {
                config->logging.levels[i].value = DEFAULT_PRIORITY_LEVELS[i].value;
                config->logging.levels[i].name = strdup(DEFAULT_PRIORITY_LEVELS[i].label);
                log_config_section_item("Level", "%s (%d)", LOG_LEVEL_STATE, 1, 1, NULL, NULL,
                    config->logging.levels[i].name, config->logging.levels[i].value);
            }
        }

        // Console Logging
        json_t* console = json_object_get(logging, "Console");
        if (json_is_object(console)) {
            log_config_section_item("Console", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* enabled = json_object_get(console, "Enabled");
            config->logging.console.enabled = get_config_bool(enabled, true);
            log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL, 
                config->logging.console.enabled ? "true" : "false");

            if (config->logging.console.enabled) {
                json_t* default_level = json_object_get(console, "DefaultLevel");
                config->logging.console.default_level = get_config_int(default_level, LOG_LEVEL_DEBUG);
                log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL, 
                    config_logging_get_level_name(&config->logging, config->logging.console.default_level));

                json_t* subsystems = json_object_get(console, "Subsystems");
                if (json_is_object(subsystems)) {
                    log_config_section_item("Subsystems", "Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL);
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }

        // File Logging
        json_t* file = json_object_get(logging, "File");
        if (json_is_object(file)) {
            log_config_section_item("File", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* enabled = json_object_get(file, "Enabled");
            config->logging.file.enabled = get_config_bool(enabled, true);
            log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL,
                config->logging.file.enabled ? "true" : "false");

            if (config->logging.file.enabled) {
                json_t* default_level = json_object_get(file, "DefaultLevel");
                config->logging.file.default_level = get_config_int(default_level, LOG_LEVEL_DEBUG);
                log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL,
                    config_logging_get_level_name(&config->logging, config->logging.file.default_level));

                json_t* path = json_object_get(file, "Path");
                config->logging.file.file_path = get_config_string_with_env("Path", path, DEFAULT_LOG_FILE_PATH);
                log_config_section_item("Path", "%s", LOG_LEVEL_STATE, !path, 1, NULL, NULL,
                    config->logging.file.file_path);

                json_t* subsystems = json_object_get(file, "Subsystems");
                if (json_is_object(subsystems)) {
                    log_config_section_item("Subsystems", "Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL);
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }

        // Database Logging
        json_t* database = json_object_get(logging, "Database");
        if (json_is_object(database)) {
            log_config_section_item("Database", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* enabled = json_object_get(database, "Enabled");
            config->logging.database.enabled = get_config_bool(enabled, true);
            log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL,
                config->logging.database.enabled ? "true" : "false");

            if (config->logging.database.enabled) {
                json_t* default_level = json_object_get(database, "DefaultLevel");
                config->logging.database.default_level = get_config_int(default_level, LOG_LEVEL_ERROR);
                log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL,
                    config_logging_get_level_name(&config->logging, config->logging.database.default_level));

                json_t* conn_string = json_object_get(database, "ConnectionString");
                config->logging.database.connection_string = get_config_string_with_env("ConnectionString", 
                    conn_string, "sqlite:///var/lib/hydrogen/logs.db");
                log_config_section_item("ConnectionString", "%s", LOG_LEVEL_STATE, !conn_string, 1, NULL, NULL,
                    config->logging.database.connection_string);

                json_t* subsystems = json_object_get(database, "Subsystems");
                if (json_is_object(subsystems)) {
                    log_config_section_item("Subsystems", "Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL);
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }

        // Validate the logging configuration
        if (config_logging_validate(&config->logging) != 0) {
            log_this("Config", "Invalid logging configuration", LOG_LEVEL_ERROR);
            json_decref(root);
            return NULL;
        }
    } else {
        log_config_section_header("Logging");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
        
        // Initialize with defaults
        if (config_logging_init(&config->logging) != 0) {
            log_this("Config", "Failed to initialize default logging configuration", LOG_LEVEL_ERROR);
            json_decref(root);
            return NULL;
        }
    }

    
    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        log_config_section_header("WebServer");
        
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
                config->web.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL,
                config->web.enable_ipv6 ? "true" : "false");

        json_t* port = json_object_get(web, "Port");
        config->web.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, config->web.port);

        json_t* web_root = json_object_get(web, "WebRoot");
        config->web.web_root = get_config_string_with_env("WebRoot", web_root, "/var/www/html");
        log_config_section_item("WebRoot", "%s", LOG_LEVEL_STATE, !web_root, 0, NULL, NULL, config->web.web_root);

        json_t* upload_path = json_object_get(web, "UploadPath");
        config->web.upload_path = get_config_string_with_env("UploadPath", upload_path, DEFAULT_UPLOAD_PATH);
        log_config_section_item("UploadPath", "%s", LOG_LEVEL_STATE, !upload_path, 0, NULL, NULL, config->web.upload_path);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        config->web.upload_dir = get_config_string_with_env("UploadDir", upload_dir, DEFAULT_UPLOAD_DIR);
        log_config_section_item("UploadDir", "%s", LOG_LEVEL_STATE, !upload_dir, 0, NULL, NULL, config->web.upload_dir);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
        log_config_section_item("MaxUploadSize", "%zu", LOG_LEVEL_STATE, !max_upload_size, 0, "B", "MB", config->web.max_upload_size);
        
        json_t* api_prefix = json_object_get(web, "ApiPrefix");
        config->web.api_prefix = get_config_string_with_env("ApiPrefix", api_prefix, "/api");
        log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, !api_prefix, 0, NULL, NULL, config->web.api_prefix);
    } else {
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.api_prefix = strdup("/api");
        log_config_section_header("WebServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
        log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_WEB_PORT);
        log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, config->web.api_prefix);
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        log_config_section_header("WebSocket");
        
        json_t* enabled = json_object_get(websocket, "Enabled");
        config->websocket.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
                config->websocket.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = get_config_bool(enable_ipv6, 0);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL,
                config->websocket.enable_ipv6 ? "true" : "false");

        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEBSOCKET_PORT;
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, config->websocket.port);

        json_t* key = json_object_get(websocket, "Key");
        config->websocket.key = get_config_string_with_env("Key", key, "default_key");

        json_t* protocol = json_object_get(websocket, "protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "Protocol");  // Try legacy uppercase key
        }
        config->websocket.protocol = get_config_string_with_env("Protocol", protocol, "hydrogen-protocol");
        if (!config->websocket.protocol) {
            log_config_section_item("Protocol", "Failed to allocate string", LOG_LEVEL_ERROR, 1, 0, NULL, NULL);
            free(config->websocket.key);
            return NULL;
        }
        log_config_section_item("Protocol", "%s", LOG_LEVEL_STATE, !protocol, 0, NULL, NULL, config->websocket.protocol);

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, 10 * 1024 * 1024);
        log_config_section_item("MaxMessageSize", "%zu", LOG_LEVEL_STATE, !max_message_size, 0, "B", "MB", config->websocket.max_message_size);

        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            log_config_section_item("ConnectionTimeouts", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, 10);
            log_config_section_item("ExitWaitSeconds", "%d", LOG_LEVEL_STATE, !exit_wait_seconds, 1, NULL, NULL, config->websocket.exit_wait_seconds);
        } else {
            config->websocket.exit_wait_seconds = 10;
        }
    } else {
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;
        config->websocket.exit_wait_seconds = 10;
        
        log_config_section_header("WebSocket");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
        log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_WEBSOCKET_PORT);
        log_config_section_item("Protocol", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "hydrogen-protocol");
    }

    // mDNS Server Configuration
    json_t* mdns_server = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns_server)) {
        log_config_section_header("mDNSServer");
        
        json_t* enabled = json_object_get(mdns_server, "Enabled");
        config->mdns_server.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
                 config->mdns_server.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
        config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL,
                 config->mdns_server.enable_ipv6 ? "true" : "false");

        json_t* device_id = json_object_get(mdns_server, "DeviceId");
        config->mdns_server.device_id = get_config_string_with_env("DeviceId", device_id, "hydrogen-printer");
        log_config_section_item("DeviceId", "%s", LOG_LEVEL_STATE, !device_id, 0, NULL, NULL, config->mdns_server.device_id);

        json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
        config->mdns_server.friendly_name = get_config_string_with_env("FriendlyName", friendly_name, "Hydrogen 3D Printer");
        log_config_section_item("FriendlyName", "%s", LOG_LEVEL_STATE, !friendly_name, 0, NULL, NULL, config->mdns_server.friendly_name);

        json_t* model = json_object_get(mdns_server, "Model");
        config->mdns_server.model = get_config_string_with_env("Model", model, "Hydrogen");
        log_config_section_item("Model", "%s", LOG_LEVEL_STATE, !model, 0, NULL, NULL, config->mdns_server.model);

        json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
        config->mdns_server.manufacturer = get_config_string_with_env("Manufacturer", manufacturer, "Philement");
        log_config_section_item("Manufacturer", "%s", LOG_LEVEL_STATE, !manufacturer, 0, NULL, NULL, config->mdns_server.manufacturer);

        json_t* version = json_object_get(mdns_server, "Version");
        config->mdns_server.version = get_config_string_with_env("Version", version, VERSION);
        log_config_section_item("Version", "%s", LOG_LEVEL_STATE, !version, 0, NULL, NULL, config->mdns_server.version);
        
        json_t* services = json_object_get(mdns_server, "Services");
        if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            log_config_section_item("Services", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->mdns_server.num_services);
            config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));
            for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;
                // Get service properties
                json_t* name = json_object_get(service, "Name");
                config->mdns_server.services[i].name = get_config_string_with_env("Name", name, "hydrogen");
                
                json_t* type = json_object_get(service, "Type");
                config->mdns_server.services[i].type = get_config_string_with_env("Type", type, "_http._tcp.local");

                json_t* port = json_object_get(service, "Port");
                config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);
                
                // Log service details after all properties are populated
        log_config_section_item("Service", "%s: %s on port %d", LOG_LEVEL_STATE, 0, 1, NULL, NULL,
                                       config->mdns_server.services[i].name,
                                       config->mdns_server.services[i].type,
                                       config->mdns_server.services[i].port);

                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = 1;
                    config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                    config->mdns_server.services[i].txt_records[0] = get_config_string_with_env("TxtRecord", txt_records, "");
                } else if (json_is_array(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns_server.services[i].txt_records = malloc(config->mdns_server.services[i].num_txt_records * sizeof(char*));
                    for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                        json_t* record = json_array_get(txt_records, j);
                        config->mdns_server.services[i].txt_records[j] = get_config_string_with_env("TxtRecord", record, "");
                    }
                } else {
                    config->mdns_server.services[i].num_txt_records = 0;
                    config->mdns_server.services[i].txt_records = NULL;
                }
            }
        }
    } else {
        log_config_section_header("mDNSServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        log_config_section_header("SystemResources");
        
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_section_item("Queues", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(queues, "MaxQueueBlocks");
            config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
        log_config_section_item("MaxQueueBlocks", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.max_queue_blocks);
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
            log_config_section_item("QueueHashSize", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.queue_hash_size);
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
            log_config_section_item("DefaultQueueCapacity", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.default_capacity);
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            log_config_section_item("Buffers", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
        log_config_section_item("DefaultMessageBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.message_buffer_size);
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
            log_config_section_item("MaxLogMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.max_log_message_size);
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);
            log_config_section_item("LineBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.line_buffer_size);

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);
            log_config_section_item("PostProcessorBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.post_processor_buffer_size);

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);
            log_config_section_item("LogBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.log_buffer_size);

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);
            log_config_section_item("JsonMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.json_message_size);

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
            log_config_section_item("LogEntrySize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.log_entry_size);
        }
    } else {
        config->resources.max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
        config->resources.queue_hash_size = DEFAULT_QUEUE_HASH_SIZE;
        config->resources.default_capacity = DEFAULT_QUEUE_CAPACITY;
        config->resources.message_buffer_size = DEFAULT_MESSAGE_BUFFER_SIZE;
        config->resources.max_log_message_size = DEFAULT_MAX_LOG_MESSAGE_SIZE;
        config->resources.line_buffer_size = DEFAULT_LINE_BUFFER_SIZE;
        config->resources.post_processor_buffer_size = DEFAULT_POST_PROCESSOR_BUFFER_SIZE;
        config->resources.log_buffer_size = DEFAULT_LOG_BUFFER_SIZE;
        config->resources.json_message_size = DEFAULT_JSON_MESSAGE_SIZE;
        config->resources.log_entry_size = DEFAULT_LOG_ENTRY_SIZE;
        
        log_config_section_header("SystemResources");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    // Network Configuration
    json_t* network = json_object_get(root, "Network");
    if (json_is_object(network)) {
        log_config_section_header("Network");
        
        json_t* interfaces = json_object_get(network, "Interfaces");
        if (json_is_object(interfaces)) {
            log_config_section_item("Interfaces", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(interfaces, "MaxInterfaces");
            config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
        log_config_section_item("MaxInterfaces", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_interfaces);
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
            log_config_section_item("MaxIPsPerInterface", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_ips_per_interface);
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
            log_config_section_item("MaxInterfaceNameLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_interface_name_length);
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
            log_config_section_item("MaxIPAddressLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_ip_address_length);
        }

        json_t* port_allocation = json_object_get(network, "PortAllocation");
        if (json_is_object(port_allocation)) {
            log_config_section_item("PortAllocation", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(port_allocation, "StartPort");
            config->network.start_port = get_config_int(val, DEFAULT_WEB_PORT);
        log_config_section_item("StartPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.start_port);
            
            val = json_object_get(port_allocation, "EndPort");
            config->network.end_port = get_config_int(val, 65535);
            log_config_section_item("EndPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.end_port);
                log_config_section_item("ReservedPorts", "%zu ports reserved", LOG_LEVEL_STATE, 0, 1, NULL, NULL, config->network.reserved_ports_count);
            json_t* reserved_ports = json_object_get(port_allocation, "ReservedPorts");
            if (json_is_array(reserved_ports)) {
                config->network.reserved_ports_count = json_array_size(reserved_ports);
                config->network.reserved_ports = malloc(sizeof(int) * config->network.reserved_ports_count);
                for (size_t i = 0; i < config->network.reserved_ports_count; i++) {
                    config->network.reserved_ports[i] = json_integer_value(json_array_get(reserved_ports, i));
                }
            }
        }
    } else {
        config->network.max_interfaces = DEFAULT_MAX_INTERFACES;
        config->network.max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
        config->network.max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
        config->network.max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;
        config->network.start_port = DEFAULT_WEB_PORT;
        config->network.end_port = 65535;
        config->network.reserved_ports = NULL;
        config->network.reserved_ports_count = 0;
        
        log_config_section_header("Network");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    // System Monitoring Configuration
    json_t* monitoring = json_object_get(root, "SystemMonitoring");
    if (json_is_object(monitoring)) {
        log_config_section_header("SystemMonitoring");
        
        json_t* intervals = json_object_get(monitoring, "Intervals");
        if (json_is_object(intervals)) {
            log_config_section_item("Intervals", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(intervals, "StatusUpdateMs");
            config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
        log_config_section_item("StatusUpdateFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.status_update_ms);
            
            val = json_object_get(intervals, "ResourceCheckMs");
            config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
            log_config_section_item("ResourceCheckFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.resource_check_ms);
            
            val = json_object_get(intervals, "MetricsUpdateMs");
            config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
            log_config_section_item("MetricsUpdateFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.metrics_update_ms);
        }

        json_t* thresholds = json_object_get(monitoring, "Thresholds");
        if (json_is_object(thresholds)) {
            log_config_section_item("Thresholds", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
        log_config_section_item("MemoryWarningPercent", "%d%%", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.memory_warning_percent);
            
            val = json_object_get(thresholds, "DiskSpaceWarningPercent");
            config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
            log_config_section_item("DiskSpaceWarningPercent", "%d%%", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.disk_warning_percent);
            
            val = json_object_get(thresholds, "LoadAverageWarning");
            config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
            log_config_section_item("LoadAverageWarning", "%.1f", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.load_warning);
        }
    } else {
        config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
        config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
        config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
        config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
        config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
        config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
        
        log_config_section_header("SystemMonitoring");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    if (json_is_object(print_queue)) {
        log_config_section_header("PrintQueue");
        
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
                 config->print_queue.enabled ? "true" : "false");

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            log_config_section_item("QueueSettings", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = get_config_int(val, 1);
        log_config_section_item("DefaultPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.default_priority);
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
            log_config_section_item("EmergencyPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.emergency_priority);
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
            log_config_section_item("MaintenancePriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.maintenance_priority);
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = get_config_int(val, 3);
            log_config_section_item("SystemPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.system_priority);
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            log_config_section_item("Timeouts", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
        log_config_section_item("ShutdownDelay", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms",
                   config->print_queue.timeouts.shutdown_wait_ms);
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
            log_config_section_item("JobProcessingTimeout", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->print_queue.timeouts.job_processing_timeout_ms);
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            log_config_section_item("Buffers", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
            
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = get_config_size(val, 256);
        log_config_section_item("JobMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->print_queue.buffers.job_message_size);
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = get_config_size(val, 256);
            log_config_section_item("StatusMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->print_queue.buffers.status_message_size);
        }
    } else {
        config->print_queue.enabled = 1;
        config->print_queue.priorities.default_priority = 1;
        config->print_queue.priorities.emergency_priority = 0;
        config->print_queue.priorities.maintenance_priority = 2;
        config->print_queue.priorities.system_priority = 3;
        config->print_queue.timeouts.shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
        config->print_queue.timeouts.job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
        config->print_queue.buffers.job_message_size = 256;
        config->print_queue.buffers.status_message_size = 256;
        
        log_config_section_header("PrintQueue");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        log_config_section_header("API");
        
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        config->api.jwt_secret = get_config_string_with_env("JWTSecret", jwt_secret, "hydrogen_api_secret_change_me");
        log_config_section_item("JWTSecret", "%s", LOG_LEVEL_STATE, 
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0, 0, NULL, NULL,
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0 ? "(default)" : "configured");
    } else {
        config->api.jwt_secret = strdup("hydrogen_api_secret_change_me");
        log_config_section_header("API");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    }

    json_decref(root);
    
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