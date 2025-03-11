/*
 * OpenID Connect Endpoints Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "config_oidc_endpoints.h"
#include "../types/config_string.h"

int config_oidc_endpoints_init(OIDCEndpointsConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize all endpoints with default paths
    config->authorization = strdup(DEFAULT_OIDC_AUTH_PATH);
    config->token = strdup(DEFAULT_OIDC_TOKEN_PATH);
    config->userinfo = strdup(DEFAULT_OIDC_USERINFO_PATH);
    config->jwks = strdup(DEFAULT_OIDC_JWKS_PATH);
    config->introspection = strdup(DEFAULT_OIDC_INTROSPECTION_PATH);
    config->revocation = strdup(DEFAULT_OIDC_REVOCATION_PATH);
    config->registration = strdup(DEFAULT_OIDC_REGISTRATION_PATH);

    // Check if any allocation failed
    if (!config->authorization || !config->token || !config->userinfo ||
        !config->jwks || !config->introspection || !config->revocation ||
        !config->registration) {
        config_oidc_endpoints_cleanup(config);
        return -1;
    }

    return 0;
}

void config_oidc_endpoints_cleanup(OIDCEndpointsConfig* config) {
    if (!config) {
        return;
    }

    // Free all endpoint strings
    free(config->authorization);
    free(config->token);
    free(config->userinfo);
    free(config->jwks);
    free(config->introspection);
    free(config->revocation);
    free(config->registration);

    // Zero out the structure
    memset(config, 0, sizeof(OIDCEndpointsConfig));
}

int config_oidc_endpoints_validate(const OIDCEndpointsConfig* config) {
    if (!config) {
        return -1;
    }

    // Required endpoints must be present and non-empty
    if (!config->authorization || !config->authorization[0] ||
        !config->token || !config->token[0] ||
        !config->userinfo || !config->userinfo[0] ||
        !config->jwks || !config->jwks[0]) {
        return -1;
    }

    // Optional endpoints can be NULL, but if present must not be empty
    if ((config->introspection && !config->introspection[0]) ||
        (config->revocation && !config->revocation[0]) ||
        (config->registration && !config->registration[0])) {
        return -1;
    }

    // All URLs should start with '/'
    if (config->authorization[0] != '/' ||
        config->token[0] != '/' ||
        config->userinfo[0] != '/' ||
        config->jwks[0] != '/' ||
        (config->introspection && config->introspection[0] != '/') ||
        (config->revocation && config->revocation[0] != '/') ||
        (config->registration && config->registration[0] != '/')) {
        return -1;
    }

    return 0;
}