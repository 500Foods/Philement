/*
 * OIDC configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "json_oidc.h"
#include "../config.h"
#include "../config_utils.h"
#include "../oidc/config_oidc.h"
#include "../../logging/logging.h"
#include "../logging/config_logging_utils.h"

bool load_json_oidc(json_t* root, AppConfig* config) {
    // OIDC Configuration
    json_t* oidc = json_object_get(root, "OIDC");
    if (json_is_object(oidc)) {
        log_config_section_header("OIDC");
        
        // Enabled flag
        json_t* enabled = json_object_get(oidc, "Enabled");
        config->oidc.enabled = get_config_bool(enabled, DEFAULT_OIDC_ENABLED);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                config->oidc.enabled ? "true" : "false");

        // Port
        json_t* port = json_object_get(oidc, "Port");
        config->oidc.port = get_config_int(port, DEFAULT_OIDC_PORT);
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, "Config",
                config->oidc.port);

        // Core settings
        json_t* val;
        val = json_object_get(oidc, "IssuerURL");
        config->oidc.issuer = get_config_string_with_env("IssuerURL", val, "${env.OIDC_ISSUER_URL}");
        log_config_section_item("IssuerURL", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                config->oidc.issuer);

        val = json_object_get(oidc, "ClientID");
        config->oidc.client_id = get_config_string_with_env("ClientID", val, "${env.OIDC_CLIENT_ID}");
        log_config_section_item("ClientID", "configured", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config");

        val = json_object_get(oidc, "ClientSecret");
        config->oidc.client_secret = get_config_string_with_env("ClientSecret", val, "${env.OIDC_CLIENT_SECRET}");
        log_config_section_item("ClientSecret", "configured", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config");

        val = json_object_get(oidc, "RedirectURI");
        config->oidc.redirect_uri = get_config_string_with_env("RedirectURI", val, NULL);
        if (config->oidc.redirect_uri) {
            log_config_section_item("RedirectURI", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.redirect_uri);
        }

        // Authentication settings
        val = json_object_get(oidc, "AuthMethod");
        config->oidc.auth_method = get_config_string_with_env("AuthMethod", val, DEFAULT_AUTH_METHOD);
        log_config_section_item("AuthMethod", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                config->oidc.auth_method);

        val = json_object_get(oidc, "Scope");
        config->oidc.scope = get_config_string_with_env("Scope", val, "openid profile email");
        log_config_section_item("Scope", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                config->oidc.scope);

        // Token settings
        json_t* token_expiry = json_object_get(oidc, "TokenExpiry");
        config->oidc.tokens.access_token_lifetime = get_config_int(token_expiry, DEFAULT_TOKEN_EXPIRY);
        log_config_section_item("TokenExpiry", "%d", LOG_LEVEL_STATE, !token_expiry, 1, "s", "s", "Config",
                config->oidc.tokens.access_token_lifetime);

        json_t* refresh_expiry = json_object_get(oidc, "RefreshExpiry");
        config->oidc.tokens.refresh_token_lifetime = get_config_int(refresh_expiry, DEFAULT_REFRESH_EXPIRY);
        log_config_section_item("RefreshExpiry", "%d", LOG_LEVEL_STATE, !refresh_expiry, 1, "s", "s", "Config",
                config->oidc.tokens.refresh_token_lifetime);

        // Set ID token lifetime same as access token by default
        config->oidc.tokens.id_token_lifetime = config->oidc.tokens.access_token_lifetime;

        // SSL verification
        json_t* verify_ssl = json_object_get(oidc, "VerifySSL");
        config->oidc.verify_ssl = get_config_bool(verify_ssl, true);
        log_config_section_item("VerifySSL", "%s", LOG_LEVEL_STATE, !verify_ssl, 1, NULL, NULL, "Config",
                config->oidc.verify_ssl ? "true" : "false");

        // JWKS settings
        val = json_object_get(oidc, "JWKSURI");
        config->oidc.keys.jwks_uri = get_config_string_with_env("JWKSURI", val, NULL);
        if (config->oidc.keys.jwks_uri) {
            log_config_section_item("JWKSURI", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.keys.jwks_uri);
        }

        // Endpoint settings
        val = json_object_get(oidc, "AuthEndpoint");
        config->oidc.endpoints.authorization = get_config_string_with_env("AuthEndpoint", val, NULL);
        if (config->oidc.endpoints.authorization) {
            log_config_section_item("AuthEndpoint", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.endpoints.authorization);
        }

        val = json_object_get(oidc, "TokenEndpoint");
        config->oidc.endpoints.token = get_config_string_with_env("TokenEndpoint", val, NULL);
        if (config->oidc.endpoints.token) {
            log_config_section_item("TokenEndpoint", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.endpoints.token);
        }

        val = json_object_get(oidc, "UserInfoEndpoint");
        config->oidc.endpoints.userinfo = get_config_string_with_env("UserInfoEndpoint", val, NULL);
        if (config->oidc.endpoints.userinfo) {
            log_config_section_item("UserInfoEndpoint", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.endpoints.userinfo);
        }

        val = json_object_get(oidc, "EndSessionEndpoint");
        config->oidc.endpoints.end_session = get_config_string_with_env("EndSessionEndpoint", val, NULL);
        if (config->oidc.endpoints.end_session) {
            log_config_section_item("EndSessionEndpoint", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.endpoints.end_session);
        }

        // Validate configuration
        if (config_oidc_validate(&config->oidc) != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            config_oidc_init(&config->oidc);
        }

        return true;
    } else {
        // Set defaults if no OIDC configuration is provided
        config_oidc_init(&config->oidc);
        
        log_config_section_header("OIDC");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        return true;
    }
}