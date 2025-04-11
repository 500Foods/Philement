/*
 * API Configuration
 *
 * Defines the configuration structure and handlers for the API subsystem.
 * Includes settings for API endpoints, routing, and JSON parsing.
 */

#ifndef HYDROGEN_CONFIG_API_H
#define HYDROGEN_CONFIG_API_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// API configuration structure
struct APIConfig {
    bool enabled;         // Whether API endpoints are enabled
    char* prefix;         // API URL prefix (e.g., "/api")
    char* jwt_secret;     // Secret key for JWT token signing
};
typedef struct APIConfig APIConfig;

/*
 * Load API configuration from JSON
 *
 * This function loads the API configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_api_config(json_t* root, AppConfig* config);

/*
 * Initialize API configuration with default values
 *
 * This function initializes a new APIConfig structure with default
 * values that provide a secure baseline configuration.
 *
 * @param config Pointer to APIConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any string field
 */
int config_api_init(APIConfig* config);

/*
 * Free resources allocated for API configuration
 *
 * This function frees all resources allocated by config_api_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to APIConfig structure to cleanup
 */
void config_api_cleanup(APIConfig* config);

/*
 * Validate API configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies prefix is valid (must start with /)
 * - Checks enabled flag
 * - Validates JWT secret is set
 *
 * @param config Pointer to APIConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If prefix is invalid
 * - If JWT secret is not set
 */
int config_api_validate(const APIConfig* config);

#endif /* HYDROGEN_CONFIG_API_H */