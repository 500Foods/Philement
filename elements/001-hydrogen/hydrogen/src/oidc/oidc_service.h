/*
 * OpenID Connect (OIDC) Service
 *
 * Main header file for Hydrogen's OIDC implementation, providing interfaces
 * for authentication, authorization, and identity management using the
 * OpenID Connect protocol.
 */

#ifndef OIDC_SERVICE_H
#define OIDC_SERVICE_H

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Project Libraries
#include "../config/configuration.h"
#include "../logging/logging.h"

/*
 * OIDC Configuration
 * Defines the configuration settings for the OIDC service
 */
typedef struct {
    bool enabled;                    // Whether the OIDC service is enabled
    char *issuer;                    // The issuer identifier (URL)
    
    struct {
        char *authorization;         // Authorization endpoint path
        char *token;                 // Token endpoint path
        char *userinfo;              // UserInfo endpoint path
        char *jwks;                  // JWKS endpoint path
        char *introspection;         // Token introspection endpoint path
        char *revocation;            // Token revocation endpoint path
        char *registration;          // Client registration endpoint path
    } endpoints;
    
    struct {
        int rotation_interval_days;  // Number of days between key rotations
        char *storage_path;          // Path to key storage
        bool encryption_enabled;     // Whether to encrypt stored keys
    } keys;
    
    struct {
        int access_token_lifetime_seconds;    // Access token lifetime
        int refresh_token_lifetime_seconds;   // Refresh token lifetime
        int id_token_lifetime_seconds;        // ID token lifetime
    } tokens;
    
    struct {
        bool require_pkce;            // Whether to require PKCE
        bool allow_implicit_flow;     // Whether to allow the implicit flow
        bool allow_client_credentials; // Whether to allow client credentials
        bool require_consent;         // Whether to require user consent
    } security;
} OIDCConfig;

/*
 * OIDC Context
 * Main context structure for the OIDC service
 */
typedef struct {
    OIDCConfig config;              // OIDC configuration
    bool initialized;               // Whether the service is initialized
    bool shutting_down;             // Whether the service is shutting down
    void *key_context;              // Key management context
    void *token_context;            // Token service context
    void *user_context;             // User management context
    void *client_context;           // Client registry context
    void *data_context;             // Data storage context
} OIDCContext;

/*
 * OIDC Token Types
 * Defines the different types of tokens issued by the OIDC service
 */
typedef enum {
    TOKEN_TYPE_ACCESS,               // Access token for resource access
    TOKEN_TYPE_REFRESH,              // Refresh token for obtaining new tokens
    TOKEN_TYPE_ID                    // ID token for authentication
} OIDCTokenType;

/*
 * OIDC Grant Types
 * Defines the different grant types supported by the OIDC service
 */
typedef enum {
    GRANT_TYPE_AUTHORIZATION_CODE,   // Authorization code grant
    GRANT_TYPE_IMPLICIT,             // Implicit grant
    GRANT_TYPE_CLIENT_CREDENTIALS,   // Client credentials grant
    GRANT_TYPE_REFRESH_TOKEN         // Refresh token grant
} OIDCGrantType;

/*
 * Core OIDC functions
 */

// Initialize the OIDC service
bool init_oidc_service(OIDCConfig *config);

// Shutdown the OIDC service
void shutdown_oidc_service(void);

// Get the global OIDC context
OIDCContext* get_oidc_context(void);

// Process an authorization request
char* oidc_process_authorization_request(const char *client_id, const char *redirect_uri, 
                                        const char *response_type, const char *scope, 
                                        const char *state, const char *nonce, 
                                        const char *code_challenge, const char *code_challenge_method);

// Process a token request
char* oidc_process_token_request(const char *grant_type, const char *code, 
                                const char *redirect_uri, const char *client_id, 
                                const char *client_secret, const char *refresh_token, 
                                const char *code_verifier);

// Process a userinfo request
char* oidc_process_userinfo_request(const char *access_token);

// Process a token introspection request
char* oidc_process_introspection_request(const char *token, const char *token_type_hint, 
                                        const char *client_id, const char *client_secret);

// Process a token revocation request
bool oidc_process_revocation_request(const char *token, const char *token_type_hint, 
                                    const char *client_id, const char *client_secret);

// Generate OIDC discovery document
char* oidc_generate_discovery_document(void);

// Generate JWKS document
char* oidc_generate_jwks_document(void);

#endif // OIDC_SERVICE_H