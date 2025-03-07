/*
 * OpenID Connect Security Configuration
 *
 * Defines the configuration structure and defaults for OIDC security settings.
 * This includes PKCE requirements, allowed flows, and consent settings.
 */

#ifndef HYDROGEN_CONFIG_OIDC_SECURITY_H
#define HYDROGEN_CONFIG_OIDC_SECURITY_H

// Project headers
#include "config_forward.h"

// Default security settings (secure by default)
#define DEFAULT_REQUIRE_PKCE 1              // Require PKCE for public clients
#define DEFAULT_ALLOW_IMPLICIT_FLOW 0       // Implicit flow disabled by default
#define DEFAULT_ALLOW_CLIENT_CREDENTIALS 1  // Allow client credentials flow
#define DEFAULT_REQUIRE_CONSENT 1           // Require user consent

// OIDC security configuration structure
struct OIDCSecurityConfig {
    int require_pkce;             // Whether to require PKCE for public clients
    int allow_implicit_flow;      // Whether to allow the implicit flow
    int allow_client_credentials; // Whether to allow client credentials flow
    int require_consent;          // Whether to require user consent
};

/*
 * Initialize OIDC security configuration with default values
 *
 * This function initializes a new OIDCSecurityConfig structure with secure
 * default values. These defaults prioritize security over convenience.
 *
 * @param config Pointer to OIDCSecurityConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_oidc_security_init(OIDCSecurityConfig* config);

/*
 * Free resources allocated for OIDC security configuration
 *
 * This function cleans up any resources allocated by config_oidc_security_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to OIDCSecurityConfig structure to cleanup
 */
void config_oidc_security_cleanup(OIDCSecurityConfig* config);

/*
 * Validate OIDC security configuration values
 *
 * This function performs comprehensive validation of security settings:
 * - Verifies all boolean flags have valid values (0 or 1)
 * - Validates combinations of security settings
 * - Ensures security requirements are met
 *
 * @param config Pointer to OIDCSecurityConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any flag has an invalid value
 * - If security settings create an invalid combination
 */
int config_oidc_security_validate(const OIDCSecurityConfig* config);

#endif /* HYDROGEN_CONFIG_OIDC_SECURITY_H */