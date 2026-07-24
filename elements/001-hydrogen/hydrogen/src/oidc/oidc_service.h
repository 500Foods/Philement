/*
 * OpenID Connect (OIDC) Service
 *
 * Main header for Hydrogen's OIDC IdP: lifecycle, authorization, token,
 * userinfo, introspection, revocation, and discovery/JWKS.
 *
 * Implementation is split across:
 *   oidc_service.c              — init / shutdown / get_oidc_context
 *   oidc_service_authorize.c    — resource owner + auth codes
 *   oidc_service_token.c        — token endpoint + mint helpers
 *   oidc_service_userinfo.c     — userinfo + scope helpers
 *   oidc_service_introspect.c   — RFC 7662 introspection
 *   oidc_service_revoke.c       — RFC 7009 revocation
 *   oidc_service_discovery.c    — discovery document + JWKS
 */

#ifndef OIDC_SERVICE_H
#define OIDC_SERVICE_H

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Third-party (userinfo/introspect helpers)
#include <jansson.h>

/*
 * OIDC Context
 * Main context structure for the OIDC service
 */
#include "oidc_clients.h"
#include "oidc_keys.h"
#include "oidc_tokens.h"
#include "oidc_users.h"
#include "oidc_auth_codes.h"
#include "oidc_refresh_tokens.h"
#include "oidc_pkce.h"

typedef struct {
    OIDCConfig config;              // OIDC configuration
    bool initialized;               // Whether the service is initialized
    bool shutting_down;             // Whether the service is shutting down
    void *key_context;              // Key management context
    void *token_context;            // Token service context
    void *user_context;             // User management context
    void *client_context;           // Client registry context
    OIDCAuthCodeStore *auth_code_store; // Authorization codes (in-memory)
    OIDCRefreshStore *refresh_store;    // Refresh tokens (in-memory)
    char *database_name;            // Accounts DB name for resource-owner auth
} OIDCContext;

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
char* oidc_generate_jwks(const OIDCKeyContext *key_context);

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
 * Core OIDC lifecycle
 */

// Free a context and all owned subcomponents (partial or full init).
void oidc_service_release_context(OIDCContext *ctx);

// Initialize the OIDC service
bool init_oidc_service(const OIDCConfig *config);

// Shutdown the OIDC service
void shutdown_oidc_service(void);

// Get the global OIDC context
OIDCContext* get_oidc_context(void);

/*
 * Issue an authorization code after client + resource-owner authentication.
 * Returns plaintext code (caller frees) or NULL on failure.
 * error_code receives a short OAuth error token when non-NULL and failure.
 */
char* oidc_issue_authorization_code(const char *client_id,
                                    const char *redirect_uri,
                                    const char *scope,
                                    const char *nonce,
                                    const char *code_challenge,
                                    const char *code_challenge_method,
                                    int account_id,
                                    const char **error_code);

/* Authenticate resource owner via accounts + QueryRef #012 path. */
bool oidc_authenticate_resource_owner(const char *login_id,
                                      const char *password,
                                      int *account_id_out);

/* Resolve DB name used for IdP resource-owner auth. */
const char* oidc_get_accounts_database(void);

// Legacy wrapper — prefer oidc_issue_authorization_code after login
char* oidc_process_authorization_request(const char *client_id, const char *redirect_uri,
                                        const char *response_type, const char *scope,
                                        const char *state, const char *nonce,
                                        const char *code_challenge, const char *code_challenge_method);

// Process a token request (returns JSON success or OAuth error object; caller frees)
char* oidc_process_token_request(const char *grant_type, const char *code,
                                const char *redirect_uri, const char *client_id,
                                const char *client_secret, const char *refresh_token,
                                const char *code_verifier);

/* Build OAuth token-endpoint error JSON (caller frees). */
char* oidc_token_error_json(const char *error, const char *description);

bool oidc_client_allows_refresh(const OIDCClient *client);
bool oidc_should_issue_refresh(const OIDCClient *client, const char *scope);
char* oidc_build_token_response_json(const char *access_token,
                                     const char *id_token,
                                     int expires_in,
                                     const char *scope,
                                     const char *refresh_token_or_null);
char* oidc_mint_token_pair_response(int account_id,
                                    const char *client_id,
                                    const char *scope,
                                    const char *nonce_or_null,
                                    const char *refresh_plaintext_or_null);

/* Grant-specific token handlers (used by oidc_process_token_request). */
char* oidc_token_handle_authorization_code(const char *code,
                                           const char *redirect_uri,
                                           const char *client_id,
                                           const char *code_verifier,
                                           const OIDCClient *client);
char* oidc_token_handle_refresh_token(const char *refresh_token,
                                      const char *client_id,
                                      const OIDCClient *client);

// Process a userinfo request (returns claims JSON or NULL if token invalid)
char* oidc_process_userinfo_request(const char *access_token);

/* True if space-delimited scope list contains needle. */
bool oidc_scope_has(const char *scope, const char *needle);

/* Merge email/profile claims from user_data JSON into out (by scope). */
void oidc_userinfo_apply_scoped_claims(json_t *out, const char *scope, const char *user_data);

char* oidc_inactive_json(void);
char* oidc_introspect_access_json(const OIDCTokenClaims *claims, const char *client_id);
char* oidc_introspect_refresh_json(const OIDCRefreshRecord *rec, const char *client_id);
char* oidc_introspect_try_access(const OIDCContext *ctx, const char *token, const char *client_id);
char* oidc_introspect_try_refresh(OIDCContext *ctx, const char *token, const char *client_id);

// Process a token introspection request
char* oidc_process_introspection_request(const char *token, const char *token_type_hint,
                                        const char *client_id, const char *client_secret);

bool oidc_revoke_try_refresh(OIDCContext *ctx, const char *token, const char *client_id);
bool oidc_revoke_try_access(const OIDCContext *ctx, const char *token);

// Process a token revocation request
bool oidc_process_revocation_request(const char *token, const char *token_type_hint,
                                    const char *client_id, const char *client_secret);

/* Discovery URL helpers */
char* oidc_discovery_join_url(const char *issuer, const char *path);
const char* oidc_discovery_endpoint_or_default(const char *configured, const char *fallback);

// Generate OIDC discovery document
char* oidc_generate_discovery_document(void);

// Generate JWKS document
char* oidc_generate_jwks_document(void);

#endif // OIDC_SERVICE_H
