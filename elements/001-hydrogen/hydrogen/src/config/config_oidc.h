/*
 * OpenID Connect Configuration
 *
 * Defines the main configuration structure for the OIDC subsystem.
 * This coordinates all OIDC-related configuration components.
 */

#ifndef HYDROGEN_CONFIG_OIDC_H
#define HYDROGEN_CONFIG_OIDC_H

// Project headers
#include "config_forward.h"
#include "config_oidc_endpoints.h"
#include "config_oidc_keys.h"
#include "config_oidc_tokens.h"
#include "config_oidc_security.h"

// Default values
#define DEFAULT_OIDC_ENABLED 0
#define DEFAULT_OIDC_ISSUER "http://localhost:5000"

// OIDC configuration structure
struct OIDCConfig {
    int enabled;                     // Whether OIDC is enabled
    char* issuer;                    // OIDC issuer URL
    
    // Subsystem configurations
    OIDCEndpointsConfig endpoints;   // Endpoint URLs
    OIDCKeysConfig keys;            // Key management settings
    OIDCTokensConfig tokens;        // Token lifetime settings
    OIDCSecurityConfig security;    // Security-related settings
};

/*
 * Initialize OIDC configuration with default values
 *
 * This function initializes a new OIDCConfig structure and all its
 * subsystem configurations with secure default values.
 *
 * @param config Pointer to OIDCConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails
 * - If any subsystem initialization fails
 */
int config_oidc_init(OIDCConfig* config);

/*
 * Free resources allocated for OIDC configuration
 *
 * This function cleans up the main OIDC configuration and all its
 * subsystem configurations. It safely handles NULL pointers and
 * partial initialization.
 *
 * @param config Pointer to OIDCConfig structure to cleanup
 */
void config_oidc_cleanup(OIDCConfig* config);

/*
 * Validate OIDC configuration values
 *
 * This function performs comprehensive validation of all OIDC settings:
 * - Validates core OIDC settings (enabled flag, issuer URL)
 * - Validates all subsystem configurations
 * - Ensures consistency between subsystem settings
 *
 * @param config Pointer to OIDCConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required settings are missing
 * - If any subsystem validation fails
 * - If settings are inconsistent across subsystems
 */
int config_oidc_validate(const OIDCConfig* config);

#endif /* HYDROGEN_CONFIG_OIDC_H */