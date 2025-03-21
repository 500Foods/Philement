/*
 * Environment variable handling for configuration system - Adapter File
 * 
 * This file acts as an adapter to forward calls to the implementation
 * in the env/ subdirectory. This maintains backward compatibility while
 * allowing us to modularize the code.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#include "config_env.h"
#include "logging/config_logging.h"

// Forward declaration of the actual implementation
extern json_t* env_process_env_variable(const char* value);

/*
 * Adapter function that forwards to the actual implementation in env/config_env.c
 * 
 * This maintains backward compatibility with existing code that calls
 * process_env_variable() directly.
 */
json_t* process_env_variable(const char* value) {
    // Simply forward to the implementation in env/config_env.c
    return env_process_env_variable(value);
}