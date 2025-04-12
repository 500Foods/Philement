/*
 * API Configuration Implementation
 *
 * Implements the configuration handlers for the API subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_api.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load API configuration from JSON
bool load_api_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->api = (APIConfig){
        .enabled = true,
        .prefix = "/api",
        .jwt_secret = "${env.JWT_SECRET}"
    };

    // Process all config items in sequence
    bool success = true;

    // One line per key/value using macros
    success = PROCESS_SECTION(root, "API");
    success = success && PROCESS_BOOL(root, &config->api, enabled, "API.Enabled", "API");
    success = success && PROCESS_STRING(root, &config->api, prefix, "API.Prefix", "API");
    success = success && PROCESS_SENSITIVE(root, &config->api, jwt_secret, "API.JWTSecret", "API");

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_api_validate(&config->api) == 0);
    }

    return success;
}

// Initialize API configuration with default values
int config_api_init(APIConfig* config) {
    if (!config) {
        log_this("Config-API", "API config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = true;
    
    // Allocate and copy default prefix
    config->prefix = strdup("/api");
    if (!config->prefix) {
        log_this("Config-API", "Failed to allocate API prefix", LOG_LEVEL_ERROR);
        return -1;
    }

    // Allocate and copy default JWT secret
    config->jwt_secret = strdup("${env.JWT_SECRET}");
    if (!config->jwt_secret) {
        log_this("Config-API", "Failed to allocate API JWT secret", LOG_LEVEL_ERROR);
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
        log_this("Config-API", "API config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate prefix
    if (!config->prefix || !config->prefix[0] || config->prefix[0] != '/') {
        log_this("Config-API", "Invalid API prefix (must start with /)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate JWT secret
    if (!config->jwt_secret || !config->jwt_secret[0]) {
        log_this("Config-API", "Invalid API JWT secret (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}