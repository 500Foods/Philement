/*
 * OpenID Connect (OIDC) Configuration
 *
 * Defines the configuration structure and defaults for OIDC integration.
 * This includes settings for identity providers, client credentials,
 * and endpoint configurations.
 */

#ifndef CONFIG_OIDC_H
#define CONFIG_OIDC_H

#include <stdbool.h>
#include "../config_forward.h"

// Default values
#define DEFAULT_OIDC_ENABLED true
#define DEFAULT_OIDC_PORT 8443
#define DEFAULT_TOKEN_EXPIRY 3600        // 1 hour in seconds
#define DEFAULT_REFRESH_EXPIRY 86400     // 24 hours in seconds
#define DEFAULT_AUTH_METHOD "client_secret_basic"

// OIDC endpoints configuration
typedef struct OIDCEndpointsConfig {
    char* authorization;     // Authorization endpoint
    char* token;            // Token endpoint
    char* userinfo;         // UserInfo endpoint
    char* jwks;             // JSON Web Key Set endpoint
    char* end_session;      // End session endpoint
    char* introspection;    // Token introspection endpoint
    char* revocation;       // Token revocation endpoint
    char* registration;     // Dynamic client registration endpoint
} OIDCEndpointsConfig;

// OIDC keys configuration
typedef struct OIDCKeysConfig {
    char* signing_key;      // Key for signing tokens
    char* encryption_key;   // Key for encryption
    char* jwks_uri;         // JSON Web Key Set URI
    char* storage_path;     // Path to key storage
    bool encryption_enabled; // Whether encryption is enabled
    int rotation_interval_days; // Key rotation interval
} OIDCKeysConfig;

// OIDC tokens configuration
typedef struct OIDCTokensConfig {
    int access_token_lifetime;  // Access token lifetime (seconds)
    int refresh_token_lifetime; // Refresh token lifetime (seconds)
    int id_token_lifetime;      // ID token lifetime (seconds)
    char* signing_alg;          // Token signing algorithm
    char* encryption_alg;       // Token encryption algorithm
} OIDCTokensConfig;

// Main OIDC configuration structure
struct OIDCConfig {
    bool enabled;                // Whether OIDC is enabled
    char* issuer;               // Identity provider URL
    char* client_id;            // Client identifier
    char* client_secret;        // Client secret
    char* redirect_uri;         // Redirect URI for auth code flow
    int port;                   // Port for OIDC endpoints
    char* auth_method;          // Token endpoint auth method
    char* scope;                // Default scope for requests
    bool verify_ssl;            // Whether to verify SSL certificates
    
    // Sub-configurations
    OIDCEndpointsConfig endpoints;  // Endpoint configurations
    OIDCKeysConfig keys;            // Key configurations
    OIDCTokensConfig tokens;        // Token configurations
};

/*
 * Initialize OIDC configuration with default values
 *
 * This function initializes a new OIDCConfig structure with secure
 * defaults suitable for most deployments.
 *
 * @param config Pointer to OIDCConfig structure to initialize
 * @return 0 on success, -1 on failure
 */
int config_oidc_init(OIDCConfig* config);

/*
 * Free resources allocated for OIDC configuration
 *
 * This function cleans up any resources allocated during initialization
 * or configuration loading.
 *
 * @param config Pointer to OIDCConfig structure to cleanup
 */
void config_oidc_cleanup(OIDCConfig* config);

/*
 * Validate OIDC configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies required fields are present
 * - Validates URLs and endpoints
 * - Checks token expiry times
 * - Ensures auth method is supported
 *
 * @param config Pointer to OIDCConfig structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_oidc_validate(const OIDCConfig* config);

#endif /* CONFIG_OIDC_H */