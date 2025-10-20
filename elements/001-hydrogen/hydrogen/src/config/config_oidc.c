/*
 * OpenID Connect (OIDC) Configuration Implementation
 *
 * Implements configuration loading and management for OIDC integration.
 * All validation has been moved to launch readiness checks.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_oidc.h"

// Helper function to construct endpoint URL
char* construct_endpoint_path(const char* base_path) {
    if (!base_path) return NULL;
    char* path = strdup(base_path);
    if (!path) return NULL;
    // Ensure path starts with '/'
    if (path[0] != '/') {
        char* new_path = malloc(strlen(path) + 2);
        if (!new_path) {
            free(path);
            return NULL;
        }
        sprintf(new_path, "/%s", path);
        free(path);
        path = new_path;
    }
    return path;
}

bool load_oidc_config(json_t* root, AppConfig* config) {
    bool success = true;
    OIDCConfig* oidc = &config->oidc;

    // Zero out the config structure
    memset(oidc, 0, sizeof(OIDCConfig));

    // Initialize core settings with secure defaults
    oidc->enabled = true;
    oidc->port = 8443;
    oidc->auth_method = strdup("client_secret_basic");
    oidc->scope = strdup("openid profile email");
    oidc->verify_ssl = true;

    // Initialize endpoints with default paths
    oidc->endpoints.authorization = construct_endpoint_path("/authorize");
    oidc->endpoints.token = construct_endpoint_path("/token");
    oidc->endpoints.userinfo = construct_endpoint_path("/userinfo");
    oidc->endpoints.jwks = construct_endpoint_path("/jwks");
    oidc->endpoints.end_session = construct_endpoint_path("/end_session");
    oidc->endpoints.introspection = construct_endpoint_path("/introspect");
    oidc->endpoints.revocation = construct_endpoint_path("/revoke");
    oidc->endpoints.registration = construct_endpoint_path("/register");

    // Initialize tokens with secure defaults
    oidc->tokens.access_token_lifetime = 3600;      // 1 hour
    oidc->tokens.refresh_token_lifetime = 86400;    // 24 hours
    oidc->tokens.id_token_lifetime = 3600;         // 1 hour
    oidc->tokens.signing_alg = strdup("RS256");    // RSA with SHA-256
    oidc->tokens.encryption_alg = strdup("A256GCM"); // AES-256 GCM

    // Initialize keys with secure defaults
    oidc->keys.encryption_enabled = true;
    oidc->keys.rotation_interval_days = 30;        // 30 days default
    oidc->keys.storage_path = strdup("/var/lib/hydrogen/keys");

    // Process main section
    success = PROCESS_SECTION(root, "OIDC");

    // Process core settings
    if (success) {
        success = success && PROCESS_BOOL(root, oidc, enabled, "OIDC.Enabled", "OIDC");
        success = success && PROCESS_STRING(root, oidc, issuer, "OIDC.Issuer", "OIDC");
        success = success && PROCESS_STRING(root, oidc, client_id, "OIDC.ClientId", "OIDC");
        success = success && PROCESS_SENSITIVE(root, oidc, client_secret, "OIDC.ClientSecret", "OIDC");
        success = success && PROCESS_STRING(root, oidc, redirect_uri, "OIDC.RedirectUri", "OIDC");
        success = success && PROCESS_INT(root, oidc, port, "OIDC.Port", "OIDC");
        success = success && PROCESS_STRING(root, oidc, auth_method, "OIDC.AuthMethod", "OIDC");
        success = success && PROCESS_STRING(root, oidc, scope, "OIDC.Scope", "OIDC");
        success = success && PROCESS_BOOL(root, oidc, verify_ssl, "OIDC.VerifySSL", "OIDC");
    }

    // Process endpoints section
    if (success) {
        success = PROCESS_SECTION(root, "OIDC.Endpoints");
        success = success && PROCESS_STRING(root, &oidc->endpoints, authorization,
                                          "OIDC.Endpoints.Authorization", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, token,
                                          "OIDC.Endpoints.Token", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, userinfo,
                                          "OIDC.Endpoints.UserInfo", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, jwks,
                                          "OIDC.Endpoints.JWKS", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, end_session,
                                          "OIDC.Endpoints.EndSession", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, introspection,
                                          "OIDC.Endpoints.Introspection", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, revocation,
                                          "OIDC.Endpoints.Revocation", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->endpoints, registration,
                                          "OIDC.Endpoints.Registration", "OIDC");
    }

    // Process keys section
    if (success) {
        success = PROCESS_SECTION(root, "OIDC.Keys");
        success = success && PROCESS_SENSITIVE(root, &oidc->keys, signing_key,
                                             "OIDC.Keys.SigningKey", "OIDC");
        success = success && PROCESS_SENSITIVE(root, &oidc->keys, encryption_key,
                                             "OIDC.Keys.EncryptionKey", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->keys, jwks_uri,
                                          "OIDC.Keys.JWKSUri", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->keys, storage_path,
                                          "OIDC.Keys.StoragePath", "OIDC");
        success = success && PROCESS_BOOL(root, &oidc->keys, encryption_enabled,
                                        "OIDC.Keys.EncryptionEnabled", "OIDC");
        success = success && PROCESS_INT(root, &oidc->keys, rotation_interval_days,
                                       "OIDC.Keys.RotationIntervalDays", "OIDC");
    }

    // Process tokens section
    if (success) {
        success = PROCESS_SECTION(root, "OIDC.Tokens");
        success = success && PROCESS_INT(root, &oidc->tokens, access_token_lifetime,
                                       "OIDC.Tokens.AccessTokenLifetime", "OIDC");
        success = success && PROCESS_INT(root, &oidc->tokens, refresh_token_lifetime,
                                       "OIDC.Tokens.RefreshTokenLifetime", "OIDC");
        success = success && PROCESS_INT(root, &oidc->tokens, id_token_lifetime,
                                       "OIDC.Tokens.IdTokenLifetime", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->tokens, signing_alg,
                                          "OIDC.Tokens.SigningAlg", "OIDC");
        success = success && PROCESS_STRING(root, &oidc->tokens, encryption_alg,
                                          "OIDC.Tokens.EncryptionAlg", "OIDC");
    }

    if (!success) {
        cleanup_oidc_config(oidc);
    }

    return success;
}

void cleanup_oidc_config(OIDCConfig* config) {
    if (!config) {
        return;
    }

    // Cleanup main configuration strings
    free(config->issuer);
    free(config->client_id);
    free(config->client_secret);
    free(config->redirect_uri);
    free(config->auth_method);
    free(config->scope);

    // Cleanup endpoints
    free(config->endpoints.authorization);
    free(config->endpoints.token);
    free(config->endpoints.userinfo);
    free(config->endpoints.jwks);
    free(config->endpoints.end_session);
    free(config->endpoints.introspection);
    free(config->endpoints.revocation);
    free(config->endpoints.registration);

    // Cleanup keys
    free(config->keys.signing_key);
    free(config->keys.encryption_key);
    free(config->keys.jwks_uri);
    free(config->keys.storage_path);

    // Cleanup tokens
    free(config->tokens.signing_alg);
    free(config->tokens.encryption_alg);

    // Zero out the structure
    memset(config, 0, sizeof(OIDCConfig));
}

void dump_oidc_config(const OIDCConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL OIDC config");
        return;
    }

    // Core settings
    DUMP_TEXT("――", "Core Settings");
    DUMP_BOOL("――――Enabled", config->enabled);
    DUMP_STRING("――――Issuer", config->issuer);
    DUMP_STRING("――――Client ID", config->client_id);
    DUMP_SECRET("――――Client Secret", config->client_secret);
    DUMP_STRING("――――Redirect URI", config->redirect_uri);
    DUMP_INT("――――Port", config->port);
    DUMP_STRING("――――Auth Method", config->auth_method);
    DUMP_STRING("――――Scope", config->scope);
    DUMP_BOOL("――――Verify SSL", config->verify_ssl);

    // Endpoints
    DUMP_TEXT("――", "Endpoints");
    DUMP_STRING("――――Authorization", config->endpoints.authorization);
    DUMP_STRING("――――Token", config->endpoints.token);
    DUMP_STRING("――――UserInfo", config->endpoints.userinfo);
    DUMP_STRING("――――JWKS", config->endpoints.jwks);
    DUMP_STRING("――――End Session", config->endpoints.end_session);
    DUMP_STRING("――――Introspection", config->endpoints.introspection);
    DUMP_STRING("――――Revocation", config->endpoints.revocation);
    DUMP_STRING("――――Registration", config->endpoints.registration);

    // Keys
    DUMP_TEXT("――", "Keys");
    DUMP_SECRET("――――Signing Key", config->keys.signing_key);
    DUMP_SECRET("――――Encryption Key", config->keys.encryption_key);
    DUMP_STRING("――――JWKS URI", config->keys.jwks_uri);
    DUMP_STRING("――――Storage Path", config->keys.storage_path);
    DUMP_BOOL("――――Encryption Enabled", config->keys.encryption_enabled);
    DUMP_INT("――――Rotation Interval (days)", config->keys.rotation_interval_days);

    // Tokens
    DUMP_TEXT("――", "Tokens");
    DUMP_INT("――――Access Token Lifetime", config->tokens.access_token_lifetime);
    DUMP_INT("――――Refresh Token Lifetime", config->tokens.refresh_token_lifetime);
    DUMP_INT("――――ID Token Lifetime", config->tokens.id_token_lifetime);
    DUMP_STRING("――――Signing Algorithm", config->tokens.signing_alg);
    DUMP_STRING("――――Encryption Algorithm", config->tokens.encryption_alg);
}
