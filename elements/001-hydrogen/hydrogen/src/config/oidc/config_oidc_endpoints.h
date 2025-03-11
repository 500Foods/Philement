/*
 * OpenID Connect Endpoints Configuration
 *
 * Defines the configuration structure and defaults for OIDC endpoint URLs.
 * This includes all standard OIDC endpoints required for the protocol.
 */

#ifndef HYDROGEN_CONFIG_OIDC_ENDPOINTS_H
#define HYDROGEN_CONFIG_OIDC_ENDPOINTS_H

// Project headers
#include "../config_forward.h"

// Default endpoint paths (relative to issuer URL)
#define DEFAULT_OIDC_AUTH_PATH "/authorize"
#define DEFAULT_OIDC_TOKEN_PATH "/token"
#define DEFAULT_OIDC_USERINFO_PATH "/userinfo"
#define DEFAULT_OIDC_JWKS_PATH "/jwks"
#define DEFAULT_OIDC_INTROSPECTION_PATH "/introspect"
#define DEFAULT_OIDC_REVOCATION_PATH "/revoke"
#define DEFAULT_OIDC_REGISTRATION_PATH "/register"

// OIDC endpoints configuration structure
struct OIDCEndpointsConfig {
    char* authorization;    // Authorization endpoint URL
    char* token;           // Token endpoint URL
    char* userinfo;        // UserInfo endpoint URL
    char* jwks;            // JWKS (JSON Web Key Set) endpoint URL
    char* introspection;   // Token introspection endpoint URL
    char* revocation;      // Token revocation endpoint URL
    char* registration;    // Dynamic client registration endpoint URL
};

/*
 * Initialize OIDC endpoints configuration with default values
 *
 * This function initializes a new OIDCEndpointsConfig structure with default
 * endpoint paths. The actual URLs will be constructed by combining these paths
 * with the issuer URL during runtime.
 *
 * @param config Pointer to OIDCEndpointsConfig structure to initialize
 * @return 0 on success, -1 on failure (memory allocation or null pointer)
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any endpoint URL
 */
int config_oidc_endpoints_init(OIDCEndpointsConfig* config);

/*
 * Free resources allocated for OIDC endpoints configuration
 *
 * This function frees all resources allocated by config_oidc_endpoints_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to OIDCEndpointsConfig structure to cleanup
 */
void config_oidc_endpoints_cleanup(OIDCEndpointsConfig* config);

/*
 * Validate OIDC endpoints configuration values
 *
 * This function performs comprehensive validation of the endpoints:
 * - Verifies all required endpoint URLs are present
 * - Validates URL format for each endpoint
 * - Ensures endpoints are properly configured for enabled features
 *
 * @param config Pointer to OIDCEndpointsConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any required endpoint URL is missing or invalid
 * - If endpoint URLs are inconsistent with the issuer URL
 */
int config_oidc_endpoints_validate(const OIDCEndpointsConfig* config);

#endif /* HYDROGEN_CONFIG_OIDC_ENDPOINTS_H */