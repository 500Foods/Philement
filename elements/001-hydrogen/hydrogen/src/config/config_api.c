/*
 * API Configuration Implementation
 *
 * Implements the configuration handlers for the API subsystem,
 * including JSON parsing and environment variable handling.
 * Note: Validation moved to launch readiness checks
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_api.h"

// Load API configuration from JSON
bool load_api_config(json_t* root, AppConfig* config) {
    bool success = true;
    APIConfig* api_config = &config->api;

    // Initialize with defaults
    memset(api_config, 0, sizeof(APIConfig));
    api_config->enabled = true;
    api_config->prefix = strdup("/api");
    api_config->jwt_secret = strdup("${env.JWT_SECRET}");

    // Process all config items in sequence
    success = PROCESS_SECTION(root, "API");
    success = success && PROCESS_BOOL(root, api_config, enabled, "API.Enabled", "API");
    success = success && PROCESS_STRING(root, api_config, prefix, "API.Prefix", "API");
    success = success && PROCESS_SENSITIVE(root, api_config, jwt_secret, "API.JWTSecret", "API");

    // Clean up and return on failure
    if (!success) {
        cleanup_api_config(api_config);
        return false;
    }

    return success;
}

// Clean up API configuration
void cleanup_api_config(APIConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->prefix);
    free(config->jwt_secret);

    // Zero out the structure
    memset(config, 0, sizeof(APIConfig));
}

// Dump API configuration for debugging
void dump_api_config(const APIConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Cannot dump NULL API config", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Dump API settings
    DUMP_BOOL("―― Enabled", config->enabled);
    DUMP_STRING("―― Prefix", config->prefix);
    DUMP_SECRET("―― JWTSecret", config->jwt_secret);
}
