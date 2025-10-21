/*
 * OIDC Token Service
 *
 * Implements token operations for the OIDC service:
 * - JWT token generation (ID, Access, Refresh)
 * - Token validation and verification
 * - Token storage and retrieval
 * - Token revocation
 */

#ifndef OIDC_TOKENS_H
#define OIDC_TOKENS_H

#include <src/globals.h>

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Project Libraries
#include "oidc_keys.h"

/*
 * Token Context
 * Main structure for token service operations
 */
typedef struct {
    OIDCKeyContext *key_context;      // Reference to key management
    int access_token_lifetime;        // Access token lifetime in seconds
    int refresh_token_lifetime;       // Refresh token lifetime in seconds
    int id_token_lifetime;            // ID token lifetime in seconds
    void *token_storage;              // Token storage implementation (opaque)
} OIDCTokenContext;

/*
 * Token Claims
 * Standard claims used in OIDC tokens
 */
typedef struct {
    char *iss;                        // Issuer identifier
    char *sub;                        // Subject identifier
    char **aud;                       // Audience(s)
    size_t aud_count;                 // Number of audiences
    time_t exp;                       // Expiration time
    time_t iat;                       // Issued at time
    time_t nbf;                       // Not before time
    char *jti;                        // JWT ID
    char *nonce;                      // Nonce value (for ID tokens)
    char *auth_time;                  // Authentication time
    char *acr;                        // Authentication context reference
    char *amr;                        // Authentication methods references
    char *azp;                        // Authorized party
    char *scope;                      // Scope values
    char *client_id;                  // Client identifier
    char *user_data;                  // JSON string of additional user claims
} OIDCTokenClaims;

/*
 * Token Status
 * Represents the current status of a token
 */
typedef enum {
    TOKEN_STATUS_ACTIVE,              // Token is active
    TOKEN_STATUS_EXPIRED,             // Token has expired
    TOKEN_STATUS_REVOKED,             // Token has been revoked
    TOKEN_STATUS_INACTIVE,            // Token is inactive
    TOKEN_STATUS_INVALID              // Token is invalid
} OIDCTokenStatus;

/*
 * Token Service Functions
 */

/*
 * Initialize token service
 * 
 * @param key_context Key context for signing operations
 * @param access_token_lifetime Access token lifetime in seconds
 * @param refresh_token_lifetime Refresh token lifetime in seconds
 * @param id_token_lifetime ID token lifetime in seconds
 * @return Initialized token context or NULL on failure
 */
OIDCTokenContext* init_oidc_token_service(OIDCKeyContext *key_context,
                                         int access_token_lifetime,
                                         int refresh_token_lifetime,
                                         int id_token_lifetime);

/*
 * Cleanup token service resources
 * 
 * @param context Token context to clean up
 */
void cleanup_oidc_token_service(OIDCTokenContext *context);

/*
 * Create a new set of token claims with default values
 * 
 * @param issuer Issuer identifier
 * @param subject Subject identifier
 * @param audience Audience identifier
 * @return New token claims structure (caller must free with free_token_claims)
 */
OIDCTokenClaims* oidc_create_token_claims(const char *issuer, const char *subject, const char *audience);

/*
 * Free token claims resources
 * 
 * @param claims Claims to free
 */
void oidc_free_token_claims(OIDCTokenClaims *claims);

/*
 * Generate an ID token
 *
 * @param context Token context
 * @param claims Token claims
 * @return JWT string (caller must free)
 */
char* oidc_generate_id_token(const OIDCTokenContext *context, const OIDCTokenClaims *claims);

/*
 * Generate an access token
 *
 * @param context Token context
 * @param claims Token claims
 * @param reference Opaque reference token value
 * @return JWT string (caller must free)
 */
char* oidc_generate_access_token(const OIDCTokenContext *context, const OIDCTokenClaims *claims, char **reference);

/*
 * Generate a refresh token
 *
 * @param context Token context
 * @param claims Token claims
 * @return Reference token string (caller must free)
 */
char* oidc_generate_refresh_token(const OIDCTokenContext *context, const OIDCTokenClaims *claims);

/*
 * Generate an authorization code
 * 
 * @param context Token context
 * @param client_id Client identifier
 * @param redirect_uri Redirect URI
 * @param user_id User identifier
 * @param scope Requested scope
 * @param nonce Nonce value (optional)
 * @param code_challenge PKCE code challenge (optional)
 * @param code_challenge_method PKCE code challenge method (optional)
 * @return Authorization code string (caller must free)
 */
char* oidc_generate_authorization_code(OIDCTokenContext *context, const char *client_id,
                                     const char *redirect_uri, const char *user_id,
                                     const char *scope, const char *nonce,
                                     const char *code_challenge, const char *code_challenge_method);

/*
 * Validate an ID token
 * 
 * @param context Token context
 * @param id_token ID token to validate
 * @param audience Expected audience
 * @param nonce Expected nonce (optional)
 * @param claims Output parameter for parsed claims (optional, caller must free)
 * @return true if token is valid, false otherwise
 */
bool oidc_validate_id_token(OIDCTokenContext *context, const char *id_token,
                           const char *audience, const char *nonce,
                           OIDCTokenClaims **claims);

/*
 * Validate an access token
 *
 * @param context Token context
 * @param access_token Access token to validate
 * @param claims Output parameter for parsed claims (optional, caller must free)
 * @return true if token is valid, false otherwise
 */
bool oidc_validate_access_token(const OIDCTokenContext *context, const char *access_token,
                                OIDCTokenClaims **claims);

/*
 * Validate a refresh token
 *
 * @param context Token context
 * @param refresh_token Refresh token to validate
 * @param client_id Client identifier
 * @return true if token is valid, false otherwise
 */
bool oidc_validate_refresh_token(const OIDCTokenContext *context, const char *refresh_token,
                                 const char *client_id);

/*
 * Validate an authorization code
 * 
 * @param context Token context
 * @param code Authorization code to validate
 * @param client_id Client identifier
 * @param redirect_uri Redirect URI
 * @param code_verifier PKCE code verifier (optional)
 * @return User ID if code is valid, NULL otherwise
 */
char* oidc_validate_authorization_code(OIDCTokenContext *context, const char *code,
                                     const char *client_id, const char *redirect_uri,
                                     const char *code_verifier);

/*
 * Revoke a token
 *
 * @param context Token context
 * @param token Token to revoke
 * @param token_type_hint Token type hint (optional)
 * @param client_id Client identifier
 * @return true if token was revoked, false otherwise
 */
bool oidc_revoke_token(const OIDCTokenContext *context, const char *token,
                       const char *token_type_hint, const char *client_id);

/*
 * Get token status
 * 
 * @param context Token context
 * @param token Token to check
 * @param token_type_hint Token type hint (optional)
 * @return Token status
 */
OIDCTokenStatus oidc_get_token_status(OIDCTokenContext *context, const char *token,
                                    const char *token_type_hint);

/*
 * Parse a JWT token
 * 
 * @param token JWT token to parse
 * @return Token claims (caller must free with free_token_claims)
 */
OIDCTokenClaims* oidc_parse_jwt(const char *token);

/*
 * Create a JWT token
 * 
 * @param context Token context
 * @param claims Token claims
 * @param key Key to use for signing
 * @return JWT string (caller must free)
 */
char* oidc_create_jwt(OIDCTokenContext *context, OIDCTokenClaims *claims, OIDCKey *key);

#endif // OIDC_TOKENS_H
