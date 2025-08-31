/*
 * API Configuration
 *
 * Defines the configuration structure and handlers for the API subsystem.
 * Includes settings for API endpoints, routing, and JWT authentication.
 */

#ifndef HYDROGEN_CONFIG_API_H
#define HYDROGEN_CONFIG_API_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// API configuration structure
typedef struct APIConfig {
    bool enabled;         // Whether API endpoints are enabled
    char* prefix;         // API URL prefix (e.g., "/api")
    char* jwt_secret;     // Secret key for JWT token signing
    char* cors_origin;    // NEW: CORS origin for API endpoints
} APIConfig;

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
 * Clean up API configuration
 *
 * This function frees all resources allocated during configuration loading.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to APIConfig structure to cleanup
 */
void cleanup_api_config(APIConfig* config);

/*
 * Dump API configuration for debugging
 *
 * This function outputs the current API configuration settings
 * using the standard logging system.
 *
 * @param config Pointer to APIConfig structure to dump
 */
void dump_api_config(const APIConfig* config);

#endif /* HYDROGEN_CONFIG_API_H */
