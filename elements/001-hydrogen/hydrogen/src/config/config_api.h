/*
 * API Configuration
 *
 * Defines the configuration structure and defaults for the API subsystem.
 * This includes settings for authentication and security.
 */

#ifndef HYDROGEN_CONFIG_API_H
#define HYDROGEN_CONFIG_API_H

// Project headers
#include "config_forward.h"

// Default values
#define DEFAULT_JWT_SECRET "hydrogen_api_secret_change_me"

// API configuration structure
struct APIConfig {
    char* jwt_secret;         // Secret for JWT token signing
};

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
 * - If memory allocation fails for jwt_secret
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
 * This function performs validation of the configuration:
 * - Verifies jwt_secret is set and not empty
 * - Checks for minimum secret length
 *
 * @param config Pointer to APIConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If jwt_secret is NULL or empty
 * - If jwt_secret is too short
 */
int config_api_validate(const APIConfig* config);

#endif /* HYDROGEN_CONFIG_API_H */