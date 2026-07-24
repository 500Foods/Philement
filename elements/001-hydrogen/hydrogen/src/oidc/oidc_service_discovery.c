/*
 * OIDC discovery and JWKS document generation
 *
 * OpenID Provider Metadata (/.well-known/openid-configuration) and JWKS export.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_keys.h"

/*
 * Join issuer + path into an absolute endpoint URL.
 * path may be absolute path ("/oauth/token") or already absolute URL.
 * @return heap string (caller frees), or NULL on OOM / bad args
 */
char* oidc_discovery_join_url(const char *issuer, const char *path) {
    if (!path) {
        return NULL;
    }
    const char *iss = issuer ? issuer : "";
    char *url = NULL;
    if (asprintf(&url, "%s%s", iss, path) < 0) {
        return NULL;
    }
    return url;
}

/*
 * Resolve configured endpoint path or a default relative path.
 */
const char* oidc_discovery_endpoint_or_default(const char *configured, const char *fallback) {
    if (configured && configured[0] != '\0') {
        return configured;
    }
    return fallback;
}

/*
 * Generate OpenID Provider configuration document (caller frees).
 * MVP: authorization_code + PKCE S256 + RS256 only (no implicit).
 */
char* oidc_generate_discovery_document(void) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_OIDC, "Generating discovery document", LOG_LEVEL_STATE, 0);

    const char *issuer = ctx->config.issuer ? ctx->config.issuer : "";
    const char *auth_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.authorization, "/oauth/authorize");
    const char *token_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.token, "/oauth/token");
    const char *userinfo_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.userinfo, "/oauth/userinfo");
    const char *jwks_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.jwks, "/oauth/jwks");
    const char *introspect_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.introspection, "/oauth/introspect");
    const char *revoke_endpoint = oidc_discovery_endpoint_or_default(
        ctx->config.endpoints.revocation, "/oauth/revoke");

    char *auth_url = oidc_discovery_join_url(issuer, auth_endpoint);
    char *token_url = oidc_discovery_join_url(issuer, token_endpoint);
    char *userinfo_url = oidc_discovery_join_url(issuer, userinfo_endpoint);
    char *jwks_url = oidc_discovery_join_url(issuer, jwks_endpoint);
    char *introspect_url = oidc_discovery_join_url(issuer, introspect_endpoint);
    char *revoke_url = oidc_discovery_join_url(issuer, revoke_endpoint);

    if (!auth_url || !token_url || !userinfo_url || !jwks_url ||
        !introspect_url || !revoke_url) {
        log_this(SR_OIDC, "Failed to build endpoint URLs", LOG_LEVEL_ERROR, 0);
        free(auth_url);
        free(token_url);
        free(userinfo_url);
        free(jwks_url);
        free(introspect_url);
        free(revoke_url);
        return NULL;
    }

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
 * Generate JWKS document from the active signing key set (caller frees).
 */
char* oidc_generate_jwks_document(void) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_OIDC, "Generating JWKS document", LOG_LEVEL_STATE, 0);

    const OIDCKeyContext *key_context = (OIDCKeyContext *)ctx->key_context;
    if (!key_context) {
        log_this(SR_OIDC, "Key context not available", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return oidc_generate_jwks(key_context);
}
