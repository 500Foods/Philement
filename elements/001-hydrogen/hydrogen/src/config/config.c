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
#include "types/config_string.h"
#include "types/config_bool.h"
#include "types/config_int.h"
#include "types/config_size.h"
#include "types/config_double.h"
#include "config_filesystem.h"
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
#include "restapi/config_api.h"

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
                                  const char* output_units, const char* subsystem, ...);
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
        char* result = strdup(default_value);
        log_config_section_item(json_key, "%s *", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", default_value);
        return result;
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
            char* result;
            if (env_value) {
                result = strdup(env_value);
                // For sensitive values, truncate in log
                if (is_sensitive_value(json_key)) {
                    char safe_value[256];
                    snprintf(safe_value, sizeof(safe_value), "$%s: %.5s...", var_name, env_value);
                    log_config_section_item(json_key, "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config-Env", safe_value);
                } else {
                    log_config_section_item(json_key, "$%s: %s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config-Env", var_name, env_value);
                }
            } else {
                // When env var is not set, we use default value - mark with single asterisk
                result = strdup(default_value);
                log_config_section_item(json_key, "$%s: not set, using %s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config-Env", var_name, default_value);
            }
            return result;
        }
    }

    // Not an environment variable, log and return the value
    char* result = strdup(str_value);
    log_config_section_item(json_key, "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", str_value);
    return result;
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
 * Helper function to handle environment variable logging
 * This function is exported for use by config_env.c
 * 
 * @param key_name The configuration key name
 * @param var_name The environment variable name
 * @param env_value The value from the environment variable
 * @param default_value The default value if not set
 * @param is_sensitive Whether this contains sensitive information
 */
void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                       const char* default_value, bool is_sensitive) {
    if (!var_name) return;

    if (env_value) {
        if (is_sensitive) {
            // For sensitive values, truncate to 5 chars
            char safe_value[256];
            strncpy(safe_value, env_value, 5);
            safe_value[5] = '\0';
            strcat(safe_value, "...");
            log_this("Config-Env", "― %s: $%s: %s", LOG_LEVEL_STATE, key_name, var_name, safe_value);
        } else {
            log_this("Config-Env", "― %s: $%s: %s", LOG_LEVEL_STATE, key_name, var_name, env_value);
        }
    } else if (default_value) {
        log_this("Config-Env", "― %s: $%s: (not set) %s *", LOG_LEVEL_STATE, key_name, var_name, default_value);
    } else {
        log_this("Config-Env", "― %s: $%s: (not set)", LOG_LEVEL_STATE, key_name, var_name);
    }
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
                           int indent, const char* input_units, const char* output_units, 
                           const char* subsystem, ...) {
    if (!key || !format) {
        return;
    }
    
    va_list args;
    va_start(args, subsystem);
    
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
    
    log_this(subsystem ? subsystem : "Config", "%s", level, message);
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
    #include "config/json_server.inc"

    // Logging Configuration
    #include "config/json_logging.inc"

    // System Resources Configuration
    // #include "config/json_resources.inc"
    
    // System Monitoring Configuration
    // #include "config/json_monitoring.inc"

    // Network Configuration
    // #include "config/json_network.inc"

    // WebServer Configuration
    // #include "config/json_webserver.inc"

    // RESTAPI Configuration
    // #include "config/json_restapi.inc"
    
    // Swagger Configuration
    // #include "config/json_swagger.inc"
    
    // WebSocket Configuration
   // #include "config/json_websocket.inc"

    // Terminal Configuration
    // #include "config/json_terminal.inc"

    // mDNS Server Configuration
    // #include "config/json_mdns_server.inc"

    // mDNSClient Configuration
    // #include "config/json_mdns_client.inc"

    // MailRelay Configuration
    // #include "config/json_mail_relay.inc"

    // Databases Configuration
    // #include "config/json_databases.inc"

    // Print Queue Configuration
    // #include "config/json_print_queue.inc"

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