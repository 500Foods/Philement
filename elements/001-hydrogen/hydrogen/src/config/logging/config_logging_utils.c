/*
 * Configuration logging utilities
 * 
 * Provides standardized logging for configuration:
 * - Section headers
 * - Configuration items with proper indentation
 * - Unit conversion for display
 * - Environment variable resolution tracking
 * - Default value indicators
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// Project headers
#include "../config_utils.h"
#include "../../logging/logging.h"
#include "config_logging_utils.h"

/*
 * Helper function to log a configuration section header
 * 
 * This function logs a section header with the name of the configuration section.
 * It's used to group related configuration items in the log.
 * 
 * @param section_name The name of the configuration section
 */
void log_config_section_header(const char* section_name) {
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
 * @param subsystem The subsystem logging the message (e.g., "Config", "Config-Env")
 * @param ... Variable arguments for the format string
 */
void log_config_section_item(const char* key, const char* format, int level, int is_default,
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
 * Helper function to handle environment variable logging
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