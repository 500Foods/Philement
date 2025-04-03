/*
 * API Configuration Implementation
 *
 * Implements the configuration handlers for the API subsystem.
 */

#include <stdlib.h>
#include <string.h>
#include "config_api.h"
#include "../config_utils.h"
#include "../../logging/logging.h"

// Initialize API configuration with default values
int config_api_init(APIConfig* config) {
    if (!config) {
        log_this("Config", "API config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = DEFAULT_API_ENABLED;
    
    // Allocate and copy default prefix
    config->prefix = strdup(DEFAULT_API_PREFIX);
    if (!config->prefix) {
        log_this("Config", "Failed to allocate API prefix", LOG_LEVEL_ERROR);
        return -1;
    }

    // Allocate and copy default JWT secret
    config->jwt_secret = strdup(DEFAULT_API_JWT_SECRET);
    if (!config->jwt_secret) {
        log_this("Config", "Failed to allocate API JWT secret", LOG_LEVEL_ERROR);
        free(config->prefix);
        config->prefix = NULL;
        return -1;
    }

    return 0;
}

// Free resources allocated for API configuration
void config_api_cleanup(APIConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    if (config->prefix) {
        free(config->prefix);
        config->prefix = NULL;
    }

    if (config->jwt_secret) {
        free(config->jwt_secret);
        config->jwt_secret = NULL;
    }

    // Zero out the structure
    memset(config, 0, sizeof(APIConfig));
}

// Validate API configuration values
int config_api_validate(const APIConfig* config) {
    if (!config) {
        log_this("Config", "API config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate prefix
    if (!config->prefix || !config->prefix[0] || config->prefix[0] != '/') {
        log_this("Config", "Invalid API prefix (must start with /)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate JWT secret
    if (!config->jwt_secret || !config->jwt_secret[0]) {
        log_this("Config", "Invalid API JWT secret (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}