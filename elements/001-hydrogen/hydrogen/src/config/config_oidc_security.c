/*
 * OpenID Connect Security Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_oidc_security.h"

int config_oidc_security_init(OIDCSecurityConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with secure default values
    config->require_pkce = DEFAULT_REQUIRE_PKCE;
    config->allow_implicit_flow = DEFAULT_ALLOW_IMPLICIT_FLOW;
    config->allow_client_credentials = DEFAULT_ALLOW_CLIENT_CREDENTIALS;
    config->require_consent = DEFAULT_REQUIRE_CONSENT;

    return 0;
}

void config_oidc_security_cleanup(OIDCSecurityConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(OIDCSecurityConfig));
}

static int is_valid_bool(int value) {
    return value == 0 || value == 1;
}

int config_oidc_security_validate(const OIDCSecurityConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate all boolean flags have valid values
    if (!is_valid_bool(config->require_pkce) ||
        !is_valid_bool(config->allow_implicit_flow) ||
        !is_valid_bool(config->allow_client_credentials) ||
        !is_valid_bool(config->require_consent)) {
        return -1;
    }

    // Security policy validations
    
    // If implicit flow is allowed, PKCE must be required
    if (config->allow_implicit_flow && !config->require_pkce) {
        return -1;
    }

    // If client credentials are allowed, consent requirement must be off
    // (as there's no user interaction in client credentials flow)
    if (config->allow_client_credentials && config->require_consent) {
        return -1;
    }

    return 0;
}