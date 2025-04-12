/*
 * OpenID Connect (OIDC) Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "../config.h"
#include "config_oidc.h"
#include "config_oidc_endpoints.h"
#include "config_oidc_keys.h"
#include "config_oidc_tokens.h"
#include "../config_utils.h"
#include "../../logging/logging.h"

static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = strdup(str);
    if (!dup) {
        log_this("Config-OIDC", "Memory allocation failed", LOG_LEVEL_ERROR);
    }
    return dup;
}

int config_oidc_init(OIDCConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize main configuration
    config->enabled = DEFAULT_OIDC_ENABLED;
    config->issuer = NULL;
    config->client_id = NULL;
    config->client_secret = NULL;
    config->redirect_uri = NULL;
    config->port = DEFAULT_OIDC_PORT;
    config->auth_method = strdup_safe(DEFAULT_AUTH_METHOD);
    config->scope = strdup_safe("openid profile email");
    config->verify_ssl = true;

    // Initialize sub-components
    int result;
    
    result = config_oidc_endpoints_init(&config->endpoints);
    if (result != 0) {
        log_this("Config-OIDC", "Failed to initialize OIDC endpoints", LOG_LEVEL_ERROR);
        return -1;
    }

    result = config_oidc_keys_init(&config->keys);
    if (result != 0) {
        log_this("Config-OIDC", "Failed to initialize OIDC keys", LOG_LEVEL_ERROR);
        config_oidc_endpoints_cleanup(&config->endpoints);
        return -1;
    }

    result = config_oidc_tokens_init(&config->tokens);
    if (result != 0) {
        log_this("Config-OIDC", "Failed to initialize OIDC tokens", LOG_LEVEL_ERROR);
        config_oidc_endpoints_cleanup(&config->endpoints);
        config_oidc_keys_cleanup(&config->keys);
        return -1;
    }

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

    // Cleanup sub-components using their specific cleanup functions
    config_oidc_endpoints_cleanup(&config->endpoints);
    config_oidc_keys_cleanup(&config->keys);
    config_oidc_tokens_cleanup(&config->tokens);

    // Zero out the structure
    memset(config, 0, sizeof(OIDCConfig));
}

static int validate_url(const char* url, const char* field_name) {
    if (!url || strlen(url) == 0) {
        log_this("Config-OIDC", "OIDC URL validation failed for field: %s", LOG_LEVEL_ERROR, field_name);
        return -1;
    }
    // Basic URL validation - must start with http:// or https://
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        log_this("Config-OIDC", "Invalid URL format for field: %s", LOG_LEVEL_ERROR, field_name);
        return -1;
    }
    return 0;
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
        log_this("Config-OIDC", "OIDC issuer is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->client_id || strlen(config->client_id) == 0) {
        log_this("Config-OIDC", "OIDC client_id is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->client_secret || strlen(config->client_secret) == 0) {
        log_this("Config-OIDC", "OIDC client_secret is required", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate URLs
    if (validate_url(config->issuer, "issuer") != 0) return -1;
    if (config->redirect_uri && validate_url(config->redirect_uri, "redirect_uri") != 0) return -1;

    // Validate port
    if (config->port < 1024 || config->port > 65535) {
        log_this("Config-OIDC", "Invalid OIDC port", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate sub-components
    if (config_oidc_endpoints_validate(&config->endpoints) != 0) {
        log_this("Config-OIDC", "OIDC endpoints validation failed", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config_oidc_keys_validate(&config->keys) != 0) {
        log_this("Config-OIDC", "OIDC keys validation failed", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config_oidc_tokens_validate(&config->tokens) != 0) {
        log_this("Config-OIDC", "OIDC tokens validation failed", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}