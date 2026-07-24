/*
 * OIDC authorization path helpers
 *
 * Resource-owner authentication, accounts DB resolution, and authorization
 * code issuance (authorization_code + PKCE S256).
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_pkce.h"
#include <src/api/auth/auth_service.h>

/*
 * Resolve the database name used for IdP resource-owner lookups.
 *
 * Preference order:
 *  1. Context copy of config.database (set at init)
 *  2. Live app_config->oidc.database
 *  3. First databases.connections[] name
 *  4. Literal "demo"
 */
const char* oidc_get_accounts_database(void) {
    OIDCContext *ctx = get_oidc_context();
    if (ctx && ctx->database_name && ctx->database_name[0] != '\0') {
        return ctx->database_name;
    }
    if (app_config && app_config->oidc.database && app_config->oidc.database[0] != '\0') {
        return app_config->oidc.database;
    }
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

/*
 * Authenticate a resource owner via accounts + QueryRef password path.
 * On success writes positive account_id_out.
 */
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

/*
 * Issue a one-time authorization code after client + resource-owner checks.
 *
 * Requires PKCE S256 (code_challenge + method). Caller frees the returned
 * plaintext code. error_code receives a short OAuth error token on failure.
 */
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

    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized || !ctx->auth_code_store) {
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

    OIDCClientContext *clients = (OIDCClientContext*)ctx->client_context;
    if (!oidc_validate_client(clients, client_id, redirect_uri)) {
        if (error_code) {
            *error_code = "unauthorized_client";
        }
        return NULL;
    }

    /* S256 is mandatory for all clients; require_pkce is covered by the gate above. */
    const char *use_scope = (scope && scope[0] != '\0') ? scope : "openid";
    char *code = oidc_auth_code_issue(ctx->auth_code_store,
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

/*
 * Legacy authorize entry without an authenticated account_id.
 * Always returns NULL; real issuance uses oidc_issue_authorization_code after login.
 */
char* oidc_process_authorization_request(const char *client_id, const char *redirect_uri,
                                        const char *response_type, const char *scope,
                                        const char *state, const char *nonce,
                                        const char *code_challenge, const char *code_challenge_method) {
    (void)state;
    (void)client_id;
    (void)redirect_uri;
    (void)scope;
    (void)nonce;
    (void)code_challenge;
    (void)code_challenge_method;

    if (!response_type || strcmp(response_type, "code") != 0) {
        return NULL;
    }
    log_this(SR_OIDC, "Authorization requires resource-owner login (POST)", LOG_LEVEL_DEBUG, 0);
    return NULL;
}
