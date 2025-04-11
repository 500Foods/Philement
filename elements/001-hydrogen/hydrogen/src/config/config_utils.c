/*
 * Configuration utility functions implementation
 * 
 * Provides unified configuration processing and utilities:
 * - High-level config item processing
 * - Type-safe value handling
 * - Environment variable resolution
 * - Logging with proper formatting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <jansson.h>

#include "config_utils.h"
#include "../logging/logging.h"

// Thread-local storage for formatting
static __thread char value_buffer[1024];
static __thread char indent_buffer[32];

// Helper to check if a string starts with ${env.
static bool is_env_var_ref(const char* str) {
    return str && strncmp(str, "${env.", 6) == 0;
}

// Extract environment variable name from ${env.NAME} format
static const char* get_env_var_name(const char* str, char* buffer, size_t buffer_size) {
    if (!is_env_var_ref(str)) return NULL;
    
    const char* start = str + 6;  // Skip "${env."
    const char* end = strchr(start, '}');
    if (!end || end <= start) return NULL;
    
    size_t len = end - start;
    if (len >= buffer_size) len = buffer_size - 1;
    
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return buffer;
}

// Create indentation based on path depth
static const char* get_indent(const char* path) {
    if (!path) return "";
    
    int level = 0;
    const char* p = path;
    while ((p = strchr(p, '.')) != NULL) {
        level++;
        p++;
    }
    
    // Limit indentation level
    if (level > 5) level = 5;
    
    // Build indentation string
    char* pos = indent_buffer;
    for (int i = 0; i < level; i++) {
        *pos++ = '-';
        *pos++ = '-';
    }
    if (level > 0) {
        *pos++ = ' ';
    }
    *pos = '\0';
    
    return indent_buffer;
}

// Format a value that might be sensitive
static const char* format_sensitive(const char* value) {
    if (!value) return "(not set)";
    
    size_t len = strlen(value);
    if (len > 5) {
        snprintf(value_buffer, sizeof(value_buffer), "%.5s...", value);
    } else {
        snprintf(value_buffer, sizeof(value_buffer), "%s...", value);
    }
    return value_buffer;
}

// Check if a name indicates sensitive content
bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    const char* sensitive_terms[] = {
        "key", "token", "pass", "secret", "auth", "cred", 
        "cert", "jwt", "seed", "private", "hash", "salt",
        "cipher", "encrypt", "signature", "access"
    };
    
    for (size_t i = 0; i < sizeof(sensitive_terms)/sizeof(sensitive_terms[0]); i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    return false;
}

// Format integer for output
const char* format_int(int value) {
    snprintf(value_buffer, sizeof(value_buffer), "%d", value);
    return value_buffer;
}

// Format integer for buffer output (uses same implementation as format_int)
const char* format_int_buffer(int value) {
    return format_int(value);
}

/*
 * Process environment variable references and convert to appropriate JSON types
 * Handles type inference and conversion for environment values
 */
json_t* process_env_variable(const char* value) {
    const char* key_name = "EnvVar"; // Default key name for logging when not part of a specific config
    if (!value || strncmp(value, "${env.", 6) != 0) {
        return NULL;
    }
    
    // Find the closing brace
    const char* closing_brace = strchr(value + 6, '}');
    if (!closing_brace || *(closing_brace + 1) != '\0') {
        return NULL;  // Malformed or has trailing characters
    }
    
    // Extract the variable name
    size_t var_name_len = closing_brace - (value + 6);
    char* var_name = malloc(var_name_len + 1);
    if (!var_name) {
        return NULL;
    }
    
    strncpy(var_name, value + 6, var_name_len);
    var_name[var_name_len] = '\0';
    
    // Look up the environment variable
    const char* env_value = getenv(var_name);
    
    // Only log non-PAYLOAD_KEY environment variables
    if (env_value) {
        if (strcmp(var_name, "PAYLOAD_KEY") != 0) {
            if (is_sensitive_value(var_name)) {
                char safe_value[256];
                snprintf(safe_value, sizeof(safe_value), "$%s: ********", var_name);
                log_config_item(key_name, safe_value, false);
            } else {
                char value_buffer[512];
                snprintf(value_buffer, sizeof(value_buffer), "$%s: %s", var_name, env_value);
                log_config_item(key_name, value_buffer, false);
            }
        }
    } else {
        if (strcmp(var_name, "PAYLOAD_KEY") != 0) {
            char value_buffer[512];
            snprintf(value_buffer, sizeof(value_buffer), "$%s: not set", var_name);
            log_config_item(key_name, value_buffer, true);
        }
        free(var_name);
        return NULL;
    }
    
    if (env_value[0] == '\0') {
        free(var_name);
        return json_null();
    }
    
    // Check for boolean values (case insensitive)
    if (strcasecmp(env_value, "true") == 0) {
        free(var_name);
        return json_true();
    }
    if (strcasecmp(env_value, "false") == 0) {
        free(var_name);
        return json_false();
    }
    
    // Check if it's a number
    char* endptr;
    // Try parsing as integer first
    long long int_value = strtoll(env_value, &endptr, 10);
    if (*endptr == '\0') {
        // It's a valid integer
        free(var_name);
        return json_integer(int_value);
    }
    
    // Try parsing as double
    double real_value = strtod(env_value, &endptr);
    if (*endptr == '\0') {
        // It's a valid floating point number
        free(var_name);
        return json_real(real_value);
    }
    
    // Otherwise, treat it as a string
    free(var_name);
    return json_string(env_value);
}

// Format and log a configuration value
static void log_value(const char* path, const char* value, bool is_default, bool is_sensitive) {
    if (!path) return;
    
    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;
    const char* indent = get_indent(path);
    
    // Handle environment variable reference
    char env_name[256];
    if (value && is_env_var_ref(value)) {
        const char* env_var = get_env_var_name(value, env_name, sizeof(env_name));
        if (env_var) {
            const char* env_val = getenv(env_var);
            if (is_sensitive) {
                log_this("Config", "%s%s {%s}: %s%s", LOG_LEVEL_STATE,
                        indent, key, env_var,
                        format_sensitive(env_val ? env_val : "(not set)"),
                        is_default ? " *" : "");
            } else {
                log_this("Config", "%s%s {%s}: %s%s", LOG_LEVEL_STATE,
                        indent, key, env_var,
                        env_val ? env_val : "(not set)",
                        is_default ? " *" : "");
            }
            return;
        }
    }
    
    // Handle regular value
    if (is_sensitive) {
        log_this("Config", "%s%s: %s%s", LOG_LEVEL_STATE,
                indent, key, format_sensitive(value),
                is_default ? " *" : "");
    } else {
        log_this("Config", "%s%s: %s%s", LOG_LEVEL_STATE,
                indent, key, value ? value : "(not set)",
                is_default ? " *" : "");
    }
}

// Process a configuration value with full context
bool process_config_value(json_t* root, ConfigValue value, ConfigValueType type,
                         const char* path, const char* section) {
    if (!path) return false;
    
    // Handle section headers
    if (type == CONFIG_TYPE_SECTION) {
        log_this("Config", "%s%s", LOG_LEVEL_STATE,
                section, root ? "" : " *");
        return true;
    }
    
    // Get JSON value if config exists
    json_t* json_val = NULL;
    if (root) {
        char* path_copy = strdup(path);
        if (path_copy) {
            json_val = root;
            char* token = strtok(path_copy, ".");
            while (token && json_is_object(json_val)) {
                json_val = json_object_get(json_val, token);
                token = strtok(NULL, ".");
            }
            free(path_copy);
        }
    }
    
    bool using_default = !json_val;
    bool is_sensitive = (type == CONFIG_TYPE_SENSITIVE) || is_sensitive_value(path);
    
    // Process based on type
    switch (type) {
        case CONFIG_TYPE_BOOL: {
            bool val = *value.bool_val;  // Default value
            if (json_val && json_is_boolean(json_val)) {
                val = json_boolean_value(json_val);
                *value.bool_val = val;
                using_default = false;
            }
            log_value(path, val ? "true" : "false", using_default, false);
            return true;
        }
        
        case CONFIG_TYPE_INT: {
            int val = *value.int_val;  // Default value
            if (json_val && json_is_integer(json_val)) {
                val = (int)json_integer_value(json_val);
                *value.int_val = val;
                using_default = false;
            }
            snprintf(value_buffer, sizeof(value_buffer), "%d", val);
            log_value(path, value_buffer, using_default, false);
            return true;
        }
        
        case CONFIG_TYPE_STRING:
        case CONFIG_TYPE_SENSITIVE: {
            char** str_val = value.string_val;
            const char* json_str = NULL;
            
            if (json_val && json_is_string(json_val)) {
                json_str = json_string_value(json_val);
                if (json_str) {
                    if (*str_val) free(*str_val);
                    *str_val = strdup(json_str);
                    using_default = false;
                }
            }
            
            log_value(path, *str_val, using_default, is_sensitive);
            return true;
        }
        
        default:
            return false;
    }
}

// Log configuration section
void log_config_section(const char* section_name, bool using_defaults) {
    if (!section_name) return;
    log_this("Config", "%s%s", LOG_LEVEL_STATE,
             section_name, using_defaults ? " *" : "");
}

// Log configuration item
void log_config_item(const char* key, const char* value, bool is_default) {
    if (!key || !value) return;
    const char* indent_str = get_indent(key);
    log_this("Config", "%s%s: %s%s", LOG_LEVEL_STATE,
             indent_str, value, is_default ? " *" : "");
}