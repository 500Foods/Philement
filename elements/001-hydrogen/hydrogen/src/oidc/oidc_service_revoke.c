/*
 * OIDC token revocation (RFC 7009)
 *
 * Authenticated clients may revoke refresh tokens. Access JWTs have no server
 * denylist: revocation is a no-op success until exp (prefer short TTL + refresh kill).
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_tokens.h"
#include "oidc_refresh_tokens.h"

/*
 * Attempt to revoke a refresh token belonging to client_id.
 * @return true if the token was found in the refresh store (revoked or wrong client)
 */
bool oidc_revoke_try_refresh(OIDCContext *ctx, const char *token, const char *client_id) {
    if (!ctx || !ctx->refresh_store || !token || !client_id) {
        return false;
    }
    OIDCRefreshRecord rec;
    memset(&rec, 0, sizeof(rec));
    if (!oidc_refresh_lookup(ctx->refresh_store, token, &rec)) {
        return false;
    }
    if (strcmp(rec.client_id, client_id) == 0) {
        (void)oidc_refresh_revoke(ctx->refresh_store, token);
    }
    /* Known refresh handled (or wrong client — still 200, no leak). */
    return true;
}

/*
 * Validate access JWT presence for revocation path (no denylist).
 * @return true if token validates as an access token
 */
bool oidc_revoke_try_access(const OIDCContext *ctx, const char *token) {
    if (!ctx || !ctx->token_context || !token) {
        return false;
    }
    OIDCTokenClaims *claims = NULL;
    if (!oidc_validate_access_token((OIDCTokenContext*)ctx->token_context, token, &claims)) {
        return false;
    }
    oidc_free_token_claims(claims);
    return true;
}

/*
 * Process a token revocation request (RFC 7009).
 * Returns true when the request was accepted (including unknown tokens).
 * Returns false only when service is down or client authentication fails.
 */
bool oidc_process_revocation_request(const char *token, const char *token_type_hint,
                                    const char *client_id, const char *client_secret) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!client_id || client_id[0] == '\0') {
        return false;
    }

    OIDCClientContext *clients = (OIDCClientContext*)ctx->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Revocation client authentication failed", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    if (!token || token[0] == '\0') {
        /* RFC 7009: invalid/missing token still yields success after auth. */
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

    if (try_refresh && oidc_revoke_try_refresh(ctx, token, client_id)) {
        return true;
    }

    if (try_access && oidc_revoke_try_access(ctx, token)) {
        /* Access JWTs remain valid until exp; success acknowledges the request. */
        return true;
    }

    /* Unknown token: still success per RFC 7009. */
    return true;
}
