/**
 * @file oidc_rp_end_session.h
 * @brief OIDC Relying Party — RP-initiated logout endpoint.
 *
 * Implements `/api/auth/oidc/end-session`. The endpoint validates the
 * current Hydrogen JWT, deletes it from storage, and returns a JSON object
 * with a `redirect_url` that the SPA should navigate to in order to sign
 * the user out of the upstream IdP (Keycloak). If the current session was
 * not established via OIDC, `redirect_url` is null and the local logout
 * still occurs.
 */

#ifndef OIDC_RP_END_SESSION_H
#define OIDC_RP_END_SESSION_H

#include <src/hydrogen.h>

/**
 * Handle POST /api/auth/oidc/end-session requests.
 *
 * Requires a valid Hydrogen JWT in the Authorization header. On success,
 * returns HTTP 200 with JSON body:
 *   { "redirect_url": "<IdP logout URL>" | null }
 *
 * On authentication or validation failure, returns the appropriate 4xx
 * error response.
 */
enum MHD_Result handle_post_auth_oidc_end_session(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

/**
 * Extract scheme://authority from a URL.
 *
 * "https://lithium.philement.com/api/auth/oidc/callback"
 *   -> "https://lithium.philement.com"
 *
 * Returns a newly-allocated string (caller frees) or NULL when url is
 * NULL/empty or has no "://" scheme separator.
 */
char *oidc_rp_end_session_extract_origin(const char *url);

/**
 * Build the IdP end_session URL from its endpoint, an id_token_hint, an
 * optional post_logout_redirect_uri and an optional client_id.
 *
 * Returns a newly-allocated string (caller frees) or NULL on allocation
 * failure or when end_session_endpoint / id_token is NULL.
 */
char *oidc_rp_end_session_build_idp_logout_url(const char *end_session_endpoint,
                                               const char *id_token,
                                               const char *post_logout_redirect_uri,
                                               const char *client_id);

#endif // OIDC_RP_END_SESSION_H
