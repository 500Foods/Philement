/*
 * Integer configuration value handler
 * 
 * This module contains routines for extracting integer values from configuration
 * objects with robust error handling and default value fallbacks.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <ctype.h>
#include <jansson.h>

#include "config_env.h"
#include "config_defaults.h"
#include "../logging/logging.h"

/*
 * Parse a numeric value with optional unit suffix
 * Returns the parsed value in the base unit (milliseconds for time, bytes for size)
 * Returns 0 on parsing failure
 */
static int parse_value_with_unit(const char* str_value) {
    if (!str_value) {
        return 0;
    }
    
    char* endptr;
    errno = 0;
    double val = strtod(str_value, &endptr);
    
    if (errno != 0 || val > INT_MAX || val < INT_MIN) {
        return 0;
    }

    // Skip whitespace after number
    while (isspace(*endptr)) endptr++;
    
    // No unit suffix, return raw value
    if (*endptr == '\0') {
        return (int)val;
    }

    // Handle time units
    if (strcasecmp(endptr, "ms") == 0 ||
        strcasecmp(endptr, "milliseconds") == 0) {
        return (int)val;
    }
    if (strcasecmp(endptr, "s") == 0 ||
        strcasecmp(endptr, "seconds") == 0) {
        val *= 1000.0;
    }
    else if (strcasecmp(endptr, "min") == 0 ||
             strcasecmp(endptr, "minutes") == 0) {
        val *= 60.0 * 1000.0;
    }
    
    if (val > INT_MAX || val < INT_MIN) {
        return 0;
    }
    
    return (int)val;
}

int get_config_int(json_t* value, int default_value) {
    if (!value) {
        return default_value;
    }

    // Handle direct integer values - take them as-is
    if (json_is_integer(value)) {
        return (int)json_integer_value(value);
    }

    // Handle real numbers - convert directly
    if (json_is_real(value)) {
        double real_val = json_real_value(value);
        if (real_val > INT_MAX || real_val < INT_MIN) {
            return default_value;
        }
        return (int)real_val;
    }

    // Handle boolean values
    if (json_is_boolean(value)) {
        return json_is_true(value) ? 1 : 0;
    }

    // Handle null values
    if (json_is_null(value)) {
        return default_value;
    }

    // Handle string values (with units or env vars)
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check for environment variable
        json_t* env_value = process_env_variable(str_value);
        if (env_value) {
            int result = get_config_int(env_value, default_value);
            json_decref(env_value);
            return result;
        }

        // Parse string value with potential unit suffix
        int parsed = parse_value_with_unit(str_value);
        if (parsed != 0) {
            return parsed;
        }
        
        return default_value;
    }

    // For any other types or unexpected cases, return the default
    return default_value;
}