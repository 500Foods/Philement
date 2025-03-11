/*
 * Size_t configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to size_t, with proper validation and logging.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>

// Project headers
#include "config_size.h"
#include "../config_env.h"
#include "../../logging/logging.h"

/*
 * Parse a size value with optional unit suffix
 * Returns the parsed value in bytes
 */
static size_t parse_size_with_unit(const char* str_value) {
    if (!str_value || str_value[0] == '-') return 0;
    
    char* endptr;
    errno = 0;
    double val = strtod(str_value, &endptr);
    
    if (errno != 0 || val < 0) {
        return 0;
    }

    // Skip whitespace after number
    while (isspace(*endptr)) endptr++;
    
    // No unit suffix, return raw value
    if (*endptr == '\0') {
        return val <= SIZE_MAX ? (size_t)val : 0;
    }

    // Handle size units
    if (strcasecmp(endptr, "b") == 0) {
        return val <= SIZE_MAX ? (size_t)val : 0;
    }
    if (strcasecmp(endptr, "k") == 0 ||
        strcasecmp(endptr, "kb") == 0) {
        val *= 1024.0;
    }
    else if (strcasecmp(endptr, "m") == 0 ||
             strcasecmp(endptr, "mb") == 0) {
        val *= 1024.0 * 1024.0;
    }
    else if (strcasecmp(endptr, "g") == 0 ||
             strcasecmp(endptr, "gb") == 0) {
        val *= 1024.0 * 1024.0 * 1024.0;
    }
    
    return val <= SIZE_MAX ? (size_t)val : 0;
}

size_t get_config_size(json_t* value, size_t default_value) {
    if (!value) {
        return default_value;
    }

    // Handle direct integer values
    if (json_is_integer(value)) {
        json_int_t val = json_integer_value(value);
        return (val >= 0) ? (size_t)val : default_value;
    }

    // Handle real numbers - just check for non-negative
    if (json_is_real(value)) {
        double val = json_real_value(value);
        return val >= 0 ? (size_t)val : default_value;
    }

    // Handle boolean values
    if (json_is_boolean(value)) {
        return json_is_true(value) ? 1 : 0;
    }

    // Handle string values (with units or env vars)
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check for environment variable
        json_t* env_value = process_env_variable(str_value);
        if (env_value) {
            size_t result = get_config_size(env_value, default_value);
            json_decref(env_value);
            return result;
        }

        // Parse string value with potential unit suffix
        size_t parsed = parse_size_with_unit(str_value);
        return parsed != 0 ? parsed : default_value;
    }

    return default_value;
}