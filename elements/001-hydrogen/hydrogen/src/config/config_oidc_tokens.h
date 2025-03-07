/*
 * OpenID Connect Tokens Configuration
 *
 * Defines the configuration structure and defaults for OIDC token management.
 * This includes settings for token lifetimes and related parameters.
 */

#ifndef HYDROGEN_CONFIG_OIDC_TOKENS_H
#define HYDROGEN_CONFIG_OIDC_TOKENS_H

// Project headers
#include "config_forward.h"

// Default token lifetimes (in seconds)
#define DEFAULT_ACCESS_TOKEN_LIFETIME 3600     // 1 hour
#define DEFAULT_REFRESH_TOKEN_LIFETIME 2592000 // 30 days
#define DEFAULT_ID_TOKEN_LIFETIME 3600         // 1 hour

// Maximum token lifetimes (in seconds)
#define MAX_ACCESS_TOKEN_LIFETIME 86400      // 24 hours
#define MAX_REFRESH_TOKEN_LIFETIME 7776000   // 90 days
#define MAX_ID_TOKEN_LIFETIME 86400          // 24 hours

// OIDC tokens configuration structure
struct OIDCTokensConfig {
    int access_token_lifetime;   // Lifetime of access tokens in seconds
    int refresh_token_lifetime;  // Lifetime of refresh tokens in seconds
    int id_token_lifetime;      // Lifetime of ID tokens in seconds
};

/*
 * Initialize OIDC tokens configuration with default values
 *
 * This function initializes a new OIDCTokensConfig structure with secure
 * default values for token lifetimes. These defaults balance security
 * with usability.
 *
 * @param config Pointer to OIDCTokensConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_oidc_tokens_init(OIDCTokensConfig* config);

/*
 * Free resources allocated for OIDC tokens configuration
 *
 * This function cleans up any resources allocated by config_oidc_tokens_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to OIDCTokensConfig structure to cleanup
 */
void config_oidc_tokens_cleanup(OIDCTokensConfig* config);

/*
 * Validate OIDC tokens configuration values
 *
 * This function performs comprehensive validation of token settings:
 * - Verifies all token lifetimes are within acceptable ranges
 * - Ensures refresh token lifetime is longer than access token lifetime
 * - Validates relationships between different token types
 *
 * @param config Pointer to OIDCTokensConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any token lifetime is invalid (<=0 or exceeds maximum)
 * - If token lifetime relationships are invalid
 */
int config_oidc_tokens_validate(const OIDCTokensConfig* config);

#endif /* HYDROGEN_CONFIG_OIDC_TOKENS_H */