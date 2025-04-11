/*
 * Configuration utility functions implementation
 * 
 * Provides common utilities used across configuration modules:
 * - Sensitive value detection
 * - Configuration logging
 * - Common configuration patterns
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// Project headers
#include <jansson.h>
#include "config_utils.h"
#include "env/config_env_utils.h"
#include "../logging/logging.h"

/*
 * Helper function to format integer values for configuration output
+ * Thread-safe using thread-local storage
+ */
static __thread char int_buffer[32];

const char* format_int_buffer(int value) {
    snprintf(int_buffer, sizeof(int_buffer), "%d", value);
    return int_buffer;
}

/*
 * Helper function to detect sensitive configuration values
 */
bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    const char* sensitive_terms[] = {
        "key", "token", "pass", "secret", "auth", "cred", 
        "cert", "jwt", "seed", "private", "hash", "salt",
        "cipher", "encrypt", "signature", "access"
    };
    const int num_terms = sizeof(sensitive_terms) / sizeof(sensitive_terms[0]);
    
    for (int i = 0; i < num_terms; i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    
    return false;
}

/*
 * Configuration logging utilities
 */

void log_config_section_header(const char* section_name) {
    if (!section_name) return;
    log_this("Config", "%s", LOG_LEVEL_STATE, section_name);
}

void log_config_section_item(const char* key, const char* format, int level, int is_default,
                           int indent, const char* input_units, const char* output_units, 
                           const char* subsystem, ...) {
    if (!key || !format) return;
    
    va_list args;
    va_start(args, subsystem);
    
    char value_buffer[1024] = {0};
    vsnprintf(value_buffer, sizeof(value_buffer), format, args);
    va_end(args);
    
    char message[2048] = {0};
    
    // Add indent prefix
    const char* indents[] = {
        "― ",
        "――― ",
        "――――― ",
        "――――――― ",
        "――――――――― "
    };
    if (indent >= 0 && indent < 5) {
        strncat(message, indents[indent], sizeof(message) - strlen(message) - 1);
    } else {
        strncat(message, indents[4], sizeof(message) - strlen(message) - 1);
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
            strncat(message, value_buffer, sizeof(message) - strlen(message) - 1);
            strncat(message, " ", sizeof(message) - strlen(message) - 1);
            strncat(message, output_units, sizeof(message) - strlen(message) - 1);
        } else {
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

void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                         const char* default_value, bool is_sensitive) {
    if (!var_name) return;

    if (env_value) {
        if (is_sensitive) {
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
 * Common configuration patterns
 */

void log_config_section(const char* section_name, bool using_defaults) {
    if (!section_name) return;
    log_this("Config", "%s%s", LOG_LEVEL_STATE, section_name, using_defaults ? " *" : "");
}

void log_config_item(const char* key, const char* value, bool is_default, int indent) {
    if (!key || !value) return;
    
    const char* indents[] = {
        "― ",
        "――― ",
        "――――― ",
        "――――――― ",
        "――――――――― "
    };
    
    const char* prefix = (indent >= 0 && indent < 5) ? indents[indent] : indents[4];
    log_this("Config", "%s%s: %s%s", LOG_LEVEL_STATE, prefix, key, value, 
             is_default ? " *" : "");
}

void log_config_sensitive_item(const char* key, const char* value, bool is_default, int indent) {
    if (!key || !value) return;
    
    const char* indents[] = {
        "― ",
        "――― ",
        "――――― ",
        "――――――― ",
        "――――――――― "
    };
    
    const char* prefix = (indent >= 0 && indent < 5) ? indents[indent] : indents[4];
    
    // Truncate sensitive values to 5 chars + "..."
    char safe_value[256];
    strncpy(safe_value, value, 5);
    safe_value[5] = '\0';
    strcat(safe_value, "...");
    
    log_this("Config", "%s%s: %s%s", LOG_LEVEL_STATE, prefix, key, safe_value,
             is_default ? " *" : "");
}

char* process_config_env_var(const char* key, json_t* value, const char* default_value, 
                           bool is_sensitive, bool is_default) {
    char* result = get_config_string_with_env(key, value, default_value);
    if (!result) return NULL;

    // Check if this is an environment variable
    if (strncmp(result, "${env.", 6) == 0) {
        const char* env_name = result + 6;
        size_t len = strlen(env_name);
        if (len > 1 && env_name[len-1] == '}') {
            char env_var[256];
            size_t copy_len = len - 1;
            if (copy_len >= sizeof(env_var)) {
                copy_len = sizeof(env_var) - 1;
            }
            memcpy(env_var, env_name, copy_len);
            env_var[copy_len] = '\0';
            
            const char* env_val = getenv(env_var);
            log_config_env_value(key, env_var, env_val, default_value, is_sensitive);
        }
    } else if (result[0] != '\0') {
        log_config_item(key, result, is_default, 1);
    } else {
        log_config_item(key, "(not set)", is_default, 1);
    }

    return result;
}