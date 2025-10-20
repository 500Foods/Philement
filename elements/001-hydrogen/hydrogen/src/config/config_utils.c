/*
 * Configuration utility functions implementation
 * 
 * Provides unified configuration processing and utilities:
 * - High-level config item processing
 * - Type-safe value handling
 * - Environment variable resolution
 * - Logging with proper formatting
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_utils.h"

// Thread-local storage for formatting
static __thread char value_buffer[1024];

// Maximum length for a section name in log category
#define MAX_SECTION_LENGTH 248  // 256 - strlen("Config-") - 1

// Extract top-level section name from a dotted path
// Forward declarations for internal functions
const char* get_top_level_section(const char* section);
const char* get_indent(const char* path);

const char* get_top_level_section(const char* section) {
    static __thread char section_buffer[MAX_SECTION_LENGTH + 1];
    
    if (!section) return "Unknown";
    
    const char* dot = strchr(section, '.');
    size_t len;
    
    if (!dot) {
        len = strlen(section);
        if (len > MAX_SECTION_LENGTH) len = MAX_SECTION_LENGTH;
    } else {
        ptrdiff_t diff = dot - section;
        len = (size_t)diff;
        if (len > MAX_SECTION_LENGTH) len = MAX_SECTION_LENGTH;
    }
    
    strncpy(section_buffer, section, len);
    section_buffer[len] = '\0';
    return section_buffer;
}

// Process a log level with its name display
bool process_level_config(json_t* root, int* level_ptr, const char* level_name, 
                         const char* path, const char* section, int default_value) {
    if (!level_ptr || !path || !section) return false;

    // Check if the path exists in JSON
    bool using_default = true;

    // Use the provided default_value if specified, otherwise use the current value
    if (default_value >= 0) {
        *level_ptr = default_value;
    }
    
    if (root) {
        // Parse the path to traverse JSON
        char* path_copy = strdup(path);
        if (path_copy) {
            json_t* json_val = root;
            char* token = strtok(path_copy, ".");
            while (token && json_is_object(json_val)) {
                json_val = json_object_get(json_val, token);
                token = strtok(NULL, ".");
            }
            free(path_copy);
            
            // If we found a valid integer value, update the level
            if (json_val && json_is_integer(json_val)) {
                *level_ptr = (int)json_integer_value(json_val);
                using_default = false;
            }
        }
    }
    
    // Extract the key name for consistent logging format
    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;
    
    // Create log category
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));
    
    // Get indentation based on path depth
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s", path);
    const char* indent = get_indent(temp_path);
    
    // Log with level name format
    log_this(category, "%s%s: %d (%s)%s", LOG_LEVEL_DEBUG, 5,
            indent, 
            key, 
            *level_ptr, 
            level_name ? level_name : "unknown",
            using_default ? " *" : "");
            
    return true;
}

// Helper to check if a string starts with ${env.
bool is_env_var_ref(const char* str) {
    return str && strncmp(str, "${env.", 6) == 0;
}

// Extract environment variable name from ${env.NAME} format
const char* get_env_var_name(const char* str, char* buffer, size_t buffer_size) {
    if (!is_env_var_ref(str)) return NULL;
    
    const char* start = str + 6;  // Skip "${env."
    const char* end = strchr(start, '}');
    if (!end || end <= start) return NULL;
    
    ptrdiff_t diff = end - start;
    size_t len = (size_t)diff;
    if (len >= buffer_size) len = buffer_size - 1;
    
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return buffer;
}

// Create indentation based on path depth
const char* get_indent(const char* path) {
    if (!path) return "";
    
    // Count dots and array indices, up to max level
    int level = 0;
    while (*path && level < 5) {
        if (*path == '.') {
            level++;
        }
        path++;
    }
    
    // Precomputed indentation strings
    static const char indents[][32] = {
        "",
        "― ",
        "――― ",
        "――――― ",
        "――――――― ",
        "――――――――― "
    };
    
    return indents[level];
}

// Format a value that might be sensitive
const char* format_sensitive(const char* value) {
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
    if (!value || strncmp(value, "${env.", 6) != 0) {
        return NULL;
    }
    
    // Find the closing brace
    const char* closing_brace = strchr(value + 6, '}');
    if (!closing_brace || *(closing_brace + 1) != '\0') {
        return NULL;  // Malformed or has trailing characters
    }
    
    // Extract the variable name
    ptrdiff_t var_name_diff = closing_brace - (value + 6);
    if (var_name_diff < 0) {
        return NULL;
    }
    size_t var_name_len = (size_t)var_name_diff;
    char* var_name = malloc(var_name_len + 1);
    if (!var_name) {
        return NULL;
    }
    
    strncpy(var_name, value + 6, var_name_len);
    var_name[var_name_len] = '\0';
    
    // Look up the environment variable
    const char* env_value = getenv(var_name);

    if (env_value) {
        // Process the environment variable value
    } else {
        free(var_name);
        return NULL;
    }

    // Check if env_value is empty string
    if (strcmp(env_value, "") == 0) {
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

char* process_env_variable_string(const char* value) {
    json_t *result = process_env_variable(value);
    if (result && json_is_string(result)) {
        char *env_string = strdup(json_string_value(result));
        json_decref(result);
        return env_string;
    }
    json_decref(result);
    return NULL;
}

// Format and log a configuration value
void log_value(const char* path, const char* value, bool is_default, bool is_sensitive, const char* section) {
    if (!path) return;
    
    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;
    const char* indent = get_indent(path);
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));
    
    // Handle environment variable reference
    char env_name[256];
    if (value && is_env_var_ref(value)) {
        const char* env_var = get_env_var_name(value, env_name, sizeof(env_name));
        if (env_var) {
            const char* env_val = getenv(env_var);
            if (is_sensitive) {
                log_this(category, "%s%s {%s}: %s%s", LOG_LEVEL_DEBUG, 5,
                        indent, 
                        key, 
                        env_var,
                        format_sensitive(env_val ? env_val : "(not set)"),
                        is_default ? " *" : "");
            } else {
                log_this(category, "%s%s {%s}: %s%s", LOG_LEVEL_DEBUG, 5,
                        indent, 
                        key, 
                        env_var,
                        env_val ? env_val : "(not set)",
                        is_default ? " *" : "");
            }
            return;
        }
    }
    
    // Handle regular value
    if (is_sensitive) {
        log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
                indent, 
                key, 
                format_sensitive(value),
                is_default ? " *" : "");
    } else {
        log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
                indent, 
                key, 
                value ? value : "(not set)",
                is_default ? " *" : "");
    }
}

// Process a configuration value with full context
bool process_config_value(json_t* root, ConfigValue value, ConfigValueType type,
                         const char* path, const char* section) {
    if (!path) return false;
    
    // Handle section headers
    if (type == CONFIG_TYPE_SECTION) {
        char category[256];
        const char* top_section = get_top_level_section(section);
        snprintf(category, sizeof(category), "Config-%s", top_section);
        // Extract last part of section name for display
        const char* display_name = strrchr(section, '.');
        display_name = display_name ? display_name + 1 : section;
        const char* indent = get_indent(section);
        log_this(category, "%s%s%s", LOG_LEVEL_DEBUG, 3, indent, display_name, root ? "" : " *");
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
    
    bool is_sensitive = (type == CONFIG_TYPE_SENSITIVE) || is_sensitive_value(path);
    
    // Process based on type
    switch (type) {
        
        case CONFIG_TYPE_SECTION:
            // Already handled above
            return true;
            
        case CONFIG_TYPE_SUBSECTION:
            // Subsections are handled by parent logic
            return true;
            
        case CONFIG_TYPE_NULL:
            // NULL values are handled specially
            log_value(path, "(null)", true, false, section);
            return true;
            
        case CONFIG_TYPE_ENV_BOOL:
        case CONFIG_TYPE_ENV_INT:
        case CONFIG_TYPE_ENV_FLOAT:
        case CONFIG_TYPE_ENV_STRING:
        case CONFIG_TYPE_ENV_SENSITIVE:
            // Environment variable types are processed above
            return true;
            
        case CONFIG_TYPE_BOOL: {
            bool val = *value.bool_val;  // Default value
            bool using_default = !(json_val && json_is_boolean(json_val));
            if (!using_default) {
                val = json_boolean_value(json_val);
                *value.bool_val = val;
            }
            log_value(path, val ? "true" : "false", using_default, false, section);
            return true;
        }
        
        case CONFIG_TYPE_INT: {
            int val = *value.int_val;  // Default value
            bool using_default = !(json_val && json_is_integer(json_val));
            if (!using_default) {
                val = (int)json_integer_value(json_val);
                *value.int_val = val;
            }
            snprintf(value_buffer, sizeof(value_buffer), "%d", val);
            log_value(path, value_buffer, using_default, false, section);
            return true;
        }

        case CONFIG_TYPE_FLOAT: {
            double val = *value.float_val;  // Default value
            bool using_default = !(json_val && (json_is_real(json_val) || json_is_integer(json_val)));
            if (!using_default) {
                if (json_is_real(json_val)) {
                    val = json_real_value(json_val);
                } else {
                    val = (double)json_integer_value(json_val);
                }
                *value.float_val = val;
            }
            snprintf(value_buffer, sizeof(value_buffer), "%.2f", val);
            log_value(path, value_buffer, using_default, false, section);
            return true;
        }
        
        case CONFIG_TYPE_STRING:
        case CONFIG_TYPE_SENSITIVE: {
            char** str_val = value.string_val;
            const char* final_value = NULL;
            const char* env_var_name = NULL;
            bool using_default = true;
            char original_ref[256] = {0};

            // Check current value for environment variable reference
            if (*str_val && is_env_var_ref(*str_val)) {
                strncpy(original_ref, *str_val, sizeof(original_ref) - 1);
                char env_name[256];
                env_var_name = get_env_var_name(*str_val, env_name, sizeof(env_name));
                if (env_var_name) {
                    const char* env_val = getenv(env_var_name);
                    if (env_val) {
                        free(*str_val);
                        *str_val = strdup(env_val);
                        final_value = *str_val;
                        using_default = false;
                    }
                }
            }

            // Check JSON value for environment variable reference
            if (using_default && json_val && json_is_string(json_val)) {
                const char* json_str = json_string_value(json_val);
                if (json_str && is_env_var_ref(json_str)) {
                    strncpy(original_ref, json_str, sizeof(original_ref) - 1);
                    char env_name[256];
                    env_var_name = get_env_var_name(json_str, env_name, sizeof(env_name));
                    if (env_var_name) {
                        const char* env_val = getenv(env_var_name);
                        if (env_val) {
                            if (*str_val) free(*str_val);
                            *str_val = strdup(env_val);
                            final_value = *str_val;
                            using_default = false;
                        }
                    }
                } else if (json_str) {
                    if (*str_val) free(*str_val);
                    *str_val = strdup(json_str);
                    final_value = *str_val;
                    using_default = false;
                }
            }

            // Use structure value if no other value set
            if (!final_value) {
                final_value = *str_val;
            }

            // Log with environment variable name if available
            const char* key = strrchr(path, '.');
            key = key ? key + 1 : path;
            char category[256];
            snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));
            const char* indent = get_indent(path);

            if (original_ref[0] != '\0') {
                // Extract variable name from ${env.VAR_NAME}
                const char* env_pos = strstr(original_ref, "${env.");
                if (env_pos) {
                    const char* var_start = env_pos + 6;
                    const char* var_end = strchr(var_start, '}');
                    if (var_end) {
                        char env_var_buffer[256];  // Renamed to avoid shadow warning
                        size_t var_len = (size_t)(var_end - var_start);
                        strncpy(env_var_buffer, var_start, var_len);
                        env_var_buffer[var_len] = '\0';

                        if (is_sensitive) {
                            log_this(category, "%s%s {%s}: %s%s", LOG_LEVEL_DEBUG, 5,
                                    indent,
                                    key,
                                    env_var_buffer,
                                    format_sensitive(final_value ? final_value : "(not set)"),
                                    using_default ? " *" : "");
                        } else {
                            log_this(category, "%s%s {%s}: %s%s", LOG_LEVEL_DEBUG, 5,
                                    indent,
                                    key,
                                    env_var_buffer,
                                    final_value ? final_value : "(not set)",
                                    using_default ? " *" : "");
                        }
                    } else {
                        log_value(path, final_value, using_default, is_sensitive, section);
                    }
                } else {
                    log_value(path, final_value, using_default, is_sensitive, section);
                }
            } else {
                log_value(path, final_value, using_default, is_sensitive, section);
            }
            return true;
        }
        
        default:
            return false;
    }
}

// Log configuration section
void log_config_section(const char* section_name, bool using_defaults) {
    if (!section_name) return;
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section_name));
    log_this(category, "%s%s", LOG_LEVEL_DEBUG, 2, section_name, using_defaults ? " *" : "");
}

// Log configuration item
void log_config_item(const char* key, const char* value, bool is_default, const char* section) {
    if (!key || !value) return;
    const char* indent_str = get_indent(key);
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));
    log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
             indent_str, 
             key,
             value, 
             is_default ? " *" : "");
}

// Format array output
const char* format_int_array(const int* array, size_t count) {
    if (!array || count == 0) {
        return "[none]";  // Show [none] for empty arrays
    }

    // Start with opening bracket
    size_t pos = 0;
    value_buffer[pos++] = '[';

    // Add each number
    for (size_t i = 0; i < count; i++) {
        // Add comma if not first number
        if (i > 0) {
            value_buffer[pos++] = ',';
        }
        
        // Convert number to string
        int written = snprintf(value_buffer + pos, sizeof(value_buffer) - pos, "%d", array[i]);
        if (written < 0 || (size_t)written >= (sizeof(value_buffer) - pos)) {
            return "[...]"; // Buffer would overflow
        }
        pos += (size_t)written;
    }

    // Add closing bracket
    if (pos < sizeof(value_buffer) - 1) {
        value_buffer[pos++] = ']';
        value_buffer[pos] = '\0';
        return value_buffer;
    }

    return "[...]"; // Buffer would overflow
}

// Format string array for output
const char* format_string_array(const char** array, size_t count) {
    if (!array || count == 0) {
        return "[none]";  // Show [none] for empty arrays
    }

    // Start with opening bracket
    size_t pos = 0;
    value_buffer[pos++] = '[';

    // Add each string
    for (size_t i = 0; i < count; i++) {
        // Add comma if not first string
        if (i > 0) {
            if (pos >= sizeof(value_buffer) - 2) return "[...]";
            value_buffer[pos++] = ',';
            value_buffer[pos++] = ' ';
        }
        
        // Add the string in quotes
        if (pos >= sizeof(value_buffer) - 3) return "[...]";
        value_buffer[pos++] = '"';
        
        const char* str = array[i] ? array[i] : "(null)";
        size_t str_len = strlen(str);
        if (pos + str_len >= sizeof(value_buffer) - 3) return "[...]";
        
        strcpy(value_buffer + pos, str);
        pos += str_len;
        
        value_buffer[pos++] = '"';
    }

    // Add closing bracket
    if (pos >= sizeof(value_buffer) - 2) return "[...]";
    value_buffer[pos++] = ']';
    value_buffer[pos] = '\0';
    return value_buffer;
}

// Process array configurations
bool process_int_array_config(json_t* root, ConfigIntArray value, const char* path, const char* section) {
    if (!path || !value.array || !value.count || value.capacity == 0) {
        return false;
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

    // Default to empty array
    *value.count = 0;
    bool using_default = true;

    // Process array if present
    if (json_val && json_is_array(json_val)) {
        size_t index;
        json_t* element;
        using_default = false;

        json_array_foreach(json_val, index, element) {
            if (index >= value.capacity) {
                break;  // Array is full
            }

            if (json_is_integer(element)) {
                value.array[(*value.count)++] = (int)json_integer_value(element);
            }
        }
    }

    // Log the array with proper indentation
    const char* indent = get_indent(path);
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));

    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;

    log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
            indent, 
            key, 
            format_int_array(value.array, *value.count),
            using_default ? " *" : "");

    return true;
}

// Process array element configuration
bool process_array_element_config(json_t* root, ConfigArrayElement value, const char* path, const char* section) {
    if (!value.element || !path || !section) return false;

    // Extract the key name for display (last part of path)
    const char* display_key = strrchr(path, '.');
    display_key = display_key ? display_key + 1 : path;
    
    // Determine log category
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));

    // Get JSON array using path traversal for nested objects
    json_t* json_val = NULL;
    bool using_default = true;
    
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

    // Check if we got an array and element
    const char* str = NULL;
    if (json_val && json_is_array(json_val)) {
        // Get element at index
        json_t* element = json_array_get(json_val, value.index);
        if (json_is_string(element)) {
            str = json_string_value(element);
            if (str) {
                char* new_element = strdup(str);
                if (!new_element) {
                    // Memory allocation failed, but keep existing value
                    log_this(category, "Memory allocation failed for array element", LOG_LEVEL_ERROR, 0);
                    return false;
                }
                // Only free old value after new allocation succeeds
                if (*value.element) free(*value.element);
                *value.element = new_element;
                using_default = false;
            }
        }
    }

    // If we're using the default value, get it for logging
    if (using_default && *value.element) {
        str = *value.element;
    }

    // Get indent level for array elements (don't add extra dot)
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s.", path);
    const char* indent = get_indent(temp_path);

    // Format array element key with index
    char key_with_index[256];
    snprintf(key_with_index, sizeof(key_with_index), "%s[%zu]", display_key, value.index);
    
    log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
             indent, 
             key_with_index, 
             str ? str : "(not set)",
             using_default ? " *" : "");
    
    return true;
}

// Process string array configuration
bool process_string_array_config(json_t* root, ConfigStringArray value, const char* path, const char* section) {
    if (!path || !value.array || !value.count || value.capacity == 0) {
        return false;
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

    // Default to empty array
    *value.count = 0;
    bool using_default = true;

    // Process array if present
    if (json_val && json_is_array(json_val)) {
        size_t index;
        json_t* element;
        using_default = false;

        json_array_foreach(json_val, index, element) {
            if (index >= value.capacity) {
                break;  // Array is full
            }

            if (json_is_string(element)) {
                const char* str = json_string_value(element);
                if (str) {
                    value.array[index] = strdup(str);
                    if (value.array[index]) {
                        (*value.count)++;
                    }
                }
            }
        }
    }

    // Log the array with proper indentation
    const char* indent = get_indent(path);
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));

    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;

    log_this(category, "%s%s: %s%s", LOG_LEVEL_DEBUG, 4,
            indent, 
            key, 
            format_string_array((const char**)value.array, *value.count),
            using_default ? " *" : "");

    return true;
}

// Process a direct boolean value (no JSON lookup)
bool process_direct_bool_value(ConfigValue value, const char* path, const char* section, bool direct_value) {
    if (!path || !value.bool_val) return false;
    
    // Set the value
    *value.bool_val = direct_value;
    
    // Log with proper indentation
    const char* indent = get_indent(path);
    char category[256];
    snprintf(category, sizeof(category), "Config-%s", get_top_level_section(section));

    const char* key = strrchr(path, '.');
    key = key ? key + 1 : path;

    log_this(category, "%s%s: %s", LOG_LEVEL_DEBUG, 3, indent, key, direct_value ? "enabled" : "disabled");

    return true;
}

// Process a direct configuration value (no JSON lookup)
bool process_direct_value(ConfigValue value, ConfigValueType type,
                        const char* path, const char* section,
                        const char* direct_value) {
    if (!path || !direct_value) return false;
    
    bool is_sensitive = (type == CONFIG_TYPE_SENSITIVE) || is_sensitive_value(path);
    
    // Process based on type
    switch (type) {
        case CONFIG_TYPE_SECTION:
        case CONFIG_TYPE_SUBSECTION:
        case CONFIG_TYPE_NULL:
            // These types are not supported for direct values
            return false;
            
        case CONFIG_TYPE_BOOL:
        case CONFIG_TYPE_INT:
        case CONFIG_TYPE_FLOAT:
        case CONFIG_TYPE_ENV_BOOL:
        case CONFIG_TYPE_ENV_INT:
        case CONFIG_TYPE_ENV_FLOAT:
            // These types would need additional implementation
            return false;
            
        case CONFIG_TYPE_ENV_STRING:
        case CONFIG_TYPE_ENV_SENSITIVE:
            // Environment variable types should be processed differently
            return false;
            
        case CONFIG_TYPE_STRING:
        case CONFIG_TYPE_SENSITIVE: {
            char** str_val = value.string_val;
            if (!str_val) return false;
            
            // Free existing value if any
            if (*str_val) {
                free(*str_val);
            }
            
            // Set the new value from direct_value
            *str_val = strdup(direct_value);
            if (!*str_val) return false;
            
            // Direct values are never default since they're explicitly provided
            log_value(path, direct_value, false, is_sensitive, section);
            return true;
        }
    }
    
    // This should never be reached
    return false;
}
