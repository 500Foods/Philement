/*
 * OpenID Connect (OIDC) Configuration
 *
 * Defines the configuration structure and defaults for OIDC integration.
 * This includes settings for:
 * - Identity providers and client credentials
 * - Endpoint configurations and URLs
 * - Key management and security settings
 * - Token lifetimes and algorithms
 * 
 * All validation has been moved to launch readiness checks.
 *
 */

#ifndef CONFIG_OIDC_H
#define CONFIG_OIDC_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

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
    char* signing_alg;          // Token signing algorithm (e.g., RS256)
    char* encryption_alg;       // Token encryption algorithm (e.g., A256GCM)
} OIDCTokensConfig;

// Main OIDC configuration structure
typedef struct OIDCConfig {
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
} OIDCConfig;

/*
 * Helper function to construct endpoint URL
 *
 * @param base_path Base path for the endpoint
 * @return Allocated string with proper path format, or NULL on error
 */
char* construct_endpoint_path(const char* base_path);

/*
 * Load OIDC configuration from JSON
 *
 * This function loads the OIDC configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_oidc_config(json_t* root, AppConfig* config);

/*
 * Clean up OIDC configuration
 *
 * This function cleans up the OIDC configuration and all its
 * sub-configurations. It safely handles NULL pointers and partial
 * initialization.
 *
 * @param config Pointer to OIDCConfig structure to cleanup
 */
void cleanup_oidc_config(OIDCConfig* config);

/*
 * Dump OIDC configuration for debugging
 *
 * This function outputs the current state of the OIDC configuration
 * and all its sub-configurations in a structured format.
 *
 * @param config Pointer to OIDCConfig structure to dump
 */
void dump_oidc_config(const OIDCConfig* config);

#endif /* CONFIG_OIDC_H */
