/*
 * OIDC Service API module for the Hydrogen Project.
 * Provides OpenID Connect protocol endpoints.
 */

#ifndef HYDROGEN_OIDC_SERVICE_H
#define HYDROGEN_OIDC_SERVICE_H

//@ swagger:service OIDC Service
//@ swagger:description OpenID Connect authentication and identity management endpoints
//@ swagger:version 1.0.0
//@ swagger:tag "OIDC Service" Provides OpenID Connect protocol endpoints and authentication services

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project Libraries
#include <src/hydrogen.h>
#include <src/webserver/web_server_core.h>
#include <src/oidc/oidc_service.h>

// Include all OIDC endpoint headers
#include "discovery/discovery.h"
#include "authorization/authorization.h"
#include "token/token.h"
#include "userinfo/userinfo.h"
#include "jwks/jwks.h"
#include "introspection/introspection.h"
#include "revocation/revocation.h"
#include "registration/registration.h"
#include "end_session/end_session.h"

/*
 * Initialize OIDC API endpoints
 *
 * @param oidc_context The OIDC service context
 * @return true if initialization successful, false otherwise
 */
bool init_oidc_endpoints(OIDCContext *oidc_context);

/*
 * Clean up OIDC API endpoints
 */
void cleanup_oidc_endpoints(void);

/*
 * Register OIDC API endpoints with the web server
 * 
 * This function registers all OIDC endpoints with the web server's
 * routing system.
 *
 * @return true if registration successful, false otherwise
 */
bool register_oidc_endpoints(void);

/*
 * Handle an OIDC HTTP request
 *
 * This function is called by the main request handler when a request
 * to an OIDC endpoint is received.
 *
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method (GET, POST, etc.)
 * @param version The HTTP version
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_request(struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls);

/*
 * Check if a URL is an OIDC endpoint
 *
 * @param url The URL to check
 * @return true if URL is an OIDC endpoint, false otherwise
 */
bool is_oidc_endpoint(const char *url);

/* 
 * Utility Functions for OIDC Endpoints
 */

// Extract OAuth query parameters from request
bool extract_oauth_params(struct MHD_Connection *connection, char **client_id,
                         char **redirect_uri, char **response_type,
                         char **scope, char **state, char **nonce,
                         char **code_challenge, char **code_challenge_method);

// Extract token request parameters
bool extract_token_request_params(struct MHD_Connection *connection,
                                const char *upload_data, size_t upload_data_size,
                                char **grant_type, char **code, char **redirect_uri,
                                char **client_id, char **client_secret,
                                char **refresh_token, char **code_verifier);

// Extract client credentials from Basic Auth header
bool extract_client_credentials(struct MHD_Connection *connection,
                              char **client_id, char **client_secret);

// Send OAuth error response
enum MHD_Result send_oauth_error(struct MHD_Connection *connection,
                             const char *error, const char *error_description,
                             const char *redirect_uri, const char *state);

// Send OIDC JSON response
enum MHD_Result send_oidc_json_response(struct MHD_Connection *connection,
                                    const char *json, unsigned int status_code);

// Validate required OAuth parameters
bool validate_oauth_params(const char *client_id, const char *redirect_uri,
                         const char *response_type, char **error,
                         char **error_description);

// Add CORS headers for OIDC endpoints
void add_oidc_cors_headers(struct MHD_Response *response);

#endif /* HYDROGEN_OIDC_SERVICE_H */
