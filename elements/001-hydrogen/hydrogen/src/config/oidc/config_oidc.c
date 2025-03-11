/*
 * OpenID Connect Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "config_oidc.h"
#include "../types/config_string.h"

int config_oidc_init(OIDCConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize core settings
    config->enabled = DEFAULT_OIDC_ENABLED;
    config->issuer = strdup(DEFAULT_OIDC_ISSUER);
    if (!config->issuer) {
        config_oidc_cleanup(config);
        return -1;
    }

    // Initialize all subsystems
    if (config_oidc_endpoints_init(&config->endpoints) != 0) {
        config_oidc_cleanup(config);
        return -1;
    }

    if (config_oidc_keys_init(&config->keys) != 0) {
        config_oidc_cleanup(config);
        return -1;
    }

    if (config_oidc_tokens_init(&config->tokens) != 0) {
        config_oidc_cleanup(config);
        return -1;
    }

    if (config_oidc_security_init(&config->security) != 0) {
        config_oidc_cleanup(config);
        return -1;
    }

    return 0;
}

void config_oidc_cleanup(OIDCConfig* config) {
    if (!config) {
        return;
    }

    // Free core settings
    free(config->issuer);

    // Clean up all subsystems
    config_oidc_endpoints_cleanup(&config->endpoints);
    config_oidc_keys_cleanup(&config->keys);
    config_oidc_tokens_cleanup(&config->tokens);
    config_oidc_security_cleanup(&config->security);

    // Zero out the structure
    memset(config, 0, sizeof(OIDCConfig));
}

static int validate_issuer_url(const char* issuer) {
    if (!issuer || !issuer[0]) {
        return -1;
    }

    // Must start with http:// or https://
    if (strncmp(issuer, "http://", 7) != 0 &&
        strncmp(issuer, "https://", 8) != 0) {
        return -1;
    }

    // Must not end with a slash
    size_t len = strlen(issuer);
    if (issuer[len - 1] == '/') {
        return -1;
    }

    return 0;
}

int config_oidc_validate(const OIDCConfig* config) {
    if (!config) {
        return -1;
    }

    // If OIDC is enabled, validate all settings
    if (config->enabled) {
        // Validate core settings
        if (validate_issuer_url(config->issuer) != 0) {
            return -1;
        }

        // Validate all subsystems
        if (config_oidc_endpoints_validate(&config->endpoints) != 0 ||
            config_oidc_keys_validate(&config->keys) != 0 ||
            config_oidc_tokens_validate(&config->tokens) != 0 ||
            config_oidc_security_validate(&config->security) != 0) {
            return -1;
        }

        // Cross-validate subsystem relationships
        // For example, if client credentials are allowed, certain endpoints must be configured
        if (config->security.allow_client_credentials) {
            if (!config->endpoints.token || !config->endpoints.token[0]) {
                return -1;
            }
        }

        // If PKCE is required, authorization endpoint must be configured
        if (config->security.require_pkce) {
            if (!config->endpoints.authorization || !config->endpoints.authorization[0]) {
                return -1;
            }
        }
    }

    return 0;
}