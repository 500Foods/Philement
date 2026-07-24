/*
 * OpenID Connect (OIDC) Service Implementation
 *
 * Core implementation of Hydrogen's OIDC identity provider functionality:
 * - Service initialization and configuration
 * - Component coordination
 * - Protocol flow handling
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_keys.h"
#include "oidc_tokens.h"
#include "oidc_users.h"
#include "oidc_auth_codes.h"
#include "oidc_refresh_tokens.h"
#include "oidc_pkce.h"
#include <src/api/oidc/oidc_service.h>
#include <src/api/auth/auth_service.h>

// Global OIDC context
static OIDCContext *oidc_context = NULL;

/*
 * Initialize the OIDC service
 *
 * This function initializes all components of the OIDC service:
 * - Key management for JWT signing
 * - Token service for creating and validating tokens
 * - User management for authentication and user data
 * - Client registry for client applications
 * - API endpoints for protocol handling
 *
 * @param config OIDC configuration
 * @return true if initialization successful, false otherwise
 */
bool init_oidc_service(const OIDCConfig *config) {
    if (!config) {
        log_this(SR_OIDC, "Invalid configuration provided", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create OIDC context
    oidc_context = (OIDCContext *)malloc(sizeof(OIDCContext));
    if (!oidc_context) {
        log_this(SR_OIDC, "Failed to allocate OIDC context", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Copy configuration (shallow — strings owned by app_config)
    memcpy(&oidc_context->config, config, sizeof(OIDCConfig));
    oidc_context->initialized = false;
    oidc_context->shutting_down = false;
    oidc_context->auth_code_store = NULL;
    oidc_context->refresh_store = NULL;
    oidc_context->database_name = NULL;
    if (config->database && config->database[0] != '\0') {
        oidc_context->database_name = strdup(config->database);
    }

    // Authorization code store (in-memory; DB QueryRefs for multi-process later)
    oidc_context->auth_code_store = oidc_auth_code_store_create(300);
    if (!oidc_context->auth_code_store) {
        log_this(SR_OIDC, "Failed to create authorization code store", LOG_LEVEL_ERROR, 0);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    int refresh_ttl = config->tokens.refresh_token_lifetime > 0
        ? config->tokens.refresh_token_lifetime
        : OIDC_REFRESH_DEFAULT_TTL_SEC;
    oidc_context->refresh_store = oidc_refresh_store_create(refresh_ttl);
    if (!oidc_context->refresh_store) {
        log_this(SR_OIDC, "Failed to create refresh token store", LOG_LEVEL_ERROR, 0);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    // Initialize key management
    log_this(SR_OIDC, "Initializing key management", LOG_LEVEL_STATE, 0);
    oidc_context->key_context = init_oidc_key_management(
        config->keys.storage_path,
        config->keys.encryption_enabled,
        config->keys.rotation_interval_days
    );
    if (!oidc_context->key_context) {
        log_this(SR_OIDC, "Failed to initialize key management", LOG_LEVEL_ERROR, 0);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    // Initialize token service
    log_this(SR_OIDC, "Initializing token service", LOG_LEVEL_STATE, 0);
    oidc_context->token_context = init_oidc_token_service(
        oidc_context->key_context,
        config->tokens.access_token_lifetime,
        config->tokens.refresh_token_lifetime,
        config->tokens.id_token_lifetime
    );
    if (!oidc_context->token_context) {
        log_this(SR_OIDC, "Failed to initialize token service", LOG_LEVEL_ERROR, 0);
        cleanup_oidc_key_management(oidc_context->key_context);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    // Initialize user management
    log_this(SR_OIDC, "Initializing user management", LOG_LEVEL_STATE, 0);
    oidc_context->user_context = init_oidc_user_management(
        5, // max_failed_attempts
        true, // require_email_verification
        8 // password_min_length
    );
    if (!oidc_context->user_context) {
        log_this(SR_OIDC, "Failed to initialize user management", LOG_LEVEL_ERROR, 0);
        cleanup_oidc_token_service(oidc_context->token_context);
        cleanup_oidc_key_management(oidc_context->key_context);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    // Initialize client registry
    log_this(SR_OIDC, "Initializing client registry", LOG_LEVEL_STATE, 0);
    oidc_context->client_context = init_oidc_client_registry();
    if (!oidc_context->client_context) {
        log_this(SR_OIDC, "Failed to initialize client registry", LOG_LEVEL_ERROR, 0);
        cleanup_oidc_user_management(oidc_context->user_context);
        cleanup_oidc_token_service(oidc_context->token_context);
        cleanup_oidc_key_management(oidc_context->key_context);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    /* Optional config seed (ClientId + RedirectUri) for tests/dev until DB load. */
    if (!oidc_seed_client_from_config((OIDCClientContext*)oidc_context->client_context,
                                     config->client_id, config->client_secret,
                                     config->redirect_uri)) {
        log_this(SR_OIDC, "Failed to seed OIDC client from config", LOG_LEVEL_ERROR, 0);
        cleanup_oidc_client_registry(oidc_context->client_context);
        cleanup_oidc_user_management(oidc_context->user_context);
        cleanup_oidc_token_service(oidc_context->token_context);
        cleanup_oidc_key_management(oidc_context->key_context);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    // Initialize API endpoints
    log_this(SR_OIDC, "Initializing API endpoints", LOG_LEVEL_STATE, 0);
    if (!init_oidc_endpoints(oidc_context)) {
        log_this(SR_OIDC, "Failed to initialize API endpoints", LOG_LEVEL_ERROR, 0);
        cleanup_oidc_client_registry(oidc_context->client_context);
        cleanup_oidc_user_management(oidc_context->user_context);
        cleanup_oidc_token_service(oidc_context->token_context);
        cleanup_oidc_key_management(oidc_context->key_context);
        oidc_refresh_store_destroy(oidc_context->refresh_store);
        oidc_auth_code_store_destroy(oidc_context->auth_code_store);
        free(oidc_context->database_name);
        free(oidc_context);
        oidc_context = NULL;
        return false;
    }

    oidc_context->initialized = true;
    log_this(SR_OIDC, "OIDC service initialized successfully", LOG_LEVEL_STATE, 0);
    return true;
}

/*
 * Shutdown the OIDC service
 *
 * This function performs a clean shutdown of all OIDC components.
 */
void shutdown_oidc_service(void) {
    if (!oidc_context) {
        return;
    }

    oidc_context->shutting_down = true;
    log_this(SR_OIDC, "Shutting down OIDC service", LOG_LEVEL_STATE, 0);

    // Clean up components in reverse initialization order
    cleanup_oidc_endpoints();
    cleanup_oidc_client_registry(oidc_context->client_context);
    cleanup_oidc_user_management(oidc_context->user_context);
    cleanup_oidc_token_service(oidc_context->token_context);
    cleanup_oidc_key_management(oidc_context->key_context);
    oidc_refresh_store_destroy(oidc_context->refresh_store);
    oidc_auth_code_store_destroy(oidc_context->auth_code_store);
    free(oidc_context->database_name);

    free(oidc_context);
    oidc_context = NULL;
    
    log_this(SR_OIDC, "OIDC service shutdown complete", LOG_LEVEL_STATE, 0);
}

/*
 * Get the global OIDC context
 *
 * @return OIDC context pointer
 */
OIDCContext* get_oidc_context(void) {
    return oidc_context;
}

/*
 * Process an authorization request
 *
 * Handles the OAuth 2.0 authorization request flow.
 * 
 * @param client_id Client identifier
 * @param redirect_uri Redirection URI
 * @param response_type Response type (code, token, etc.)
 * @param scope Requested scope
 * @param state State parameter for CSRF protection
 * @param nonce Nonce parameter for replay protection
 * @param code_challenge PKCE code challenge
 * @param code_challenge_method PKCE code challenge method
 * @return JSON response or NULL on error
 */
const char* oidc_get_accounts_database(void) {
    if (oidc_context && oidc_context->database_name && oidc_context->database_name[0] != '\0') {
        return oidc_context->database_name;
    }
    if (app_config && app_config->oidc.database && app_config->oidc.database[0] != '\0') {
        return app_config->oidc.database;
    }
    /* Fall back to first configured connection name when present. */
    if (app_config && app_config->databases.connection_count > 0) {
        const DatabaseConnection *c = &app_config->databases.connections[0];
        if (c->name && c->name[0] != '\0') {
            return c->name;
        }
        if (c->connection_name && c->connection_name[0] != '\0') {
            return c->connection_name;
        }
    }
    return "demo";
}

bool oidc_authenticate_resource_owner(const char *login_id,
                                      const char *password,
                                      int *account_id_out) {
    if (!login_id || !password || !account_id_out) {
        return false;
    }
    *account_id_out = 0;

    const char *database = oidc_get_accounts_database();
    account_info_t *account = lookup_account(login_id, database);
    if (!account) {
        log_this(SR_OIDC, "Resource owner account not found", LOG_LEVEL_DEBUG, 0);
        return false;
    }
    if (!account->enabled || !account->authorized) {
        free_account_info(account);
        return false;
    }
    if (!verify_password_and_status(password, account->id, database, account)) {
        free_account_info(account);
        return false;
    }
    *account_id_out = account->id;
    free_account_info(account);
    return true;
}

char* oidc_issue_authorization_code(const char *client_id,
                                    const char *redirect_uri,
                                    const char *scope,
                                    const char *nonce,
                                    const char *code_challenge,
                                    const char *code_challenge_method,
                                    int account_id,
                                    const char **error_code) {
    if (error_code) {
        *error_code = NULL;
    }
    if (!oidc_context || !oidc_context->initialized || !oidc_context->auth_code_store) {
        if (error_code) {
            *error_code = "server_error";
        }
        return NULL;
    }
    if (!client_id || !redirect_uri || account_id <= 0) {
        if (error_code) {
            *error_code = "invalid_request";
        }
        return NULL;
    }
    if (!oidc_pkce_method_is_s256(code_challenge_method) || !code_challenge) {
        if (error_code) {
            *error_code = "invalid_request";
        }
        return NULL;
    }

    OIDCClientContext *clients = (OIDCClientContext*)oidc_context->client_context;
    if (!oidc_validate_client(clients, client_id, redirect_uri)) {
        if (error_code) {
            *error_code = "unauthorized_client";
        }
        return NULL;
    }

    /* PKCE S256 + non-null challenge already enforced above; require_pkce
     * clients are covered by that gate (S256 is mandatory for all clients). */

    const char *use_scope = (scope && scope[0] != '\0') ? scope : "openid";
    char *code = oidc_auth_code_issue(oidc_context->auth_code_store,
                                      client_id, account_id, redirect_uri,
                                      use_scope, nonce,
                                      code_challenge, code_challenge_method);
    if (!code) {
        if (error_code) {
            *error_code = "server_error";
        }
        return NULL;
    }
    return code;
}

char* oidc_process_authorization_request(const char *client_id, const char *redirect_uri,
                                        const char *response_type, const char *scope,
                                        const char *state, const char *nonce,
                                        const char *code_challenge, const char *code_challenge_method) {
    (void)state;
    if (!response_type || strcmp(response_type, "code") != 0) {
        return NULL;
    }
    /* Without authenticated account_id this legacy entry cannot issue codes. */
    (void)client_id;
    (void)redirect_uri;
    (void)scope;
    (void)nonce;
    (void)code_challenge;
    (void)code_challenge_method;
    log_this(SR_OIDC, "Authorization requires resource-owner login (POST)", LOG_LEVEL_DEBUG, 0);
    return NULL;
}

/*
 * Process a token request
 *
 * Handles OAuth 2.0 token requests.
 * 
 * @param grant_type Grant type (authorization_code, refresh_token, etc.)
 * @param code Authorization code (for authorization_code grant)
 * @param redirect_uri Redirection URI (for authorization_code grant)
 * @param client_id Client identifier
 * @param client_secret Client secret
 * @param refresh_token Refresh token (for refresh_token grant)
 * @param code_verifier PKCE code verifier
 * @return JSON response or NULL on error
 */
char* oidc_token_error_json(const char *error, const char *description) {
    char *out = NULL;
    const char *desc = description ? description : "";
    if (!error) {
        return NULL;
    }
    if (asprintf(&out, "{\"error\":\"%s\",\"error_description\":\"%s\"}", error, desc) < 0) {
        return NULL;
    }
    return out;
}

bool oidc_client_allows_refresh(const OIDCClient *client) {
    if (!client || !client->grant_types[0]) {
        return false;
    }
    return strstr(client->grant_types, "refresh_token") != NULL;
}

bool oidc_should_issue_refresh(const OIDCClient *client, const char *scope) {
    if (!oidc_client_allows_refresh(client)) {
        return false;
    }
    /* Issue when offline_access requested, or client is allowed and scope openid. */
    if (oidc_scope_has(scope, "offline_access")) {
        return true;
    }
    /* Also issue when client lists refresh_token (first-party convenience). */
    return true;
}

char* oidc_build_token_response_json(const char *access_token,
                                     const char *id_token,
                                     int expires_in,
                                     const char *scope,
                                     const char *refresh_token_or_null) {
    char *response = NULL;
    const char *use_scope = (scope && scope[0] != '\0') ? scope : "openid";
    if (refresh_token_or_null && refresh_token_or_null[0] != '\0') {
        if (asprintf(&response,
                     "{\"access_token\":\"%s\",\"token_type\":\"Bearer\",\"expires_in\":%d,"
                     "\"id_token\":\"%s\",\"scope\":\"%s\",\"refresh_token\":\"%s\"}",
                     access_token, expires_in, id_token, use_scope,
                     refresh_token_or_null) < 0) {
            return NULL;
        }
    } else {
        if (asprintf(&response,
                     "{\"access_token\":\"%s\",\"token_type\":\"Bearer\",\"expires_in\":%d,"
                     "\"id_token\":\"%s\",\"scope\":\"%s\"}",
                     access_token, expires_in, id_token, use_scope) < 0) {
            return NULL;
        }
    }
    return response;
}

char* oidc_mint_token_pair_response(int account_id,
                                    const char *client_id,
                                    const char *scope,
                                    const char *nonce_or_null,
                                    const char *refresh_plaintext_or_null) {
    if (!oidc_context || !oidc_context->token_context || account_id <= 0 || !client_id) {
        return NULL;
    }

    const char *issuer = oidc_context->config.issuer ? oidc_context->config.issuer : "";
    char sub_buf[32];
    snprintf(sub_buf, sizeof(sub_buf), "%d", account_id);

    OIDCTokenClaims *claims = oidc_create_token_claims(issuer, sub_buf, client_id);
    if (!claims) {
        return NULL;
    }
    if (scope && scope[0] != '\0') {
        claims->scope = strdup(scope);
    }
    if (nonce_or_null && nonce_or_null[0] != '\0') {
        claims->nonce = strdup(nonce_or_null);
    }
    {
        char auth_time_buf[32];
        snprintf(auth_time_buf, sizeof(auth_time_buf), "%ld", (long)time(NULL));
        claims->auth_time = strdup(auth_time_buf);
    }

    OIDCTokenContext *tok_ctx = (OIDCTokenContext*)oidc_context->token_context;
    char *id_token = oidc_generate_id_token(tok_ctx, claims);
    char *access_token = oidc_generate_access_token(tok_ctx, claims, NULL);
    int expires_in = tok_ctx->access_token_lifetime > 0 ? tok_ctx->access_token_lifetime : 3600;
    oidc_free_token_claims(claims);

    if (!id_token || !access_token) {
        free(id_token);
        free(access_token);
        return NULL;
    }

    char *response = oidc_build_token_response_json(access_token, id_token, expires_in,
                                                    scope, refresh_plaintext_or_null);
    free(id_token);
    free(access_token);
    return response;
}

char* oidc_process_token_request(const char *grant_type, const char *code,
                                const char *redirect_uri, const char *client_id,
                                const char *client_secret, const char *refresh_token,
                                const char *code_verifier) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return oidc_token_error_json("server_error", "OIDC service not initialized");
    }
    if (!grant_type || grant_type[0] == '\0') {
        return oidc_token_error_json("invalid_request", "Missing grant_type");
    }
    if (strcmp(grant_type, "authorization_code") != 0 &&
        strcmp(grant_type, "refresh_token") != 0) {
        return oidc_token_error_json("unsupported_grant_type", "Unsupported grant_type");
    }
    if (!client_id || client_id[0] == '\0') {
        return oidc_token_error_json("invalid_client", "Missing client_id");
    }

    OIDCClientContext *clients = (OIDCClientContext*)oidc_context->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Token request client authentication failed", LOG_LEVEL_DEBUG, 0);
        return oidc_token_error_json("invalid_client", "Client authentication failed");
    }

    OIDCClient *client = oidc_client_registry_find(clients, client_id);
    if (!oidc_context->token_context) {
        return oidc_token_error_json("server_error", "Token service unavailable");
    }

    if (strcmp(grant_type, "authorization_code") == 0) {
        if (!code || !redirect_uri || !code_verifier) {
            return oidc_token_error_json("invalid_request",
                                         "code, redirect_uri, and code_verifier required");
        }
        if (!oidc_context->auth_code_store) {
            return oidc_token_error_json("server_error", "Token service unavailable");
        }

        OIDCAuthCodeRecord rec;
        memset(&rec, 0, sizeof(rec));
        if (!oidc_auth_code_consume(oidc_context->auth_code_store, code, client_id,
                                    redirect_uri, code_verifier, &rec)) {
            return oidc_token_error_json("invalid_grant",
                                         "Invalid, expired, or already used authorization code");
        }

        char *refresh_plain = NULL;
        if (client && oidc_should_issue_refresh(client, rec.scope) &&
            oidc_context->refresh_store) {
            refresh_plain = oidc_refresh_issue(oidc_context->refresh_store, client_id,
                                               rec.account_id, rec.scope, NULL);
        }

        char *response = oidc_mint_token_pair_response(
            rec.account_id, client_id, rec.scope,
            rec.nonce[0] ? rec.nonce : NULL, refresh_plain);
        free(refresh_plain);

        if (!response) {
            return oidc_token_error_json("server_error", "Failed to mint tokens");
        }
        log_this(SR_OIDC, "Issued tokens for authorization_code grant", LOG_LEVEL_STATE, 0);
        return response;
    }

    if (strcmp(grant_type, "refresh_token") == 0) {
        if (!refresh_token || refresh_token[0] == '\0') {
            return oidc_token_error_json("invalid_request", "refresh_token required");
        }
        if (!client || !oidc_client_allows_refresh(client)) {
            return oidc_token_error_json("unauthorized_client",
                                         "Client not allowed to use refresh_token");
        }
        if (!oidc_context->refresh_store) {
            return oidc_token_error_json("server_error", "Refresh store unavailable");
        }

        OIDCRefreshRecord rrec;
        memset(&rrec, 0, sizeof(rrec));
        char *new_refresh = NULL;
        if (!oidc_refresh_rotate(oidc_context->refresh_store, refresh_token, client_id,
                                 &rrec, &new_refresh)) {
            return oidc_token_error_json("invalid_grant",
                                         "Invalid, expired, or reused refresh token");
        }

        char *response = oidc_mint_token_pair_response(
            rrec.account_id, client_id, rrec.scope, NULL, new_refresh);
        free(new_refresh);

        if (!response) {
            return oidc_token_error_json("server_error", "Failed to mint tokens");
        }
        log_this(SR_OIDC, "Issued tokens for refresh_token grant", LOG_LEVEL_STATE, 0);
        return response;
    }

    return oidc_token_error_json("unsupported_grant_type", "Unsupported grant_type");
}

/*
 * Process a userinfo request
 *
 * Retrieves user information based on an access token.
 * 
 * @param access_token Access token
 * @return JSON response or NULL on error
 */
bool oidc_scope_has(const char *scope, const char *needle) {
    if (!scope || !needle || needle[0] == '\0') {
        return false;
    }
    size_t nlen = strlen(needle);
    const char *p = scope;
    while (p && *p) {
        while (*p == ' ') {
            p++;
        }
        if (strncmp(p, needle, nlen) == 0 &&
            (p[nlen] == '\0' || p[nlen] == ' ')) {
            return true;
        }
        p = strchr(p, ' ');
    }
    return false;
}

char* oidc_process_userinfo_request(const char *access_token) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    if (!access_token || access_token[0] == '\0') {
        return NULL;
    }

    OIDCTokenContext *tok = (OIDCTokenContext*)oidc_context->token_context;
    if (!tok) {
        return NULL;
    }

    OIDCTokenClaims *claims = NULL;
    if (!oidc_validate_access_token(tok, access_token, &claims) || !claims) {
        log_this(SR_OIDC, "Userinfo rejected invalid access token", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    const char *issuer = oidc_context->config.issuer ? oidc_context->config.issuer : "";
    if (claims->iss && issuer[0] != '\0' && strcmp(claims->iss, issuer) != 0) {
        oidc_free_token_claims(claims);
        return NULL;
    }

    json_t *out = json_object();
    if (!out) {
        oidc_free_token_claims(claims);
        return NULL;
    }
    json_object_set_new(out, "sub", json_string(claims->sub ? claims->sub : ""));

    /* openid is required for userinfo; always return sub. */
    const char *scope = claims->scope ? claims->scope : "openid";
    if (claims->user_data && claims->user_data[0] != '\0') {
        json_error_t err;
        json_t *extra = json_loads(claims->user_data, 0, &err);
        if (extra && json_is_object(extra)) {
            if (oidc_scope_has(scope, "email")) {
                json_t *email = json_object_get(extra, "email");
                json_t *ev = json_object_get(extra, "email_verified");
                if (email) {
                    json_object_set(out, "email", email);
                }
                if (ev) {
                    json_object_set(out, "email_verified", ev);
                }
            }
            if (oidc_scope_has(scope, "profile")) {
                json_t *name = json_object_get(extra, "name");
                json_t *pref = json_object_get(extra, "preferred_username");
                if (name) {
                    json_object_set(out, "name", name);
                }
                if (pref) {
                    json_object_set(out, "preferred_username", pref);
                }
            }
        }
        if (extra) {
            json_decref(extra);
        }
    }

    /*
     * Accounts DB profile fill deferred — access token currently carries
     * sub (+ optional user_data). Prefer JWT claims until lookup-by-id exists.
     */
    (void)scope;

    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    oidc_free_token_claims(claims);
    return json;
}

/*
 * Process a token introspection request
 *
 * Checks the state and validity of a token.
 * 
 * @param token Token to introspect
 * @param token_type_hint Token type hint
 * @param client_id Client identifier
 * @param client_secret Client secret
 * @return JSON response or NULL on error
 */
char* oidc_inactive_json(void) {
    return strdup("{\"active\":false}");
}

char* oidc_introspect_access_json(const OIDCTokenClaims *claims, const char *client_id) {
    if (!claims || !claims->sub) {
        return oidc_inactive_json();
    }
    /* Token must be bound to this client when client_id claim present. */
    if (claims->client_id && claims->client_id[0] != '\0' && client_id &&
        strcmp(claims->client_id, client_id) != 0) {
        return oidc_inactive_json();
    }
    if (claims->aud_count > 0U && claims->aud && claims->aud[0] && client_id &&
        strcmp(claims->aud[0], client_id) != 0 &&
        (!claims->client_id || claims->client_id[0] == '\0')) {
        return oidc_inactive_json();
    }

    json_t *out = json_object();
    if (!out) {
        return NULL;
    }
    json_object_set_new(out, "active", json_true());
    json_object_set_new(out, "token_type", json_string("access_token"));
    json_object_set_new(out, "sub", json_string(claims->sub));
    if (claims->client_id && claims->client_id[0] != '\0') {
        json_object_set_new(out, "client_id", json_string(claims->client_id));
    } else if (client_id) {
        json_object_set_new(out, "client_id", json_string(client_id));
    }
    if (claims->scope && claims->scope[0] != '\0') {
        json_object_set_new(out, "scope", json_string(claims->scope));
    }
    if (claims->exp > 0) {
        json_object_set_new(out, "exp", json_integer((json_int_t)claims->exp));
    }
    if (claims->iat > 0) {
        json_object_set_new(out, "iat", json_integer((json_int_t)claims->iat));
    }
    if (claims->iss && claims->iss[0] != '\0') {
        json_object_set_new(out, "iss", json_string(claims->iss));
    }
    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    return json;
}

char* oidc_introspect_refresh_json(const OIDCRefreshRecord *rec, const char *client_id) {
    if (!rec || !client_id) {
        return oidc_inactive_json();
    }
    if (strcmp(rec->client_id, client_id) != 0) {
        return oidc_inactive_json();
    }
    time_t now = time(NULL);
    if (rec->revoked_at != 0 || rec->expires_at < now) {
        return oidc_inactive_json();
    }

    json_t *out = json_object();
    if (!out) {
        return NULL;
    }
    char sub_buf[32];
    snprintf(sub_buf, sizeof(sub_buf), "%d", rec->account_id);
    json_object_set_new(out, "active", json_true());
    json_object_set_new(out, "token_type", json_string("refresh_token"));
    json_object_set_new(out, "client_id", json_string(rec->client_id));
    json_object_set_new(out, "sub", json_string(sub_buf));
    if (rec->scope[0] != '\0') {
        json_object_set_new(out, "scope", json_string(rec->scope));
    }
    json_object_set_new(out, "exp", json_integer((json_int_t)rec->expires_at));
    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    return json;
}

char* oidc_process_introspection_request(const char *token, const char *token_type_hint,
                                        const char *client_id, const char *client_secret) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    if (!client_id || client_id[0] == '\0') {
        return NULL;
    }
    if (!token || token[0] == '\0') {
        return oidc_inactive_json();
    }

    OIDCClientContext *clients = (OIDCClientContext*)oidc_context->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Introspection client authentication failed", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    log_this(SR_OIDC, "Processing introspection request", LOG_LEVEL_STATE, 0);

    bool prefer_access = true;
    bool prefer_refresh = true;
    if (token_type_hint && token_type_hint[0] != '\0') {
        if (strcmp(token_type_hint, "access_token") == 0) {
            prefer_refresh = false;
        } else if (strcmp(token_type_hint, "refresh_token") == 0) {
            prefer_access = false;
        }
    }

    /* Try preferred type first, then the other (hint is advisory per RFC 7662). */
    if (prefer_access && oidc_context->token_context) {
        OIDCTokenClaims *claims = NULL;
        if (oidc_validate_access_token((OIDCTokenContext*)oidc_context->token_context,
                                       token, &claims) && claims) {
            char *json = oidc_introspect_access_json(claims, client_id);
            oidc_free_token_claims(claims);
            if (json) {
                return json;
            }
        }
    }

    if (prefer_refresh && oidc_context->refresh_store) {
        OIDCRefreshRecord rec;
        memset(&rec, 0, sizeof(rec));
        if (oidc_refresh_lookup(oidc_context->refresh_store, token, &rec)) {
            return oidc_introspect_refresh_json(&rec, client_id);
        }
    }

    if (!prefer_access && oidc_context->token_context) {
        OIDCTokenClaims *claims = NULL;
        if (oidc_validate_access_token((OIDCTokenContext*)oidc_context->token_context,
                                       token, &claims) && claims) {
            char *json = oidc_introspect_access_json(claims, client_id);
            oidc_free_token_claims(claims);
            if (json) {
                return json;
            }
        }
    }

    if (!prefer_refresh && oidc_context->refresh_store) {
        OIDCRefreshRecord rec;
        memset(&rec, 0, sizeof(rec));
        if (oidc_refresh_lookup(oidc_context->refresh_store, token, &rec)) {
            return oidc_introspect_refresh_json(&rec, client_id);
        }
    }

    return oidc_inactive_json();
}

/*
 * Process a token revocation request (RFC 7009).
 * Returns true when the request was accepted (including unknown tokens).
 * Returns false only when service is down or client authentication fails.
 */
bool oidc_process_revocation_request(const char *token, const char *token_type_hint,
                                    const char *client_id, const char *client_secret) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!client_id || client_id[0] == '\0') {
        return false;
    }

    OIDCClientContext *clients = (OIDCClientContext*)oidc_context->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Revocation client authentication failed", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    if (!token || token[0] == '\0') {
        /* RFC 7009: invalid/missing token still yields success response after auth. */
        return true;
    }

    log_this(SR_OIDC, "Processing revocation request", LOG_LEVEL_STATE, 0);

    bool try_refresh = true;
    bool try_access = true;
    if (token_type_hint && strcmp(token_type_hint, "access_token") == 0) {
        try_refresh = false;
    } else if (token_type_hint && strcmp(token_type_hint, "refresh_token") == 0) {
        try_access = false;
    }

    if (try_refresh && oidc_context->refresh_store) {
        OIDCRefreshRecord rec;
        memset(&rec, 0, sizeof(rec));
        if (oidc_refresh_lookup(oidc_context->refresh_store, token, &rec)) {
            if (strcmp(rec.client_id, client_id) == 0) {
                (void)oidc_refresh_revoke(oidc_context->refresh_store, token);
            }
            /* Known refresh handled (or wrong client — still 200, no leak). */
            return true;
        }
    }

    if (try_access && oidc_context->token_context) {
        /*
         * Access tokens are pure RS256 JWTs with no server denylist.
         * Revocation is a no-op success (RFC 7009); tokens remain valid until exp.
         * Prefer short access TTL; revoke refresh for session kill.
         */
        OIDCTokenClaims *claims = NULL;
        if (oidc_validate_access_token((OIDCTokenContext*)oidc_context->token_context,
                                       token, &claims)) {
            oidc_free_token_claims(claims);
            return true;
        }
    }

    /* Unknown token: still success per RFC 7009. */
    return true;
}

/*
 * Generate OIDC discovery document
 *
 * Creates the OpenID Provider configuration document.
 * 
 * @return JSON response or NULL on error
 */
char* oidc_generate_discovery_document(void) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Log discovery document request
    log_this(SR_OIDC, "Generating discovery document", LOG_LEVEL_STATE, 0);

    const char *issuer = oidc_context->config.issuer ? oidc_context->config.issuer : "";
    const char *auth_endpoint = oidc_context->config.endpoints.authorization
        ? oidc_context->config.endpoints.authorization : "/oauth/authorize";
    const char *token_endpoint = oidc_context->config.endpoints.token
        ? oidc_context->config.endpoints.token : "/oauth/token";
    const char *userinfo_endpoint = oidc_context->config.endpoints.userinfo
        ? oidc_context->config.endpoints.userinfo : "/oauth/userinfo";
    const char *jwks_endpoint = oidc_context->config.endpoints.jwks
        ? oidc_context->config.endpoints.jwks : "/oauth/jwks";
    const char *introspect_endpoint = oidc_context->config.endpoints.introspection
        ? oidc_context->config.endpoints.introspection : "/oauth/introspect";
    const char *revoke_endpoint = oidc_context->config.endpoints.revocation
        ? oidc_context->config.endpoints.revocation : "/oauth/revoke";

    char *auth_url = NULL;
    char *token_url = NULL;
    char *userinfo_url = NULL;
    char *jwks_url = NULL;
    char *introspect_url = NULL;
    char *revoke_url = NULL;

    if (asprintf(&auth_url, "%s%s", issuer, auth_endpoint) < 0 ||
        asprintf(&token_url, "%s%s", issuer, token_endpoint) < 0 ||
        asprintf(&userinfo_url, "%s%s", issuer, userinfo_endpoint) < 0 ||
        asprintf(&jwks_url, "%s%s", issuer, jwks_endpoint) < 0 ||
        asprintf(&introspect_url, "%s%s", issuer, introspect_endpoint) < 0 ||
        asprintf(&revoke_url, "%s%s", issuer, revoke_endpoint) < 0) {
        log_this(SR_OIDC, "Failed to build endpoint URLs", LOG_LEVEL_ERROR, 0);
        free(auth_url);
        free(token_url);
        free(userinfo_url);
        free(jwks_url);
        free(introspect_url);
        free(revoke_url);
        return NULL;
    }

    /* MVP discovery: authorization_code + PKCE S256 + RS256 only (no implicit). */
    char *document = NULL;
    if (asprintf(&document,
        "{"
        "\"issuer\":\"%s\","
        "\"authorization_endpoint\":\"%s\","
        "\"token_endpoint\":\"%s\","
        "\"userinfo_endpoint\":\"%s\","
        "\"jwks_uri\":\"%s\","
        "\"introspection_endpoint\":\"%s\","
        "\"revocation_endpoint\":\"%s\","
        "\"response_types_supported\":[\"code\"],"
        "\"response_modes_supported\":[\"query\"],"
        "\"grant_types_supported\":[\"authorization_code\",\"refresh_token\"],"
        "\"subject_types_supported\":[\"public\"],"
        "\"id_token_signing_alg_values_supported\":[\"RS256\"],"
        "\"scopes_supported\":[\"openid\",\"profile\",\"email\",\"offline_access\"],"
        "\"token_endpoint_auth_methods_supported\":[\"client_secret_basic\",\"client_secret_post\",\"none\"],"
        "\"claims_supported\":[\"sub\",\"iss\",\"aud\",\"exp\",\"iat\",\"auth_time\",\"nonce\",\"name\",\"preferred_username\",\"email\",\"email_verified\"],"
        "\"code_challenge_methods_supported\":[\"S256\"]"
        "}",
        issuer, auth_url, token_url, userinfo_url, jwks_url,
        introspect_url, revoke_url) < 0) {
        log_this(SR_OIDC, "Failed to generate discovery document", LOG_LEVEL_ERROR, 0);
        free(auth_url);
        free(token_url);
        free(userinfo_url);
        free(jwks_url);
        free(introspect_url);
        free(revoke_url);
        return NULL;
    }

    free(auth_url);
    free(token_url);
    free(userinfo_url);
    free(jwks_url);
    free(introspect_url);
    free(revoke_url);

    return document;
}

/*
 * Generate JWKS document
 *
 * Creates the JSON Web Key Set document containing public keys.
 * 
 * @return JSON response or NULL on error
 */
char* oidc_generate_jwks_document(void) {
    if (!oidc_context || !oidc_context->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Log JWKS document request
    log_this(SR_OIDC, "Generating JWKS document", LOG_LEVEL_STATE, 0);

    // Get key context
    const OIDCKeyContext *key_context = (OIDCKeyContext *)oidc_context->key_context;
    if (!key_context) {
        log_this(SR_OIDC, "Key context not available", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Generate JWKS document
    return oidc_generate_jwks(key_context);
}
