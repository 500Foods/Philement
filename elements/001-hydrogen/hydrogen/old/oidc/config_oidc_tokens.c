/*
 * OpenID Connect Tokens Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "../config.h"
#include "config_oidc.h"
#include "config_oidc_tokens.h"

int config_oidc_tokens_init(OIDCTokensConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default values
    config->access_token_lifetime = DEFAULT_ACCESS_TOKEN_LIFETIME;
    config->refresh_token_lifetime = DEFAULT_REFRESH_TOKEN_LIFETIME;
    config->id_token_lifetime = DEFAULT_ID_TOKEN_LIFETIME;

    return 0;
}

void config_oidc_tokens_cleanup(OIDCTokensConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(OIDCTokensConfig));
}

int config_oidc_tokens_validate(const OIDCTokensConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate access token lifetime
    if (config->access_token_lifetime <= 0 ||
        config->access_token_lifetime > MAX_ACCESS_TOKEN_LIFETIME) {
        return -1;
    }

    // Validate refresh token lifetime
    if (config->refresh_token_lifetime <= 0 ||
        config->refresh_token_lifetime > MAX_REFRESH_TOKEN_LIFETIME) {
        return -1;
    }

    // Validate ID token lifetime
    if (config->id_token_lifetime <= 0 ||
        config->id_token_lifetime > MAX_ID_TOKEN_LIFETIME) {
        return -1;
    }

    // Validate token lifetime relationships
    // Refresh token lifetime must be longer than access token lifetime
    if (config->refresh_token_lifetime <= config->access_token_lifetime) {
        return -1;
    }

    // ID token lifetime should not exceed access token lifetime
    if (config->id_token_lifetime > config->access_token_lifetime) {
        return -1;
    }

    return 0;
}