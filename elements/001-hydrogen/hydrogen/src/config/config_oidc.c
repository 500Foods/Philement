/*
 * OpenID Connect (OIDC) Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "config_oidc.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Helper function for URL validation
static int validate_url(const char* url, const char* field_name) {
    if (!url || strlen(url) == 0) {
        log_this("Config", "OIDC URL validation failed for field: %s", LOG_LEVEL_ERROR, field_name);
        return -1;
    }
    // Basic URL validation - must start with http:// or https://
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        log_this("Config", "Invalid URL format for field: %s", LOG_LEVEL_ERROR, field_name);
        return -1;
    }
    return 0;
}

bool load_oidc_config(json_t* root, AppConfig* config) {
    if (!root || !config) {
        log_this("Config", "Invalid parameters for OIDC configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize with defaults
    if (config_oidc_init(&config->oidc) != 0) {
        log_this("Config", "Failed to initialize OIDC configuration", LOG_LEVEL_ERROR);
        return false;
    }

    bool success = true;
    OIDCConfig* oidc = &config->oidc;
    PROCESS_SECTION(root, "OIDC"); {
        // Core settings
        PROCESS_BOOL(root, oidc, enabled, "enabled", "OIDC");
        PROCESS_STRING(root, oidc, issuer, "issuer", "OIDC");
        PROCESS_STRING(root, oidc, client_id, "client_id", "OIDC");
        PROCESS_SENSITIVE(root, oidc, client_secret, "client_secret", "OIDC");
        PROCESS_STRING(root, oidc, redirect_uri, "redirect_uri", "OIDC");
        PROCESS_INT(root, oidc, port, "port", "OIDC");
        PROCESS_STRING(root, oidc, auth_method, "auth_method", "OIDC");
        PROCESS_STRING(root, oidc, scope, "scope", "OIDC");
        PROCESS_BOOL(root, oidc, verify_ssl, "verify_ssl", "OIDC");

        // Endpoints configuration
        PROCESS_STRING(root, &oidc->endpoints, authorization, "authorization_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, token, "token_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, userinfo, "userinfo_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, jwks, "jwks_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, end_session, "end_session_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, introspection, "introspection_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, revocation, "revocation_endpoint", "OIDC");
        PROCESS_STRING(root, &oidc->endpoints, registration, "registration_endpoint", "OIDC");

        // Keys configuration
        PROCESS_SENSITIVE(root, &oidc->keys, signing_key, "signing_key", "OIDC");
        PROCESS_SENSITIVE(root, &oidc->keys, encryption_key, "encryption_key", "OIDC");
        PROCESS_STRING(root, &oidc->keys, jwks_uri, "jwks_uri", "OIDC");
        PROCESS_STRING(root, &oidc->keys, storage_path, "key_storage_path", "OIDC");
        PROCESS_BOOL(root, &oidc->keys, encryption_enabled, "encryption_enabled", "OIDC");
        PROCESS_INT(root, &oidc->keys, rotation_interval_days, "key_rotation_interval_days", "OIDC");

        // Tokens configuration
        PROCESS_INT(root, &oidc->tokens, access_token_lifetime, "access_token_lifetime", "OIDC");
        PROCESS_INT(root, &oidc->tokens, refresh_token_lifetime, "refresh_token_lifetime", "OIDC");
        PROCESS_INT(root, &oidc->tokens, id_token_lifetime, "id_token_lifetime", "OIDC");
        PROCESS_STRING(root, &oidc->tokens, signing_alg, "token_signing_alg", "OIDC");
        PROCESS_STRING(root, &oidc->tokens, encryption_alg, "token_encryption_alg", "OIDC");

        // Buffer for number conversion
        char buffer[32];

        // Log configuration (masking sensitive values)
        log_config_item("Enabled", oidc->enabled ? "true" : "false", false);
        log_config_item("Issuer", oidc->issuer, false);
        log_config_item("Client ID", oidc->client_id, false);
        log_config_item("Client Secret", "********", false);
        log_config_item("Redirect URI", oidc->redirect_uri, false);
        snprintf(buffer, sizeof(buffer), "%d", oidc->port);
        log_config_item("Port", buffer, false);
        log_config_item("Auth Method", oidc->auth_method, false);
        log_config_item("Scope", oidc->scope, false);
        log_config_item("Verify SSL", oidc->verify_ssl ? "true" : "false", false);

        // Validate configuration
        if (config_oidc_validate(&config->oidc) != 0) {
            log_this("Config", "Invalid OIDC configuration", LOG_LEVEL_ERROR);
            success = false;
        }
    }

    if (!success) {
        config_oidc_cleanup(&config->oidc);
    }

    return success;
}

int config_oidc_init(OIDCConfig* config) {
    if (!config) {
        return -1;
    }

    memset(config, 0, sizeof(OIDCConfig));

    // Initialize main configuration
    config->enabled = DEFAULT_OIDC_ENABLED;
    config->port = DEFAULT_OIDC_PORT;
    config->auth_method = strdup(DEFAULT_AUTH_METHOD);
    config->scope = strdup("openid profile email");
    config->verify_ssl = true;

    if (!config->auth_method || !config->scope) {
        config_oidc_cleanup(config);
        return -1;
    }

    // Initialize tokens with defaults
    config->tokens.access_token_lifetime = DEFAULT_TOKEN_EXPIRY;
    config->tokens.refresh_token_lifetime = DEFAULT_REFRESH_EXPIRY;
    config->tokens.id_token_lifetime = DEFAULT_TOKEN_EXPIRY;

    return 0;
}

void config_oidc_cleanup(OIDCConfig* config) {
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

int config_oidc_validate(const OIDCConfig* config) {
    if (!config) {
        return -1;
    }

    // Skip validation if OIDC is disabled
    if (!config->enabled) {
        return 0;
    }

    // Validate required fields
    if (!config->issuer || strlen(config->issuer) == 0) {
        log_this("Config", "OIDC issuer is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->client_id || strlen(config->client_id) == 0) {
        log_this("Config", "OIDC client_id is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->client_secret || strlen(config->client_secret) == 0) {
        log_this("Config", "OIDC client_secret is required", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate URLs
    if (validate_url(config->issuer, "issuer") != 0) return -1;
    if (config->redirect_uri && validate_url(config->redirect_uri, "redirect_uri") != 0) return -1;

    // Validate port
    if (config->port < 1024 || config->port > 65535) {
        log_this("Config", "Invalid OIDC port", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate token lifetimes
    if (config->tokens.access_token_lifetime <= 0) {
        log_this("Config", "Invalid access token lifetime", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->tokens.refresh_token_lifetime <= 0) {
        log_this("Config", "Invalid refresh token lifetime", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->tokens.id_token_lifetime <= 0) {
        log_this("Config", "Invalid ID token lifetime", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}