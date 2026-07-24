/*
 * OIDC token endpoint implementation
 *
 * authorization_code (with PKCE) and refresh_token grants, plus JSON builders
 * used when minting access + id tokens (and optional refresh).
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_tokens.h"
#include "oidc_auth_codes.h"
#include "oidc_refresh_tokens.h"

/*
 * Build OAuth token-endpoint error JSON.
 * @return heap JSON string (caller frees), or NULL if error is NULL / OOM
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

/*
 * True when the client's registered grant_types list includes refresh_token.
 */
bool oidc_client_allows_refresh(const OIDCClient *client) {
    if (!client || !client->grant_types[0]) {
        return false;
    }
    return strstr(client->grant_types, "refresh_token") != NULL;
}

/*
 * Decide whether to mint a refresh token for this client/scope pair.
 * Currently: any client that lists refresh_token (offline_access preferred later).
 */
bool oidc_should_issue_refresh(const OIDCClient *client, const char *scope) {
    if (!oidc_client_allows_refresh(client)) {
        return false;
    }
    if (oidc_scope_has(scope, "offline_access")) {
        return true;
    }
    /* First-party convenience: refresh_token grant registered ⇒ issue refresh. */
    return true;
}

/*
 * Assemble the successful token-endpoint JSON body.
 * refresh_token_or_null may be NULL/empty to omit the refresh_token field.
 */
char* oidc_build_token_response_json(const char *access_token,
                                     const char *id_token,
                                     int expires_in,
                                     const char *scope,
                                     const char *refresh_token_or_null) {
    char *response = NULL;
    const char *use_scope = (scope && scope[0] != '\0') ? scope : "openid";

    if (!access_token || !id_token) {
        return NULL;
    }

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

/*
 * Mint id_token + access_token for account_id and wrap them in token JSON.
 * Optionally attach an already-issued refresh plaintext.
 */
char* oidc_mint_token_pair_response(int account_id,
                                    const char *client_id,
                                    const char *scope,
                                    const char *nonce_or_null,
                                    const char *refresh_plaintext_or_null) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->token_context || account_id <= 0 || !client_id) {
        return NULL;
    }

    const char *issuer = ctx->config.issuer ? ctx->config.issuer : "";
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

    OIDCTokenContext *tok_ctx = (OIDCTokenContext*)ctx->token_context;
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

/*
 * Handle authorization_code grant: consume code + PKCE, mint tokens.
 */
char* oidc_token_handle_authorization_code(const char *code,
                                           const char *redirect_uri,
                                           const char *client_id,
                                           const char *code_verifier,
                                           const OIDCClient *client) {
    OIDCContext *ctx = get_oidc_context();
    if (!code || !redirect_uri || !code_verifier) {
        return oidc_token_error_json("invalid_request",
                                     "code, redirect_uri, and code_verifier required");
    }
    if (!ctx || !ctx->auth_code_store) {
        return oidc_token_error_json("server_error", "Token service unavailable");
    }

    OIDCAuthCodeRecord rec;
    memset(&rec, 0, sizeof(rec));
    if (!oidc_auth_code_consume(ctx->auth_code_store, code, client_id,
                                redirect_uri, code_verifier, &rec)) {
        return oidc_token_error_json("invalid_grant",
                                     "Invalid, expired, or already used authorization code");
    }

    char *refresh_plain = NULL;
    if (client && oidc_should_issue_refresh(client, rec.scope) && ctx->refresh_store) {
        refresh_plain = oidc_refresh_issue(ctx->refresh_store, client_id,
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

/*
 * Handle refresh_token grant: rotate refresh, mint new access/id pair.
 */
char* oidc_token_handle_refresh_token(const char *refresh_token,
                                      const char *client_id,
                                      const OIDCClient *client) {
    OIDCContext *ctx = get_oidc_context();
    if (!refresh_token || refresh_token[0] == '\0') {
        return oidc_token_error_json("invalid_request", "refresh_token required");
    }
    if (!client || !oidc_client_allows_refresh(client)) {
        return oidc_token_error_json("unauthorized_client",
                                     "Client not allowed to use refresh_token");
    }
    if (!ctx || !ctx->refresh_store) {
        return oidc_token_error_json("server_error", "Refresh store unavailable");
    }

    OIDCRefreshRecord rrec;
    memset(&rrec, 0, sizeof(rrec));
    char *new_refresh = NULL;
    if (!oidc_refresh_rotate(ctx->refresh_store, refresh_token, client_id,
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

/*
 * OAuth 2.0 token endpoint dispatcher.
 * Returns success JSON or OAuth error object (caller frees); never NULL on OOM-free path
 * except when asprintf fails inside helpers.
 */
char* oidc_process_token_request(const char *grant_type, const char *code,
                                const char *redirect_uri, const char *client_id,
                                const char *client_secret, const char *refresh_token,
                                const char *code_verifier) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
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

    OIDCClientContext *clients = (OIDCClientContext*)ctx->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Token request client authentication failed", LOG_LEVEL_DEBUG, 0);
        return oidc_token_error_json("invalid_client", "Client authentication failed");
    }

    OIDCClient *client = oidc_client_registry_find(clients, client_id);
    if (!ctx->token_context) {
        return oidc_token_error_json("server_error", "Token service unavailable");
    }

    if (strcmp(grant_type, "authorization_code") == 0) {
        return oidc_token_handle_authorization_code(code, redirect_uri, client_id,
                                                    code_verifier, client);
    }

    if (strcmp(grant_type, "refresh_token") == 0) {
        return oidc_token_handle_refresh_token(refresh_token, client_id, client);
    }

    return oidc_token_error_json("unsupported_grant_type", "Unsupported grant_type");
}
