/**
 * OIDC Relying Party — RP-initiated logout endpoint.
 *
 * Implements POST /api/auth/oidc/end-session. This is the server-side
 * helper that lets Lithium perform a "global signout" by redirecting the
 * browser to the upstream IdP's end_session_endpoint. The endpoint:
 *
 *   1. Validates the Hydrogen JWT from the Authorization header.
 *   2. Deletes the JWT from storage (local logout).
 *   3. If the JWT carries OIDC context (id_token + idp_provider), looks up
 *      the provider's discovery document and builds the IdP logout URL.
 *   4. Returns a JSON object with the optional `redirect_url`.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/api/auth/logout/logout_utils.h>

// Local includes
#include "oidc_rp_end_session.h"
#include "oidc_rp_service.h"
#include "oidc_rp_discovery.h"

#include <string.h>
#include <stdlib.h>

// Extract scheme://authority from a URL, e.g.
// "https://lithium.philement.com/api/auth/oidc/callback" -> "https://lithium.philement.com"
char *oidc_rp_end_session_extract_origin(const char *url) {
    if (!url || !*url) return NULL;

    const char *scheme_end = strstr(url, "://");
    if (!scheme_end) return NULL;

    const char *path_start = strchr(scheme_end + 3, '/');
    size_t origin_len = path_start ? (size_t)(path_start - url) : strlen(url);

    char *origin = malloc(origin_len + 1);
    if (!origin) return NULL;

    memcpy(origin, url, origin_len);
    origin[origin_len] = '\0';
    return origin;
}

// Build the IdP end_session URL with id_token_hint, post_logout_redirect_uri
// and client_id. Returns NULL on allocation or missing-endpoint failure.
char *oidc_rp_end_session_build_idp_logout_url(const char *end_session_endpoint,
                                  const char *id_token,
                                  const char *post_logout_redirect_uri,
                                  const char *client_id) {
    if (!end_session_endpoint || !id_token) return NULL;

    char *enc_id_token = api_url_encode(id_token);
    if (!enc_id_token) return NULL;

    char *enc_redirect = api_url_encode(post_logout_redirect_uri ? post_logout_redirect_uri : "");
    if (!enc_redirect) {
        free(enc_id_token);
        return NULL;
    }

    char *enc_client_id = client_id ? api_url_encode(client_id) : NULL;

    char *url = NULL;
    int ret;
    if (enc_client_id && *enc_client_id) {
        ret = asprintf(&url, "%s?id_token_hint=%s&post_logout_redirect_uri=%s&client_id=%s",
                       end_session_endpoint, enc_id_token, enc_redirect, enc_client_id);
    } else {
        ret = asprintf(&url, "%s?id_token_hint=%s&post_logout_redirect_uri=%s",
                       end_session_endpoint, enc_id_token, enc_redirect);
    }

    free(enc_id_token);
    free(enc_redirect);
    free(enc_client_id);

    if (ret == -1) return NULL;
    return url;
}

enum MHD_Result handle_post_auth_oidc_end_session(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
    (void)url;      // Unused parameter
    (void)version;  // Unused parameter

    // --- Method discrimination — /end-session is POST-only ---
    if (!method || strcmp(method, "POST") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method,
                                                "end_session", "POST");
    }

    // Use common POST body buffering
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;
        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        case API_BUFFER_METHOD_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
        case API_BUFFER_COMPLETE:
            break;
    }

    log_this(SR_AUTH, "Handling auth/oidc/end-session endpoint request", LOG_LEVEL_DEBUG, 0);

    // Feature gate
    if (!oidc_rp_is_enabled()) {
        api_free_post_buffer(con_cls);
        return oidc_rp_send_disabled_response(connection, method, "end_session");
    }

    // Lazy runtime init
    if (!oidc_rp_runtime_lazy_init()) {
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "OIDC runtime not initialized", MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Extract and validate JWT
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Missing Authorization header", MHD_HTTP_UNAUTHORIZED);
    }

    const char *bearer_prefix = "Bearer ";
    size_t prefix_len = strlen(bearer_prefix);
    if (strncmp(auth_header, bearer_prefix, prefix_len) != 0) {
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Invalid Authorization header format", MHD_HTTP_UNAUTHORIZED);
    }

    const char *token = auth_header + prefix_len;
    if (strlen(token) == 0) {
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Empty token", MHD_HTTP_UNAUTHORIZED);
    }

    jwt_validation_result_t validation = validate_jwt_for_logout(token, NULL);
    if (!validation.valid) {
        const char *error_msg = get_jwt_validation_error_message(validation.error);
        log_this(SR_AUTH, "OIDC RP /end-session: JWT validation failed: %s",
                 LOG_LEVEL_ALERT, 1, error_msg);
        free_jwt_validation_result(&validation);
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            error_msg, MHD_HTTP_UNAUTHORIZED);
    }

    if (!validation.claims) {
        log_this(SR_AUTH, "OIDC RP /end-session: JWT valid but claims are NULL",
                 LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&validation);
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Failed to parse token claims", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    const char *database = validation.claims->database;
    if (!database) {
        log_this(SR_AUTH, "OIDC RP /end-session: JWT missing database claim",
                 LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&validation);
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Database not specified", MHD_HTTP_BAD_REQUEST);
    }

    // Delete the JWT from storage (local logout)
    char *jwt_hash = compute_token_hash(token);
    if (jwt_hash) {
        delete_jwt_from_storage(jwt_hash, database);
        free(jwt_hash);
    }

    log_this(SR_AUTH, "OIDC RP /end-session: local logout for user_id=%d",
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);

    // Build optional IdP logout URL
    char *redirect_url = NULL;
    if (validation.claims->id_token && validation.claims->idp_provider) {
        const OIDCRPProviderConfig *provider =
            oidc_rp_find_provider(validation.claims->idp_provider);
        if (provider) {
            const OidcRpDiscoveryDoc *doc = oidc_rp_discovery_get(
                provider->name,
                provider->issuer,
                provider->verify_ssl,
                provider->discovery_cache_seconds);
            if (doc && doc->end_session_endpoint) {
                char *post_logout_redirect_uri = oidc_rp_end_session_extract_origin(provider->redirect_uri);
                if (post_logout_redirect_uri) {
                    redirect_url = oidc_rp_end_session_build_idp_logout_url(
                        doc->end_session_endpoint,
                        validation.claims->id_token,
                        post_logout_redirect_uri,
                        provider->client_id);
                    free(post_logout_redirect_uri);
                }
            }
        }
    }

    // Build response JSON
    json_t *response = json_object();
    if (!response) {
        free(redirect_url);
        free_jwt_validation_result(&validation);
        api_free_post_buffer(con_cls);
        return api_send_error_and_cleanup(connection, con_cls,
            "Failed to create response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_object_set_new(response, "redirect_url",
                        redirect_url ? json_string(redirect_url) : json_null());
    free(redirect_url);

    int user_id = validation.claims ? validation.claims->user_id : 0;
    free_jwt_validation_result(&validation);
    api_free_post_buffer(con_cls);

    log_this(SR_AUTH, "POST /api/auth/oidc/end-session - HTTP 200 OK - user_id=%d",
             LOG_LEVEL_DEBUG, 1, user_id);

    return api_send_json_response(connection, response, MHD_HTTP_OK);
}
