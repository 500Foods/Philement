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

// OIDCConfig is defined in ../config/configuration.h
// No need to redefine it here

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
#include "oidc_clients.h"
#include "oidc_keys.h"
#include "oidc_tokens.h"
#include "oidc_users.h"

/*
 * Client Registry Management Functions
 * Handle OIDC client registration and lifecycle
 */
OIDCClientContext* init_oidc_client_registry(void);
void cleanup_oidc_client_registry(OIDCClientContext *client_context);

/*
 * Key Management Functions
 * Handle JWKS generation and key rotation
 */
OIDCKeyContext* init_oidc_key_management(const char *storage_path, bool encryption_enabled, int rotation_interval_days);
void cleanup_oidc_key_management(OIDCKeyContext *key_context);
char* oidc_generate_jwks(OIDCKeyContext *key_context);

/*
 * Token Service Functions
 * Handle token creation and validation
 */
OIDCTokenContext* init_oidc_token_service(OIDCKeyContext *key_context, int access_token_lifetime, int refresh_token_lifetime, int id_token_lifetime);
void cleanup_oidc_token_service(OIDCTokenContext *token_context);

/*
 * User Management Functions
 * Handle user authentication and storage
 */
OIDCUserContext* init_oidc_user_management(int max_failed_attempts, bool require_email_verification, int password_min_length);
void cleanup_oidc_user_management(OIDCUserContext *user_context);

/*
 * Endpoint Management Functions
 * Handle API endpoint registration
 */
bool init_oidc_endpoints(OIDCContext *context);
void cleanup_oidc_endpoints(void);
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