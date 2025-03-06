/*
 * Environment variable handling for configuration system
 * 
 * This module handles the resolution and type conversion of environment
 * variables in configuration values. It supports the ${env.VARIABLE}
 * syntax and provides type-safe conversion to various JSON types.
 */

#ifndef CONFIGURATION_ENV_H
#define CONFIGURATION_ENV_H

#include <jansson.h>

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
json_t* process_env_variable(const char* value);

#endif /* CONFIGURATION_ENV_H */