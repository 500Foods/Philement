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

bool load_json_oidc(json_t* root, AppConfig* config) {
    // OIDC Configuration
    json_t* oidc = json_object_get(root, "OIDC");
    bool using_defaults = !json_is_object(oidc);
    
    log_config_section("OIDC", using_defaults);
    
    if (!using_defaults) {
        // Enabled flag
        json_t* enabled = json_object_get(oidc, "Enabled");
        config->oidc.enabled = get_config_bool(enabled, DEFAULT_OIDC_ENABLED);
        log_config_item("Enabled", config->oidc.enabled ? "true" : "false", !enabled, 0);

        // Port
        json_t* port = json_object_get(oidc, "Port");
        config->oidc.port = get_config_int(port, DEFAULT_OIDC_PORT);
        log_config_item("Port", format_int_buffer(config->oidc.port), !port, 0);

        // Core settings
        json_t* val;
        val = json_object_get(oidc, "IssuerURL");
        config->oidc.issuer = get_config_string_with_env("IssuerURL", val, "${env.OIDC_ISSUER_URL}");
        log_config_item("IssuerURL", config->oidc.issuer, !val, 1);

        val = json_object_get(oidc, "ClientID");
        config->oidc.client_id = get_config_string_with_env("ClientID", val, "${env.OIDC_CLIENT_ID}");
        log_config_sensitive_item("ClientID", config->oidc.client_id, !val, 1);

        val = json_object_get(oidc, "ClientSecret");
        config->oidc.client_secret = get_config_string_with_env("ClientSecret", val, "${env.OIDC_CLIENT_SECRET}");
        log_config_sensitive_item("ClientSecret", config->oidc.client_secret, !val, 1);

        val = json_object_get(oidc, "RedirectURI");
        config->oidc.redirect_uri = get_config_string_with_env("RedirectURI", val, NULL);
        if (config->oidc.redirect_uri) {
            log_config_section_item("RedirectURI", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->oidc.redirect_uri);
        }

        // Authentication settings
        val = json_object_get(oidc, "AuthMethod");
        config->oidc.auth_method = get_config_string_with_env("AuthMethod", val, DEFAULT_AUTH_METHOD);
        log_config_item("AuthMethod", config->oidc.auth_method, !val, 1);

        val = json_object_get(oidc, "Scope");
        config->oidc.scope = get_config_string_with_env("Scope", val, "openid profile email");
        log_config_item("Scope", config->oidc.scope, !val, 1);

        // Token settings
        json_t* token_expiry = json_object_get(oidc, "TokenExpiry");
        config->oidc.tokens.access_token_lifetime = get_config_int(token_expiry, DEFAULT_TOKEN_EXPIRY);
        char token_buffer[64];
        snprintf(token_buffer, sizeof(token_buffer), "%ss", format_int_buffer(config->oidc.tokens.access_token_lifetime));
        log_config_item("TokenExpiry", token_buffer, !token_expiry, 1);

        json_t* refresh_expiry = json_object_get(oidc, "RefreshExpiry");
        config->oidc.tokens.refresh_token_lifetime = get_config_int(refresh_expiry, DEFAULT_REFRESH_EXPIRY);
        char refresh_buffer[64];
        snprintf(refresh_buffer, sizeof(refresh_buffer), "%ss", format_int_buffer(config->oidc.tokens.refresh_token_lifetime));
        log_config_item("RefreshExpiry", refresh_buffer, !refresh_expiry, 1);

        // Set ID token lifetime same as access token by default
        config->oidc.tokens.id_token_lifetime = config->oidc.tokens.access_token_lifetime;

        // SSL verification
        json_t* verify_ssl = json_object_get(oidc, "VerifySSL");
        config->oidc.verify_ssl = get_config_bool(verify_ssl, true);
        log_config_item("VerifySSL", config->oidc.verify_ssl ? "true" : "false", !verify_ssl, 1);

        // JWKS settings
        val = json_object_get(oidc, "JWKSURI");
        config->oidc.keys.jwks_uri = get_config_string_with_env("JWKSURI", val, NULL);
        if (config->oidc.keys.jwks_uri) {
            log_config_item("JWKSURI", config->oidc.keys.jwks_uri, !val, 1);
        }

        // Endpoint settings
        val = json_object_get(oidc, "AuthEndpoint");
        config->oidc.endpoints.authorization = get_config_string_with_env("AuthEndpoint", val, NULL);
        if (config->oidc.endpoints.authorization) {
            log_config_item("AuthEndpoint", config->oidc.endpoints.authorization, !val, 1);
        }

        val = json_object_get(oidc, "TokenEndpoint");
        config->oidc.endpoints.token = get_config_string_with_env("TokenEndpoint", val, NULL);
        if (config->oidc.endpoints.token) {
            log_config_item("TokenEndpoint", config->oidc.endpoints.token, !val, 1);
        }

        val = json_object_get(oidc, "UserInfoEndpoint");
        config->oidc.endpoints.userinfo = get_config_string_with_env("UserInfoEndpoint", val, NULL);
        if (config->oidc.endpoints.userinfo) {
            log_config_item("UserInfoEndpoint", config->oidc.endpoints.userinfo, !val, 1);
        }

        val = json_object_get(oidc, "EndSessionEndpoint");
        config->oidc.endpoints.end_session = get_config_string_with_env("EndSessionEndpoint", val, NULL);
        if (config->oidc.endpoints.end_session) {
            log_config_item("EndSessionEndpoint", config->oidc.endpoints.end_session, !val, 1);
        }

        // Validate configuration
        if (config_oidc_validate(&config->oidc) != 0) {
            log_config_item("Status", "Invalid configuration, using defaults", true, 0);
            config_oidc_init(&config->oidc);
        }

        return true;
    } else {
        // Set defaults if no OIDC configuration is provided
        config_oidc_init(&config->oidc);
        log_config_item("Status", "Section missing, using defaults", true, 0);
        return true;
    }
}