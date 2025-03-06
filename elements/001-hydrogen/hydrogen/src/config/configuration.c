/*
 * Configuration management system with robust fallback handling
 * 
 * The configuration system implements several key design principles:
 * 
 * Fault Tolerance:
 * - Graceful fallback to defaults for missing values
 * - Validation of critical parameters
 * - Type checking for all values
 * - Memory allocation failure recovery
 * 
 * Flexibility:
 * - Runtime configuration changes
 * - Environment-specific overrides
 * - Service-specific settings
 * - Extensible structure
 * 
 * Security:
 * - Sensitive data isolation
 * - Path validation
 * - Size limits enforcement
 * - Access control settings
 * 
 * Maintainability:
 * - Centralized default values
 * - Structured error reporting
 * - Clear upgrade paths
 * - Configuration versioning
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>
#include <errno.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "configuration.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

/*
 * Resolve environment variable references in configuration values
 * 
 * Processes a string of the form "${env.VARIABLE}" by:
 * 1. Extracting the variable name after "env."
 * 2. Looking up the variable in the environment
 * 3. Converting the value to an appropriate JSON type:
 *    - If variable doesn't exist: return NULL
 *    - If variable exists but is empty: return JSON null
 *    - If variable is "true"/"false" (case insensitive): return JSON boolean
 *    - If variable is a valid number: return JSON integer or real
 *    - Otherwise: return JSON string
 * 
 * Memory Management:
 * - Returns a new JSON value that caller must json_decref
 * - Returns NULL if input is not in "${env.VARIABLE}" format or variable doesn't exist
 */
static json_t* process_env_variable(const char* value) {
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
        
        // Log the result of the lookup once, consolidating all information
        if (env_value) {
            // Check if this is a sensitive variable (key, password, token, secret)
            bool is_sensitive = (strstr(var_name, "KEY") != NULL || 
                             strstr(var_name, "TOKEN") != NULL || 
                             strstr(var_name, "PASSWORD") != NULL || 
                             strstr(var_name, "SECRET") != NULL ||
                             strstr(var_name, "CERT") != NULL);
        
            // Create a safe value for logging
            char safe_value[256];
            if (is_sensitive && strlen(env_value) > 5) {
                strncpy(safe_value, env_value, 5);
                safe_value[5] = '\0';
                strcat(safe_value, "...");
            } else {
                strncpy(safe_value, env_value, sizeof(safe_value) - 1);
                safe_value[sizeof(safe_value) - 1] = '\0';
            }
        
            // Determine the type 
            const char* type_str = "string";
        
            // If variable exists but is empty, it's null
            if (env_value[0] == '\0') {
                type_str = "null";
            }
            // Check for boolean values
            else if (strcasecmp(env_value, "true") == 0 || strcasecmp(env_value, "false") == 0) {
                type_str = "boolean";
            }
            // Check if it's an integer
            else {
                char* endptr;
                strtoll(env_value, &endptr, 10);
                if (*endptr == '\0') {
                    type_str = "integer";
                }
                // Check if it's a double
                else {
                    strtod(env_value, &endptr);
                    if (*endptr == '\0') {
                        type_str = "double";
                    }
                }
            }
            
            // Single consolidated log message with all the information
            log_this("Environment", "Variable: %s, Type: %s, Value: '%s'", LOG_LEVEL_INFO, 
                     var_name, type_str, safe_value);
            
        } else {
            // Just log that the variable wasn't found and we'll be using default
            log_this("Environment", "Variable: %s not found, using default", LOG_LEVEL_INFO, var_name);
        }
    
    free(var_name);
    
    // If variable doesn't exist, return NULL
    if (!env_value) {
        return NULL;
    }
    
    // If variable exists but is empty, return JSON null
    if (env_value[0] == '\0') {
        log_this("Configuration", "Environment variable value is empty, using NULL", LOG_LEVEL_DEBUG);
        return json_null();
    }
    
    // Check for boolean values (case insensitive)
    if (strcasecmp(env_value, "true") == 0) {
        log_this("Configuration", "Converting environment variable value to boolean true", LOG_LEVEL_DEBUG);
        return json_true();
    }
    if (strcasecmp(env_value, "false") == 0) {
        log_this("Configuration", "Converting environment variable value to boolean false", LOG_LEVEL_DEBUG);
        return json_false();
    }
    
    // Check if it's a number
    char* endptr;
    // Try parsing as integer first
    long long int_value = strtoll(env_value, &endptr, 10);
    if (*endptr == '\0') {
        // It's a valid integer
        log_this("Configuration", "Converting environment variable value to integer: %lld", LOG_LEVEL_DEBUG, int_value);
        return json_integer(int_value);
    }
    
    // Try parsing as double
    double real_value = strtod(env_value, &endptr);
    if (*endptr == '\0') {
        // It's a valid floating point number
        log_this("Configuration", "Converting environment variable value to real: %f", LOG_LEVEL_DEBUG, real_value);
        return json_real(real_value);
    }
    
    // Otherwise, treat it as a string
    log_this("Configuration", "Using environment variable value as string", LOG_LEVEL_DEBUG);
    return json_string(env_value);
}

// Function removed - was unused

/*
 * Enhanced get_config_string that properly handles environment variables and logs results
 * 
 * This function:
 * 1. Checks if the value is a JSON string
 * 2. If it's a string with the pattern ${env.VARIABLE}, processes it as an environment variable
 * 3. Handles type conversion and logging for the result
 * 4. Falls back to default value if needed
 */
static char* get_config_string(json_t* value, const char* default_value) {
    if (!value) {
        log_this("Configuration", "Using default string value: %s", LOG_LEVEL_DEBUG, 
                 default_value ? default_value : "(null)");
        return default_value ? strdup(default_value) : NULL;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
            }
            
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                char* result = NULL;
                
                // Convert the environment variable value to a string based on its type
                if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    log_this("Configuration", "Using environment variable as string: %s", LOG_LEVEL_DEBUG, env_str);
                    result = strdup(env_str);
                } else if (json_is_null(env_value)) {
                    log_this("Configuration", "Environment variable is null, using default: %s", LOG_LEVEL_DEBUG,
                             default_value ? default_value : "(null)");
                    result = default_value ? strdup(default_value) : NULL;
                } else if (json_is_boolean(env_value)) {
                    const char* bool_str = json_is_true(env_value) ? "true" : "false";
                    log_this("Configuration", "Converting boolean environment variable to string: %s", LOG_LEVEL_DEBUG, bool_str);
                    result = strdup(bool_str);
                } else if (json_is_integer(env_value)) {
                    char buffer[64];
                    json_int_t int_val = json_integer_value(env_value);
                    snprintf(buffer, sizeof(buffer), "%lld", int_val);
                    log_this("Configuration", "Converting integer environment variable to string: %s", LOG_LEVEL_DEBUG, buffer);
                    result = strdup(buffer);
                } else if (json_is_real(env_value)) {
                    char buffer[64];
                    double real_val = json_real_value(env_value);
                    snprintf(buffer, sizeof(buffer), "%f", real_val);
                    log_this("Configuration", "Converting real environment variable to string: %s", LOG_LEVEL_DEBUG, buffer);
                    result = strdup(buffer);
                }
                
                json_decref(env_value);
                if (result) {
                    return result;
                }
            }
            
            // Environment variable not found or empty, use default
            if (var_name[0] != '\0') {
                log_this("Configuration", "Using default for %s: %s", LOG_LEVEL_INFO,
                     var_name, default_value ? default_value : "(default)");
            } else {
                log_this("Configuration", "Environment variable not found, using default string: %s", LOG_LEVEL_DEBUG,
                     default_value ? default_value : "(null)");
            }
            return default_value ? strdup(default_value) : NULL;
        }
        
        // Not an environment variable, just use the string value without logging
        return strdup(str_value);
    }
    
    // Handle non-string JSON values by converting to string
    if (json_is_boolean(value)) {
        return strdup(json_is_true(value) ? "true" : "false");
    } else if (json_is_integer(value)) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%lld", json_integer_value(value));
        return strdup(buffer);
    } else if (json_is_real(value)) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%f", json_real_value(value));
        return strdup(buffer);
    } else {
        // For other types or null, use default
        log_this("Configuration", "JSON value is not convertible to string, using default: %s", LOG_LEVEL_DEBUG,
                 default_value ? default_value : "(null)");
        return default_value ? strdup(default_value) : NULL;
    }
}

/*
 * Enhanced get_config_bool function that properly handles environment variables and logs results
 */
static int get_config_bool(json_t* value, int default_value) {
    if (!value) {
        log_this("Configuration", "Using default boolean value: %d", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                int result = default_value;
                
                if (json_is_boolean(env_value)) {
                    result = json_boolean_value(env_value);
                    log_this("Configuration", "Using environment variable as boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_integer(env_value)) {
                    result = json_integer_value(env_value) != 0;
                    log_this("Configuration", "Converting integer environment variable to boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_real(env_value)) {
                    result = json_real_value(env_value) != 0.0;
                    log_this("Configuration", "Converting real environment variable to boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    if (strcasecmp(env_str, "true") == 0 || strcmp(env_str, "1") == 0) {
                        result = 1;
                        log_this("Configuration", "Converting string environment variable '%s' to boolean true", 
                                 LOG_LEVEL_DEBUG, env_str);
                    } else if (strcasecmp(env_str, "false") == 0 || strcmp(env_str, "0") == 0) {
                        result = 0;
                        log_this("Configuration", "Converting string environment variable '%s' to boolean false", 
                                 LOG_LEVEL_DEBUG, env_str);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid boolean, using default: %s", 
                                 LOG_LEVEL_DEBUG, env_str, default_value ? "true" : "false");
                    }
                } else {
                    log_this("Configuration", "Environment variable not a boolean type, using default: %s", 
                             LOG_LEVEL_DEBUG, default_value ? "true" : "false");
                }
                
                json_decref(env_value);
                return result;
            }
            
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
                
                // Log at INFO level for better visibility and test compatibility
                log_this("Configuration", "Using default for %s: %s", LOG_LEVEL_INFO,
                     var_name, default_value ? "true" : "false");
            } else {
                log_this("Configuration", "Environment variable not found, using default boolean: %s", 
                     LOG_LEVEL_DEBUG, default_value ? "true" : "false");
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to boolean
        const char* str = json_string_value(value);
        if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0) {
            log_this("Configuration", "Converting string '%s' to boolean true", LOG_LEVEL_DEBUG, str);
            return 1;
        } else if (strcasecmp(str, "false") == 0 || strcmp(str, "0") == 0) {
            log_this("Configuration", "Converting string '%s' to boolean false", LOG_LEVEL_DEBUG, str);
            return 0;
        }
        
        log_this("Configuration", "String '%s' is not a valid boolean, using default: %s", 
                 LOG_LEVEL_DEBUG, str, default_value ? "true" : "false");
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_boolean(value)) {
        return json_boolean_value(value);
    } else if (json_is_integer(value)) {
        return json_integer_value(value) != 0;
    } else if (json_is_real(value)) {
        return json_real_value(value) != 0.0;
    }
    
    log_this("Configuration", "JSON value is not convertible to boolean, using default: %s", 
             LOG_LEVEL_DEBUG, default_value ? "true" : "false");
    return default_value;
}

/*
 * Enhanced get_config_int function that properly handles environment variables and logs results
 */
static int get_config_int(json_t* value, int default_value) {
    if (!value) {
        log_this("Configuration", "Using default integer value: %d", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                int result = default_value;
                
                if (json_is_integer(env_value)) {
                    result = json_integer_value(env_value);
                    log_this("Configuration", "Using environment variable as integer: %d", LOG_LEVEL_DEBUG, result);
                } else if (json_is_real(env_value)) {
                    result = (int)json_real_value(env_value);
                    log_this("Configuration", "Converting real environment variable to integer: %d", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_boolean(env_value)) {
                    result = json_is_true(env_value) ? 1 : 0;
                    log_this("Configuration", "Converting boolean environment variable to integer: %d", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_string(env_value)) {
                    // Try to parse as integer
                    const char* env_str = json_string_value(env_value);
                    char* endptr;
                    long val = strtol(env_str, &endptr, 10);
                    if (*endptr == '\0') {
                        result = val;
                        log_this("Configuration", "Converting string environment variable '%s' to integer: %d", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid integer, using default: %d", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else {
                    log_this("Configuration", "Environment variable not an integer type, using default: %d", 
                             LOG_LEVEL_DEBUG, default_value);
                }
                
                json_decref(env_value);
                return result;
            }
            
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
                
                // Log at INFO level for better visibility and test compatibility
                log_this("Configuration", "Using default for %s: %d", LOG_LEVEL_INFO,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default integer: %d", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to integer
        const char* str = json_string_value(value);
        char* endptr;
        long val = strtol(str, &endptr, 10);
        if (*endptr == '\0') {
            log_this("Configuration", "Converting string '%s' to integer: %ld", LOG_LEVEL_DEBUG, str, val);
            return val;
        }
        
        log_this("Configuration", "String '%s' is not a valid integer, using default: %d", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_integer(value)) {
        return json_integer_value(value);
    } else if (json_is_real(value)) {
        return (int)json_real_value(value);
    } else if (json_is_boolean(value)) {
        return json_is_true(value) ? 1 : 0;
    }
    
    log_this("Configuration", "JSON value is not convertible to integer, using default: %d", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}

/*
 * Enhanced get_config_size function that properly handles environment variables and logs results
 */
static size_t get_config_size(json_t* value, size_t default_value) {
    if (!value) {
        log_this("Configuration", "Using default size value: %zu", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                size_t result = default_value;
                
                if (json_is_integer(env_value)) {
                    json_int_t val = json_integer_value(env_value);
                    if (val >= 0) {
                        result = (size_t)val;
                        log_this("Configuration", "Using environment variable as size_t: %zu", LOG_LEVEL_DEBUG, result);
                    } else {
                        log_this("Configuration", "Integer environment variable is negative, using default size: %zu", 
                                 LOG_LEVEL_DEBUG, default_value);
                    }
                } else if (json_is_real(env_value)) {
                    double val = json_real_value(env_value);
                    if (val >= 0) {
                        result = (size_t)val;
                        log_this("Configuration", "Converting real environment variable to size_t: %zu", 
                                 LOG_LEVEL_DEBUG, result);
                    } else {
                        log_this("Configuration", "Real environment variable is negative, using default size: %zu", 
                                 LOG_LEVEL_DEBUG, default_value);
                    }
                } else if (json_is_string(env_value)) {
                    // Try to parse as size_t
                    const char* env_str = json_string_value(env_value);
                    char* endptr;
                    unsigned long long val = strtoull(env_str, &endptr, 10);
                    if (*endptr == '\0') {
                        result = (size_t)val;
                        log_this("Configuration", "Converting string environment variable '%s' to size_t: %zu", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid size_t, using default: %zu", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else {
                    log_this("Configuration", "Environment variable not a size_t type, using default: %zu", 
                             LOG_LEVEL_DEBUG, default_value);
                }
                
                json_decref(env_value);
                return result;
            }
            
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
                
                // Log at INFO level for better visibility and test compatibility
                log_this("Configuration", "Using default for %s: %zu", LOG_LEVEL_INFO,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default size: %zu", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to size_t
        const char* str = json_string_value(value);
        char* endptr;
        unsigned long long val = strtoull(str, &endptr, 10);
        if (*endptr == '\0') {
            log_this("Configuration", "Converting string '%s' to size_t: %llu", LOG_LEVEL_DEBUG, str, val);
            return (size_t)val;
        }
        
        log_this("Configuration", "String '%s' is not a valid size_t, using default: %zu", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_integer(value)) {
        json_int_t val = json_integer_value(value);
        if (val >= 0) {
            return (size_t)val;
        }
        log_this("Configuration", "JSON integer value is negative, using default size: %zu", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    } else if (json_is_real(value)) {
        double val = json_real_value(value);
        if (val >= 0) {
            return (size_t)val;
        }
        log_this("Configuration", "JSON real value is negative, using default size: %zu", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    log_this("Configuration", "JSON value is not convertible to size_t, using default: %zu", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}

/*
 * Enhanced get_config_double function that properly handles environment variables and logs results
 */
static double get_config_double(json_t* value, double default_value) {
    if (!value) {
        log_this("Configuration", "Using default double value: %f", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                double result = default_value;
                
                if (json_is_real(env_value)) {
                    result = json_real_value(env_value);
                    log_this("Configuration", "Using environment variable as double: %f", LOG_LEVEL_DEBUG, result);
                } else if (json_is_integer(env_value)) {
                    result = (double)json_integer_value(env_value);
                    log_this("Configuration", "Converting integer environment variable to double: %f", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_boolean(env_value)) {
                    result = json_is_true(env_value) ? 1.0 : 0.0;
                    log_this("Configuration", "Converting boolean environment variable to double: %f", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_string(env_value)) {
                    // Try to parse as double
                    const char* env_str = json_string_value(env_value);
                    char* endptr;
                    double val = strtod(env_str, &endptr);
                    if (*endptr == '\0') {
                        result = val;
                        log_this("Configuration", "Converting string environment variable '%s' to double: %f", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid double, using default: %f", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else {
                    log_this("Configuration", "Environment variable not a double type, using default: %f", 
                             LOG_LEVEL_DEBUG, default_value);
                }
                
                json_decref(env_value);
                return result;
            }
            
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
                
                // Log at INFO level for better visibility and test compatibility
                log_this("Configuration", "Using default for %s: %f", LOG_LEVEL_INFO,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default double: %f", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to double
        const char* str = json_string_value(value);
        char* endptr;
        double val = strtod(str, &endptr);
        if (*endptr == '\0') {
            log_this("Configuration", "Converting string '%s' to double: %f", LOG_LEVEL_DEBUG, str, val);
            return val;
        }
        
        log_this("Configuration", "String '%s' is not a valid double, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_real(value)) {
        return json_real_value(value);
    } else if (json_is_integer(value)) {
        return (double)json_integer_value(value);
    } else if (json_is_boolean(value)) {
        return json_is_true(value) ? 1.0 : 0.0;
    }
    
    log_this("Configuration", "JSON value is not convertible to double, using default: %f", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}

int MAX_PRIORITY_LABEL_WIDTH = 9;
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;

// Global static configuration instance
static AppConfig *app_config = NULL;

const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "ALL"},
    {1, "INFO"},
    {2, "WARN"},
    {3, "DEBUG"},
    {4, "ERROR"},
    {5, "CRITICAL"},
    {6, "NONE"}
};

/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 */
const AppConfig* get_app_config(void) {
    return app_config;
}

/*
 * Determine executable location with robust error handling
 * 
 * Why use /proc/self/exe?
 * - Provides the true binary path even when called through symlinks
 * - Works regardless of current working directory
 * - Handles SUID/SGID binaries correctly
 * - Gives absolute path without assumptions
 * 
 * Error Handling Strategy:
 * 1. Use readlink() for atomic path resolution
 * 2. Ensure null-termination for safety
 * 3. Handle memory allocation failures gracefully
 * 4. Return NULL on any error for consistent error handling
 */
char* get_executable_path() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        log_this("Configuration", "Error reading /proc/self/exe", LOG_LEVEL_DEBUG);
        return NULL;
    }
    path[len] = '\0';
    char* result = strdup(path);
    if (!result) {
        log_this("Configuration", "Error allocating memory for executable path", LOG_LEVEL_DEBUG);
        return NULL;
    }
    return result;
}

/*
 * Get file size with proper error detection
 * 
 * Why use stat()?
 * - Avoids opening the file unnecessarily
 * - Works for special files (devices, pipes)
 * - More efficient than seeking
 * - Provides atomic size reading
 */
long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/*
 * Get file modification time in human-readable format
 * 
 * Why this format?
 * - ISO 8601-like timestamp for consistency
 * - Local time for admin readability
 * - Fixed width for log formatting
 * - Includes date and time for complete context
 * 
 * Memory Management:
 * - Allocates fixed size buffer for safety
 * - Caller owns returned string
 * - Returns NULL on any error
 */
char* get_file_modification_time(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return NULL;
    }

    struct tm* tm_info = localtime(&st.st_mtime);
    if (tm_info == NULL) {
        return NULL;
    }

    char* time_str = malloc(20);  // YYYY-MM-DD HH:MM:SS\0
    if (time_str == NULL) {
        return NULL;
    }

    if (strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        free(time_str);
        return NULL;
    }

    return time_str;
}


/*
 * Generate default configuration with secure baseline
 * 
 * Why these defaults?
 * 1. Security First
 *    - Conservative file permissions and paths
 *    - Secure WebSocket keys and protocols
 *    - Resource limits to prevent DoS
 *    - Separate ports for different services
 * 
 * 2. Zero Configuration
 *    - Works out of the box for basic setups
 *    - Reasonable defaults for most environments
 *    - Clear upgrade path from defaults
 * 
 * 3. Discovery Ready
 *    - Standard ports for easy finding
 *    - mDNS services pre-configured
 *    - Compatible with common tools
 * 
 * 4. Operational Safety
 *    - Temporary directories for uploads
 *    - Size limits on all inputs
 *    - Separate logging for each component
 *    - Graceful failure modes
 */
void create_default_config(const char* config_path) {
    json_t* root = json_object();

    // Server Name
    json_object_set_new(root, "ServerName", json_string("Philement/hydrogen"));
    
    // Payload Key (using environment variable by default)
    json_object_set_new(root, "PayloadKey", json_string("${env.PAYLOAD_KEY}"));

    // Log File
    json_object_set_new(root, "LogFile", json_string("/var/log/hydrogen.log"));

    // Web Configuration
    json_t* web = json_object();
    json_object_set_new(web, "Enabled", json_boolean(1));
    json_object_set_new(web, "EnableIPv6", json_boolean(0));  // Default to disabled like mDNS
    json_object_set_new(web, "Port", json_integer(5000));
    json_object_set_new(web, "WebRoot", json_string("/home/asimard/lithium"));
    json_object_set_new(web, "UploadPath", json_string("/api/upload"));
    json_object_set_new(web, "UploadDir", json_string("/tmp/hydrogen_uploads"));
    json_object_set_new(web, "MaxUploadSize", json_integer(2147483648));
    json_object_set_new(root, "WebServer", web);

    // Logging Configuration
    json_t* logging = json_object();
    
    // Define logging levels
    json_t* levels = json_array();
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        json_t* level = json_array();
        json_array_append_new(level, json_integer(DEFAULT_PRIORITY_LEVELS[i].value));
        json_array_append_new(level, json_string(DEFAULT_PRIORITY_LEVELS[i].label));
        json_array_append_new(levels, level);
    }
    json_object_set_new(logging, "Levels", levels);

    // Console logging configuration
    json_t* console = json_object();
    json_object_set_new(console, "Enabled", json_boolean(1));
    json_t* console_subsystems = json_object();
    json_object_set_new(console_subsystems, "ThreadMgmt", json_integer(LOG_LEVEL_WARN));
    json_object_set_new(console_subsystems, "Shutdown", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "mDNSServer", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "WebServer", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "WebSocket", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "PrintQueue", json_integer(LOG_LEVEL_WARN));
    json_object_set_new(console_subsystems, "LogQueueManager", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console, "Subsystems", console_subsystems);
    json_object_set_new(logging, "Console", console);

    json_object_set_new(root, "Logging", logging);

    // WebSocket Configuration
    json_t* websocket = json_object();
    json_object_set_new(websocket, "Enabled", json_boolean(1));
    json_object_set_new(websocket, "EnableIPv6", json_boolean(0));  // Default to disabled like mDNS
    json_object_set_new(websocket, "Port", json_integer(5001));
    json_object_set_new(websocket, "Key", json_string("default_key_change_me"));
    json_object_set_new(websocket, "Protocol", json_string("hydrogen-protocol"));
    json_object_set_new(root, "WebSocket", websocket);

    // mDNS Server Configuration
    json_t* mdns_server = json_object();
    json_object_set_new(mdns_server, "Enabled", json_boolean(1));
    json_object_set_new(mdns_server, "EnableIPv6", json_boolean(0));  // Default to disabled since dev system doesn't support IPv6
    json_object_set_new(mdns_server, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns_server, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns_server, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns_server, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns_server, "Version", json_string("0.1.0"));

    json_t* services = json_array();

    json_t* http_service = json_object();
    json_object_set_new(http_service, "Name", json_string("hydrogen"));
    json_object_set_new(http_service, "Type", json_string("_http._tcp.local"));
    json_object_set_new(http_service, "Port", json_integer(5000));
    json_object_set_new(http_service, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, http_service);

    json_t* octoprint_service = json_object();
    json_object_set_new(octoprint_service, "Name", json_string("hydrogen"));
    json_object_set_new(octoprint_service, "Type", json_string("_octoprint._tcp.local"));
    json_object_set_new(octoprint_service, "Port", json_integer(5000));
    json_object_set_new(octoprint_service, "TxtRecords", json_string("path=/api,version=1.1.0"));
    json_array_append_new(services, octoprint_service);

    json_t* websocket_service = json_object();
    json_object_set_new(websocket_service, "Name", json_string("Hydrogen"));
    json_object_set_new(websocket_service, "Type", json_string("_websocket._tcp.local"));
    json_object_set_new(websocket_service, "Port", json_integer(5001));
    json_object_set_new(websocket_service, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, websocket_service);

    json_object_set_new(mdns_server, "Services", services);
    json_object_set_new(root, "mDNSServer", mdns_server);

    // System Resources Configuration
    json_t* resources = json_object();
    
    json_t* queues = json_object();
    json_object_set_new(queues, "MaxQueueBlocks", json_integer(DEFAULT_MAX_QUEUE_BLOCKS));
    json_object_set_new(queues, "QueueHashSize", json_integer(DEFAULT_QUEUE_HASH_SIZE));
    json_object_set_new(queues, "DefaultQueueCapacity", json_integer(DEFAULT_QUEUE_CAPACITY));
    json_object_set_new(resources, "Queues", queues);

    json_t* buffers = json_object();
    json_object_set_new(buffers, "DefaultMessageBuffer", json_integer(DEFAULT_MESSAGE_BUFFER_SIZE));
    json_object_set_new(buffers, "MaxLogMessageSize", json_integer(DEFAULT_MAX_LOG_MESSAGE_SIZE));
    json_object_set_new(buffers, "LineBufferSize", json_integer(DEFAULT_LINE_BUFFER_SIZE));
    json_object_set_new(buffers, "PostProcessorBuffer", json_integer(DEFAULT_POST_PROCESSOR_BUFFER_SIZE));
    json_object_set_new(resources, "Buffers", buffers);
    
    json_object_set_new(root, "SystemResources", resources);

    // Network Configuration
    json_t* network = json_object();
    
    json_t* interfaces = json_object();
    json_object_set_new(interfaces, "MaxInterfaces", json_integer(DEFAULT_MAX_INTERFACES));
    json_object_set_new(interfaces, "MaxIPsPerInterface", json_integer(DEFAULT_MAX_IPS_PER_INTERFACE));
    json_object_set_new(interfaces, "MaxInterfaceNameLength", json_integer(DEFAULT_MAX_INTERFACE_NAME_LENGTH));
    json_object_set_new(interfaces, "MaxIPAddressLength", json_integer(DEFAULT_MAX_IP_ADDRESS_LENGTH));
    json_object_set_new(network, "Interfaces", interfaces);

    json_t* port_allocation = json_object();
    json_object_set_new(port_allocation, "StartPort", json_integer(5000));
    json_object_set_new(port_allocation, "EndPort", json_integer(65535));
    json_t* reserved_ports = json_array();
    json_array_append_new(reserved_ports, json_integer(22));
    json_array_append_new(reserved_ports, json_integer(80));
    json_array_append_new(reserved_ports, json_integer(443));
    json_object_set_new(port_allocation, "ReservedPorts", reserved_ports);
    json_object_set_new(network, "PortAllocation", port_allocation);
    
    json_object_set_new(root, "Network", network);

    // System Monitoring Configuration
    json_t* monitoring = json_object();
    
    json_t* intervals = json_object();
    json_object_set_new(intervals, "StatusUpdateMs", json_integer(DEFAULT_STATUS_UPDATE_MS));
    json_object_set_new(intervals, "ResourceCheckMs", json_integer(DEFAULT_RESOURCE_CHECK_MS));
    json_object_set_new(intervals, "MetricsUpdateMs", json_integer(DEFAULT_METRICS_UPDATE_MS));
    json_object_set_new(monitoring, "Intervals", intervals);

    json_t* thresholds = json_object();
    json_object_set_new(thresholds, "MemoryWarningPercent", json_integer(DEFAULT_MEMORY_WARNING_PERCENT));
    json_object_set_new(thresholds, "DiskSpaceWarningPercent", json_integer(DEFAULT_DISK_WARNING_PERCENT));
    json_object_set_new(thresholds, "LoadAverageWarning", json_real(DEFAULT_LOAD_WARNING));
    json_object_set_new(monitoring, "Thresholds", thresholds);
    
    json_object_set_new(root, "SystemMonitoring", monitoring);

    // Printer Motion Configuration
    json_t* motion = json_object();
    json_object_set_new(motion, "MaxLayers", json_integer(DEFAULT_MAX_LAYERS));
    json_object_set_new(motion, "Acceleration", json_real(DEFAULT_ACCELERATION));
    json_object_set_new(motion, "ZAcceleration", json_real(DEFAULT_Z_ACCELERATION));
    json_object_set_new(motion, "EAcceleration", json_real(DEFAULT_E_ACCELERATION));
    json_object_set_new(motion, "MaxSpeedXY", json_real(DEFAULT_MAX_SPEED_XY));
    json_object_set_new(motion, "MaxSpeedTravel", json_real(DEFAULT_MAX_SPEED_TRAVEL));
    json_object_set_new(motion, "MaxSpeedZ", json_real(DEFAULT_MAX_SPEED_Z));
    json_object_set_new(motion, "ZValuesChunk", json_integer(DEFAULT_Z_VALUES_CHUNK));
    json_object_set_new(root, "Motion", motion);

    // Print Queue Configuration
    json_t* print_queue = json_object();
    json_object_set_new(print_queue, "Enabled", json_boolean(1));
    
    json_t* queue_settings = json_object();
    json_object_set_new(queue_settings, "DefaultPriority", json_integer(1));
    json_object_set_new(queue_settings, "EmergencyPriority", json_integer(0));
    json_object_set_new(queue_settings, "MaintenancePriority", json_integer(2));
    json_object_set_new(queue_settings, "SystemPriority", json_integer(3));
    json_object_set_new(print_queue, "QueueSettings", queue_settings);

    json_t* queue_timeouts = json_object();
    json_object_set_new(queue_timeouts, "ShutdownWaitMs", json_integer(DEFAULT_SHUTDOWN_WAIT_MS));
    json_object_set_new(queue_timeouts, "JobProcessingTimeoutMs", json_integer(DEFAULT_JOB_PROCESSING_TIMEOUT_MS));
    json_object_set_new(print_queue, "Timeouts", queue_timeouts);

    json_t* queue_buffers = json_object();
    json_object_set_new(queue_buffers, "JobMessageSize", json_integer(256));
    json_object_set_new(queue_buffers, "StatusMessageSize", json_integer(256));
    json_object_set_new(print_queue, "Buffers", queue_buffers);
    
    json_object_set_new(root, "PrintQueue", print_queue);

    // OIDC Configuration
    json_t* oidc = json_object();
    json_object_set_new(oidc, "Enabled", json_boolean(1));
    json_object_set_new(oidc, "Issuer", json_string("https://hydrogen.example.com"));
    
    // OIDC Endpoints
    json_t* endpoints = json_object();
    json_object_set_new(endpoints, "Authorization", json_string("/oauth/authorize"));
    json_object_set_new(endpoints, "Token", json_string("/oauth/token"));
    json_object_set_new(endpoints, "Userinfo", json_string("/oauth/userinfo"));
    json_object_set_new(endpoints, "Jwks", json_string("/oauth/jwks"));
    json_object_set_new(endpoints, "Introspection", json_string("/oauth/introspect"));
    json_object_set_new(endpoints, "Revocation", json_string("/oauth/revoke"));
    json_object_set_new(endpoints, "Registration", json_string("/oauth/register"));
    json_object_set_new(oidc, "Endpoints", endpoints);
    
    // OIDC Key Management
    json_t* keys = json_object();
    json_object_set_new(keys, "RotationIntervalDays", json_integer(30));
    json_object_set_new(keys, "StoragePath", json_string("/var/lib/hydrogen/oidc/keys"));
    json_object_set_new(keys, "EncryptionEnabled", json_boolean(1));
    json_object_set_new(oidc, "Keys", keys);
    
    // OIDC Token Settings
    json_t* tokens = json_object();
    json_object_set_new(tokens, "AccessTokenLifetime", json_integer(3600));        // 1 hour
    json_object_set_new(tokens, "RefreshTokenLifetime", json_integer(86400 * 30)); // 30 days
    json_object_set_new(tokens, "IdTokenLifetime", json_integer(3600));            // 1 hour
    json_object_set_new(oidc, "Tokens", tokens);
    
    // OIDC Security Settings
    json_t* security = json_object();
    json_object_set_new(security, "RequirePkce", json_boolean(1));
    json_object_set_new(security, "AllowImplicitFlow", json_boolean(0));
    json_object_set_new(security, "AllowClientCredentials", json_boolean(1));
    json_object_set_new(security, "RequireConsent", json_boolean(1));
    json_object_set_new(oidc, "Security", security);
    
    json_object_set_new(root, "OIDC", oidc);
    
    // API Configuration
    json_t* api = json_object();
    json_object_set_new(api, "JWTSecret", json_string("hydrogen_api_secret_change_me"));
    json_object_set_new(root, "API", api);

    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        log_this("Configuration", "Error: Unable to create default config at %s", LOG_LEVEL_DEBUG, config_path);
    } else {
        log_this("Configuration", "Created default config at %s", LOG_LEVEL_INFO, config_path);
    }

    json_decref(root);
}

/*
 * Load and validate configuration with comprehensive error handling
 * 
 * Why this approach?
 * 1. Resilient Loading
 *    - Handles partial configurations
 *    - Validates all values before use
 *    - Falls back to defaults safely
 *    - Preserves existing values when possible
 * 
 * 2. Memory Safety
 *    - Staged allocation for partial success
 *    - Complete cleanup on any failure
 *    - Minimal data copying
 *    - Proper string duplication
 * 
 * 3. Security Checks
 *    - Type validation for all values
 *    - Range checking for numeric fields
 *    - Path validation and normalization
 *    - Port availability verification
 * 
 * 4. Operational Awareness
 *    - Environment-specific defaults
 *    - Detailed error logging
 *    - Clear indication of fallback use
 *    - Maintains configuration versioning
 */
AppConfig* load_config(const char* config_path) {
    // Free previous configuration if it exists
    if (app_config) {
        // TODO: Implement proper cleanup of AppConfig resources
        free(app_config);
    }
    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        // Provide detailed error information including line and column number
        log_this("Configuration", "Failed to load config file: %s (line %d, column %d)",
             LOG_LEVEL_ERROR, error.text, error.line, error.column);
    
        // Print a user-friendly error message to stderr for operators
        fprintf(stderr, "ERROR: Hydrogen configuration file has JSON syntax errors.\n");
        fprintf(stderr, "       %s at line %d, column %d\n", error.text, error.line, error.column);
        fprintf(stderr, "       Please fix the syntax error and try again.\n");
    
        // Exit gracefully instead of returning NULL which would cause a segfault
        exit(EXIT_FAILURE);
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Configuration", "Failed to allocate memory for config", LOG_LEVEL_DEBUG);
        json_decref(root);
        return NULL;
    }
    
    // Store the configuration in the global static variable
    app_config = config;

    // Store paths
    config->config_file = strdup(config_path);  // Store the config file path
    config->executable_path = get_executable_path();
    if (!config->executable_path) {
        log_this("Configuration", "Failed to get executable path, using default", LOG_LEVEL_INFO);
        config->executable_path = strdup("./hydrogen");
    }

    // Server Name
    json_t* server_name = json_object_get(root, "ServerName");
    config->server_name = get_config_string(server_name, DEFAULT_SERVER_NAME);
    // Use "(default)" format for fallback values to match test expectations
    if (json_is_string(server_name) && 
        strncmp(json_string_value(server_name), "${env.", 6) == 0 &&
        !getenv(json_string_value(server_name) + 6)) {
        log_this("Configuration", "ServerName: (default)", LOG_LEVEL_INFO);
    } else {
        log_this("Configuration", "ServerName: %s", LOG_LEVEL_INFO, config->server_name);
    }
    
    // Payload Key (for payload decryption)
    json_t* payload_key = json_object_get(root, "PayloadKey");
    config->payload_key = get_config_string(payload_key, "${env.PAYLOAD_KEY}");
    // Don't log the actual key value for security reasons
    if (config->payload_key && strncmp(config->payload_key, "${env.", 6) == 0) {
        const char* env_var = config->payload_key + 6;
        size_t env_var_len = strlen(env_var);
        if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
            // Extract the variable name
            char var_name[256] = {0};
            // Use a safer approach with explicit size check and manual null termination
            size_t copy_len = env_var_len - 1 < sizeof(var_name) - 1 ? env_var_len - 1 : sizeof(var_name) - 1;
            memcpy(var_name, env_var, copy_len);
            var_name[copy_len] = '\0';
            
            if (getenv(var_name)) {
                log_this("Configuration", "PayloadKey: Using value from %s", LOG_LEVEL_INFO, var_name);
            } else {
                log_this("Configuration", "PayloadKey: Environment variable %s not found", LOG_LEVEL_WARN, var_name);
            }
        }
    } else if (config->payload_key) {
        log_this("Configuration", "PayloadKey: Set from configuration", LOG_LEVEL_INFO);
    } else {
        log_this("Configuration", "PayloadKey: Not configured", LOG_LEVEL_WARN);
    }

    // Log File
    json_t* log_file = json_object_get(root, "LogFile");
    config->log_file_path = get_config_string(log_file, DEFAULT_LOG_FILE);
    log_this("Configuration", "LogFile: %s", LOG_LEVEL_INFO, config->log_file_path);

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = get_config_bool(enabled, 1);
        // Exact format match for test: "WebServer Enabled: true" or "WebServer Enabled: false"
        log_this("Configuration", "WebServer Enabled: %s", LOG_LEVEL_INFO, 
                 config->web.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);

        json_t* port = json_object_get(web, "Port");
        config->web.port = get_config_int(port, DEFAULT_WEB_PORT);
        // Exact format match for test: "WebServer Port: 9000"
        log_this("Configuration", "WebServer Port: %d", LOG_LEVEL_INFO, config->web.port);

        json_t* web_root = json_object_get(web, "WebRoot");
        config->web.web_root = get_config_string(web_root, "/var/www/html");

        json_t* upload_path = json_object_get(web, "UploadPath");
        config->web.upload_path = get_config_string(upload_path, DEFAULT_UPLOAD_PATH);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        config->web.upload_dir = get_config_string(upload_dir, DEFAULT_UPLOAD_DIR);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
        
        // Load API prefix with default of "/api"
        json_t* api_prefix = json_object_get(web, "ApiPrefix");
        config->web.api_prefix = get_config_string(api_prefix, "/api");
        log_this("Configuration", "API Prefix: %s", LOG_LEVEL_INFO, config->web.api_prefix);
    } else {
    // Use defaults if web section is missing
    config->web.port = DEFAULT_WEB_PORT;
    config->web.web_root = strdup("/var/www/html");
    config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
    config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
    config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
    config->web.api_prefix = strdup("/api");  // Default API prefix
    log_this("Configuration", "API Prefix: %s (default)", LOG_LEVEL_INFO, config->web.api_prefix);
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        json_t* enabled = json_object_get(websocket, "Enabled");
        config->websocket.enabled = get_config_bool(enabled, 1);

        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = get_config_bool(enable_ipv6, 0);

        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = get_config_int(port, DEFAULT_WEBSOCKET_PORT);

        json_t* key = json_object_get(websocket, "Key");
        config->websocket.key = get_config_string(key, "default_key");

        // Get protocol with fallback to legacy uppercase key
        json_t* protocol = json_object_get(websocket, "protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "Protocol");  // Try legacy uppercase key
        }
        config->websocket.protocol = get_config_string(protocol, "hydrogen-protocol");
        if (!config->websocket.protocol) {
            log_this("Configuration", "Failed to allocate WebSocket protocol string", LOG_LEVEL_ERROR);
            // Clean up previously allocated strings
            free(config->websocket.key);
            return NULL;
        }

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, 10 * 1024 * 1024);  // Default to 10 MB if not specified

        // Process ConnectionTimeouts section
        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            // Process ExitWaitSeconds
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, 10);  // Default to 10 seconds
            log_this("Configuration", "WebSocket Exit Wait Seconds: %d", LOG_LEVEL_INFO, config->websocket.exit_wait_seconds);
        } else {
            // Default value if ConnectionTimeouts section is missing
            config->websocket.exit_wait_seconds = 10;
        }

    } else {
        // Use defaults if websocket section is missing
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;  // Default to 10 MB
        config->websocket.exit_wait_seconds = 10;  // Default to 10 seconds
    }

    // mDNS Server Configuration
    json_t* mdns_server = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns_server)) {
        json_t* enabled = json_object_get(mdns_server, "Enabled");
        config->mdns_server.enabled = get_config_bool(enabled, 1);

        json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
        config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);

        json_t* device_id = json_object_get(mdns_server, "DeviceId");
        config->mdns_server.device_id = get_config_string(device_id, "hydrogen-printer");

        json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
        config->mdns_server.friendly_name = get_config_string(friendly_name, "Hydrogen 3D Printer");

        json_t* model = json_object_get(mdns_server, "Model");
        config->mdns_server.model = get_config_string(model, "Hydrogen");

        json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
        config->mdns_server.manufacturer = get_config_string(manufacturer, "Philement");

        json_t* version = json_object_get(mdns_server, "Version");
        config->mdns_server.version = get_config_string(version, VERSION);

        json_t* services = json_object_get(mdns_server, "Services");
	if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));

            for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;

                json_t* name = json_object_get(service, "Name");
                config->mdns_server.services[i].name = get_config_string(name, "hydrogen");

                json_t* type = json_object_get(service, "Type");
                config->mdns_server.services[i].type = get_config_string(type, "_http._tcp.local");

                json_t* port = json_object_get(service, "Port");
                config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);
        
                // Handle TXT records
                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    // If TxtRecords is a single string, treat it as one record
                    config->mdns_server.services[i].num_txt_records = 1;
                    config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                    config->mdns_server.services[i].txt_records[0] = get_config_string(txt_records, "");
                } else if (json_is_array(txt_records)) {
                    // If TxtRecords is an array, handle multiple records
                    config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns_server.services[i].txt_records = malloc(config->mdns_server.services[i].num_txt_records * sizeof(char*));
                    for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                        json_t* record = json_array_get(txt_records, j);
                        config->mdns_server.services[i].txt_records[j] = get_config_string(record, "");
                    }
                } else {
                    // If TxtRecords is not present or invalid, set to NULL
                    config->mdns_server.services[i].num_txt_records = 0;
                    config->mdns_server.services[i].txt_records = NULL;
                }
            }
        }
    }

    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            json_t* val;
            val = json_object_get(queues, "MaxQueueBlocks");
            config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
        }
    } else {
        // Use defaults if section is missing
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
    }

    // Network Configuration
    json_t* network = json_object_get(root, "Network");
    if (json_is_object(network)) {
        json_t* interfaces = json_object_get(network, "Interfaces");
        if (json_is_object(interfaces)) {
            json_t* val;
            val = json_object_get(interfaces, "MaxInterfaces");
            config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
        }

        json_t* port_allocation = json_object_get(network, "PortAllocation");
        if (json_is_object(port_allocation)) {
            json_t* val;
            val = json_object_get(port_allocation, "StartPort");
            config->network.start_port = get_config_int(val, 5000);
            
            val = json_object_get(port_allocation, "EndPort");
            config->network.end_port = get_config_int(val, 65535);

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
        // Use defaults if section is missing
        config->network.max_interfaces = DEFAULT_MAX_INTERFACES;
        config->network.max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
        config->network.max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
        config->network.max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;
        config->network.start_port = 5000;
        config->network.end_port = 65535;
        config->network.reserved_ports = NULL;
        config->network.reserved_ports_count = 0;
    }

    // System Monitoring Configuration
    json_t* monitoring = json_object_get(root, "SystemMonitoring");
    if (json_is_object(monitoring)) {
        json_t* intervals = json_object_get(monitoring, "Intervals");
        if (json_is_object(intervals)) {
            json_t* val;
            val = json_object_get(intervals, "StatusUpdateMs");
            config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
            
            val = json_object_get(intervals, "ResourceCheckMs");
            config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
            
            val = json_object_get(intervals, "MetricsUpdateMs");
            config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
        }

        json_t* thresholds = json_object_get(monitoring, "Thresholds");
        if (json_is_object(thresholds)) {
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
            
            val = json_object_get(thresholds, "DiskSpaceWarningPercent");
            config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
            
            val = json_object_get(thresholds, "LoadAverageWarning");
            config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
        }
    } else {
        // Use defaults if section is missing
        config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
        config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
        config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
        config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
        config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
        config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
    }

    // Printer Motion Configuration
    json_t* motion = json_object_get(root, "Motion");
    if (json_is_object(motion)) {
        json_t* val;
        val = json_object_get(motion, "MaxLayers");
        config->motion.max_layers = get_config_size(val, DEFAULT_MAX_LAYERS);
        
        val = json_object_get(motion, "Acceleration");
        config->motion.acceleration = get_config_double(val, DEFAULT_ACCELERATION);
        
        val = json_object_get(motion, "ZAcceleration");
        config->motion.z_acceleration = get_config_double(val, DEFAULT_Z_ACCELERATION);
        
        val = json_object_get(motion, "EAcceleration");
        config->motion.e_acceleration = get_config_double(val, DEFAULT_E_ACCELERATION);
        
        val = json_object_get(motion, "MaxSpeedXY");
        config->motion.max_speed_xy = get_config_double(val, DEFAULT_MAX_SPEED_XY);
        
        val = json_object_get(motion, "MaxSpeedTravel");
        config->motion.max_speed_travel = get_config_double(val, DEFAULT_MAX_SPEED_TRAVEL);
        
        val = json_object_get(motion, "MaxSpeedZ");
        config->motion.max_speed_z = get_config_double(val, DEFAULT_MAX_SPEED_Z);
        
        val = json_object_get(motion, "ZValuesChunk");
        config->motion.z_values_chunk = get_config_size(val, DEFAULT_Z_VALUES_CHUNK);
    } else {
        // Use defaults if section is missing
        config->motion.max_layers = DEFAULT_MAX_LAYERS;
        config->motion.acceleration = DEFAULT_ACCELERATION;
        config->motion.z_acceleration = DEFAULT_Z_ACCELERATION;
        config->motion.e_acceleration = DEFAULT_E_ACCELERATION;
        config->motion.max_speed_xy = DEFAULT_MAX_SPEED_XY;
        config->motion.max_speed_travel = DEFAULT_MAX_SPEED_TRAVEL;
        config->motion.max_speed_z = DEFAULT_MAX_SPEED_Z;
        config->motion.z_values_chunk = DEFAULT_Z_VALUES_CHUNK;
    }

    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    if (json_is_object(print_queue)) {
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = get_config_bool(enabled, 1);
        // Exact format match for test: "PrintQueue Enabled: true" or "PrintQueue Enabled: false"
        log_this("Configuration", "PrintQueue Enabled: %s", LOG_LEVEL_INFO, 
                 config->print_queue.enabled ? "true" : "false");

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = get_config_int(val, 1);
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = get_config_int(val, 3);
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
            // Exact format match for test: "ShutdownWaitSeconds: 3"
            log_this("Configuration", "ShutdownWaitSeconds: %d", LOG_LEVEL_INFO, 
                    (int)(config->print_queue.timeouts.shutdown_wait_ms / 1000));
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = get_config_size(val, 256);
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = get_config_size(val, 256);
        }
    } else {
        // Use defaults if print queue section is missing
        config->print_queue.enabled = 1;
        config->print_queue.priorities.default_priority = 1;
        config->print_queue.priorities.emergency_priority = 0;
        config->print_queue.priorities.maintenance_priority = 2;
        config->print_queue.priorities.system_priority = 3;
        config->print_queue.timeouts.shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
        config->print_queue.timeouts.job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
        config->print_queue.buffers.job_message_size = 256;
        config->print_queue.buffers.status_message_size = 256;
    }

    // Load Logging Configuration
    json_t* logging = json_object_get(root, "Logging");
    if (json_is_object(logging)) {
        // Load logging levels
        json_t* levels = json_object_get(logging, "Levels");
        if (json_is_array(levels)) {
            config->Logging.LevelCount = json_array_size(levels);
            config->Logging.Levels = calloc(config->Logging.LevelCount, sizeof(struct { int value; const char *name; }));
            
            for (size_t i = 0; i < config->Logging.LevelCount; i++) {
                json_t* level = json_array_get(levels, i);
                if (json_is_array(level) && json_array_size(level) == 2) {
                    json_t* value = json_array_get(level, 0);
                    json_t* name = json_array_get(level, 1);
                    if (json_is_integer(value) && json_is_string(name)) {
                        config->Logging.Levels[i].value = json_integer_value(value);
                        config->Logging.Levels[i].name = strdup(json_string_value(name));
                    }
                }
            }
        }

        // Load console configuration
        json_t* console = json_object_get(logging, "Console");
        if (json_is_object(console)) {
            json_t* enabled = json_object_get(console, "Enabled");
            config->Logging.Console.Enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

            json_t* default_level = json_object_get(console, "DefaultLevel");
            config->Logging.Console.DefaultLevel = json_is_integer(default_level) ? json_integer_value(default_level) : LOG_LEVEL_INFO;

            json_t* subsystems = json_object_get(console, "Subsystems");
            if (json_is_object(subsystems)) {
                json_t* level;
                level = json_object_get(subsystems, "ThreadMgmt");
                config->Logging.Console.Subsystems.ThreadMgmt = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_WARN;
                level = json_object_get(subsystems, "Shutdown");
                config->Logging.Console.Subsystems.Shutdown = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "mDNSServer");
                config->Logging.Console.Subsystems.mDNSServer = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "WebServer");
                config->Logging.Console.Subsystems.WebServer = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "WebSocket");
                config->Logging.Console.Subsystems.WebSocket = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "PrintQueue");
                config->Logging.Console.Subsystems.PrintQueue = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_WARN;
                level = json_object_get(subsystems, "LogQueueManager");
                config->Logging.Console.Subsystems.LogQueueManager = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
            }
        }
    }

    // OIDC Configuration
    json_t* oidc = json_object_get(root, "OIDC");
    if (json_is_object(oidc)) {
        // Basic settings
        json_t* enabled = json_object_get(oidc, "Enabled");
        config->oidc.enabled = get_config_bool(enabled, 1);
        
        json_t* issuer = json_object_get(oidc, "Issuer");
        config->oidc.issuer = get_config_string(issuer, "https://hydrogen.example.com");
        
        // Endpoints
        json_t* endpoints = json_object_get(oidc, "Endpoints");
        if (json_is_object(endpoints)) {
            json_t* val;
            
            val = json_object_get(endpoints, "Authorization");
            config->oidc.endpoints.authorization = get_config_string(val, "/oauth/authorize");
            
            val = json_object_get(endpoints, "Token");
            config->oidc.endpoints.token = get_config_string(val, "/oauth/token");
            
            val = json_object_get(endpoints, "Userinfo");
            config->oidc.endpoints.userinfo = get_config_string(val, "/oauth/userinfo");
            
            val = json_object_get(endpoints, "Jwks");
            config->oidc.endpoints.jwks = get_config_string(val, "/oauth/jwks");
            
            val = json_object_get(endpoints, "Introspection");
            config->oidc.endpoints.introspection = get_config_string(val, "/oauth/introspect");
            
            val = json_object_get(endpoints, "Revocation");
            config->oidc.endpoints.revocation = get_config_string(val, "/oauth/revoke");
            
            val = json_object_get(endpoints, "Registration");
            config->oidc.endpoints.registration = get_config_string(val, "/oauth/register");
        } else {
            // Default endpoints if not specified
            config->oidc.endpoints.authorization = strdup("/oauth/authorize");
            config->oidc.endpoints.token = strdup("/oauth/token");
            config->oidc.endpoints.userinfo = strdup("/oauth/userinfo");
            config->oidc.endpoints.jwks = strdup("/oauth/jwks");
            config->oidc.endpoints.introspection = strdup("/oauth/introspect");
            config->oidc.endpoints.revocation = strdup("/oauth/revoke");
            config->oidc.endpoints.registration = strdup("/oauth/register");
        }
        
        // Key Management
        json_t* keys = json_object_get(oidc, "Keys");
        if (json_is_object(keys)) {
            json_t* val;
            
            val = json_object_get(keys, "RotationIntervalDays");
            config->oidc.keys.rotation_interval_days = get_config_int(val, 30);
            
            val = json_object_get(keys, "StoragePath");
            config->oidc.keys.storage_path = get_config_string(val, "/var/lib/hydrogen/oidc/keys");
            
            val = json_object_get(keys, "EncryptionEnabled");
            config->oidc.keys.encryption_enabled = get_config_bool(val, 1);
        } else {
            // Default key management settings if not specified
            config->oidc.keys.rotation_interval_days = 30;
            config->oidc.keys.storage_path = strdup("/var/lib/hydrogen/oidc/keys");
            config->oidc.keys.encryption_enabled = 1;
        }
        
        // Token Settings
        json_t* tokens = json_object_get(oidc, "Tokens");
        if (json_is_object(tokens)) {
            json_t* val;
            
            val = json_object_get(tokens, "AccessTokenLifetime");
            config->oidc.tokens.access_token_lifetime = get_config_int(val, 3600);
            
            val = json_object_get(tokens, "RefreshTokenLifetime");
            config->oidc.tokens.refresh_token_lifetime = get_config_int(val, 86400 * 30);
            
            val = json_object_get(tokens, "IdTokenLifetime");
            config->oidc.tokens.id_token_lifetime = get_config_int(val, 3600);
        } else {
            // Default token settings if not specified
            config->oidc.tokens.access_token_lifetime = 3600;        // 1 hour
            config->oidc.tokens.refresh_token_lifetime = 86400 * 30; // 30 days
            config->oidc.tokens.id_token_lifetime = 3600;            // 1 hour
        }
        
        // Security Settings
        json_t* security = json_object_get(oidc, "Security");
        if (json_is_object(security)) {
            json_t* val;
            
            val = json_object_get(security, "RequirePkce");
            config->oidc.security.require_pkce = get_config_bool(val, 1);
            
            val = json_object_get(security, "AllowImplicitFlow");
            config->oidc.security.allow_implicit_flow = get_config_bool(val, 0);
            
            val = json_object_get(security, "AllowClientCredentials");
            config->oidc.security.allow_client_credentials = get_config_bool(val, 1);
            
            val = json_object_get(security, "RequireConsent");
            config->oidc.security.require_consent = get_config_bool(val, 1);
        } else {
            // Default security settings if not specified
            config->oidc.security.require_pkce = 1;
            config->oidc.security.allow_implicit_flow = 0;
            config->oidc.security.allow_client_credentials = 1;
            config->oidc.security.require_consent = 1;
        }
    } else {
        // Use defaults if OIDC section is missing
        config->oidc.enabled = 1;
        config->oidc.issuer = strdup("https://hydrogen.example.com");
        
        // Default endpoints
        config->oidc.endpoints.authorization = strdup("/oauth/authorize");
        config->oidc.endpoints.token = strdup("/oauth/token");
        config->oidc.endpoints.userinfo = strdup("/oauth/userinfo");
        config->oidc.endpoints.jwks = strdup("/oauth/jwks");
        config->oidc.endpoints.introspection = strdup("/oauth/introspect");
        config->oidc.endpoints.revocation = strdup("/oauth/revoke");
        config->oidc.endpoints.registration = strdup("/oauth/register");
        
        // Default key management
        config->oidc.keys.rotation_interval_days = 30;
        config->oidc.keys.storage_path = strdup("/var/lib/hydrogen/oidc/keys");
        config->oidc.keys.encryption_enabled = 1;
        
        // Default tokens
        config->oidc.tokens.access_token_lifetime = 3600;        // 1 hour
        config->oidc.tokens.refresh_token_lifetime = 86400 * 30; // 30 days
        config->oidc.tokens.id_token_lifetime = 3600;            // 1 hour
        
        // Default security
        config->oidc.security.require_pkce = 1;
        config->oidc.security.allow_implicit_flow = 0;
        config->oidc.security.allow_client_credentials = 1;
        config->oidc.security.require_consent = 1;
        
        log_this("Configuration", "Using default OIDC configuration", LOG_LEVEL_INFO);
    }
    
    // Swagger Configuration (top-level)
    json_t* swagger = json_object_get(root, "Swagger");
    if (json_is_object(swagger)) {
        json_t* enabled = json_object_get(swagger, "Enabled");
        config->web.swagger.enabled = get_config_bool(enabled, 1);
        
        json_t* prefix = json_object_get(swagger, "Prefix");
        config->web.swagger.prefix = get_config_string(prefix, "/docs");

        // Load metadata configuration
        json_t* metadata = json_object_get(swagger, "Metadata");
        if (json_is_object(metadata)) {
            json_t* val;
            
            val = json_object_get(metadata, "Title");
            config->web.swagger.metadata.title = get_config_string(val, "Hydrogen REST API");
            
            val = json_object_get(metadata, "Description");
            config->web.swagger.metadata.description = get_config_string(val, "REST API for the Hydrogen Project");
            
            val = json_object_get(metadata, "Version");
            config->web.swagger.metadata.version = get_config_string(val, VERSION);

            // Contact information
            json_t* contact = json_object_get(metadata, "Contact");
            if (json_is_object(contact)) {
                val = json_object_get(contact, "Name");
                config->web.swagger.metadata.contact.name = get_config_string(val, "Philement Support");
                
                val = json_object_get(contact, "Email");
                config->web.swagger.metadata.contact.email = get_config_string(val, "api@example.com");
                
                val = json_object_get(contact, "Url");
                config->web.swagger.metadata.contact.url = get_config_string(val, "https://philement.com/support");
            } else {
                config->web.swagger.metadata.contact.name = strdup("Philement Support");
                config->web.swagger.metadata.contact.email = strdup("api@example.com");
                config->web.swagger.metadata.contact.url = strdup("https://philement.com/support");
            }

            // License information
            json_t* license = json_object_get(metadata, "License");
            if (json_is_object(license)) {
                val = json_object_get(license, "Name");
                config->web.swagger.metadata.license.name = get_config_string(val, "MIT");
                
                val = json_object_get(license, "Url");
                config->web.swagger.metadata.license.url = get_config_string(val, "https://opensource.org/licenses/MIT");
            } else {
                config->web.swagger.metadata.license.name = strdup("MIT");
                config->web.swagger.metadata.license.url = strdup("https://opensource.org/licenses/MIT");
            }
        } else {
            // Default metadata
            config->web.swagger.metadata.title = strdup("Hydrogen REST API");
            config->web.swagger.metadata.description = strdup("REST API for the Hydrogen Project");
            config->web.swagger.metadata.version = strdup(VERSION);
            config->web.swagger.metadata.contact.name = strdup("Philement Support");
            config->web.swagger.metadata.contact.email = strdup("api@example.com");
            config->web.swagger.metadata.contact.url = strdup("https://philement.com/support");
            config->web.swagger.metadata.license.name = strdup("MIT");
            config->web.swagger.metadata.license.url = strdup("https://opensource.org/licenses/MIT");
        }

        // Load UI options
        json_t* ui_options = json_object_get(swagger, "UIOptions");
        if (json_is_object(ui_options)) {
            json_t* val;
            
            val = json_object_get(ui_options, "TryItEnabled");
            config->web.swagger.ui_options.try_it_enabled = get_config_bool(val, 1);
            
            val = json_object_get(ui_options, "AlwaysExpanded");
            config->web.swagger.ui_options.always_expanded = get_config_bool(val, 1);
            
            val = json_object_get(ui_options, "DisplayOperationId");
            config->web.swagger.ui_options.display_operation_id = get_config_bool(val, 1);
            
            val = json_object_get(ui_options, "DefaultModelsExpandDepth");
            config->web.swagger.ui_options.default_models_expand_depth = get_config_int(val, 1);
            
            val = json_object_get(ui_options, "DefaultModelExpandDepth");
            config->web.swagger.ui_options.default_model_expand_depth = get_config_int(val, 1);
            
            val = json_object_get(ui_options, "ShowExtensions");
            config->web.swagger.ui_options.show_extensions = get_config_bool(val, 0);
            
            val = json_object_get(ui_options, "ShowCommonExtensions");
            config->web.swagger.ui_options.show_common_extensions = get_config_bool(val, 1);
            
            val = json_object_get(ui_options, "DocExpansion");
            config->web.swagger.ui_options.doc_expansion = get_config_string(val, "list");
            
            val = json_object_get(ui_options, "SyntaxHighlightTheme");
            config->web.swagger.ui_options.syntax_highlight_theme = get_config_string(val, "agate");
        } else {
            // Default UI options
            config->web.swagger.ui_options.try_it_enabled = 1;
            config->web.swagger.ui_options.always_expanded = 1;
            config->web.swagger.ui_options.display_operation_id = 1;
            config->web.swagger.ui_options.default_models_expand_depth = 1;
            config->web.swagger.ui_options.default_model_expand_depth = 1;
            config->web.swagger.ui_options.show_extensions = 0;
            config->web.swagger.ui_options.show_common_extensions = 1;
            config->web.swagger.ui_options.doc_expansion = strdup("list");
            config->web.swagger.ui_options.syntax_highlight_theme = strdup("agate");
        }
        
        log_this("Configuration", "Swagger UI: %s (prefix: %s)", LOG_LEVEL_INFO,
                config->web.swagger.enabled ? "enabled" : "disabled",
                config->web.swagger.prefix);
        log_this("Configuration", "Swagger Metadata: title='%s', version='%s'", LOG_LEVEL_INFO,
                config->web.swagger.metadata.title,
                config->web.swagger.metadata.version);
    } else {
        // Default Swagger configuration
        config->web.swagger.enabled = true;
        config->web.swagger.prefix = strdup("/docs");
        
        // Default metadata
        config->web.swagger.metadata.title = strdup("Hydrogen REST API");
        config->web.swagger.metadata.description = strdup("REST API for the Hydrogen Project");
        config->web.swagger.metadata.version = strdup(VERSION);
        config->web.swagger.metadata.contact.name = strdup("Philement Support");
        config->web.swagger.metadata.contact.email = strdup("api@example.com");
        config->web.swagger.metadata.contact.url = strdup("https://philement.com/support");
        config->web.swagger.metadata.license.name = strdup("MIT");
        config->web.swagger.metadata.license.url = strdup("https://opensource.org/licenses/MIT");
        
        // Default UI options
        config->web.swagger.ui_options.try_it_enabled = 1;
        config->web.swagger.ui_options.always_expanded = 1;
        config->web.swagger.ui_options.display_operation_id = 1;
        config->web.swagger.ui_options.default_models_expand_depth = 1;
        config->web.swagger.ui_options.default_model_expand_depth = 1;
        config->web.swagger.ui_options.show_extensions = 0;
        config->web.swagger.ui_options.show_common_extensions = 1;
        config->web.swagger.ui_options.doc_expansion = strdup("list");
        config->web.swagger.ui_options.syntax_highlight_theme = strdup("agate");
        
        log_this("Configuration", "Using default Swagger configuration", LOG_LEVEL_INFO);
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        config->api.jwt_secret = get_config_string(jwt_secret, "hydrogen_api_secret_change_me");
    } else {
        // Use defaults if API section is missing
        config->api.jwt_secret = strdup("hydrogen_api_secret_change_me");
        log_this("Configuration", "Using default API configuration", LOG_LEVEL_INFO);
    }

    json_decref(root);
    return config;
}



/*
 * Calculate maximum width of priority labels
 * 
 * Why pre-calculate?
 * - Ensures consistent log formatting
 * - Avoids repeated calculations
 * - Supports dynamic priority systems
 * - Maintains log readability
 */
void calculate_max_priority_label_width() {
    MAX_PRIORITY_LABEL_WIDTH = 0;
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        int label_width = strlen(DEFAULT_PRIORITY_LEVELS[i].label);
        if (label_width > MAX_PRIORITY_LABEL_WIDTH) {
            MAX_PRIORITY_LABEL_WIDTH = label_width;
        }
    }
}
