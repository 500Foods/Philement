/*
 * OIDC End Session endpoint for the Hydrogen Project.
 * Allows users to log out and end their sessions.
 */

#ifndef HYDROGEN_OIDC_END_SESSION_H
#define HYDROGEN_OIDC_END_SESSION_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/end-session endpoint request.
 * Allows users to log out and terminate their sessions.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (GET, POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/end-session
//@ swagger:method GET
//@ swagger:method POST
//@ swagger:operationId endSession
//@ swagger:tags "OIDC Service"
//@ swagger:summary OpenID Connect Session Management endpoint
//@ swagger:description Implements the OpenID Connect RP-Initiated Logout specification. Allows users to log out and terminate their session with the OpenID Provider. Can also notify Relying Parties (client applications) that the user's session has ended.
//@ swagger:parameter id_token_hint query string false "The ID Token previously issued to the client"
//@ swagger:parameter post_logout_redirect_uri query string false "URI to redirect the user to after logout"
//@ swagger:parameter state query string false "Opaque value used by the client to maintain state"
//@ swagger:response 302 Redirects to the post_logout_redirect_uri if provided and valid
//@ swagger:response 200 text/html HTML page confirming successful logout when no valid redirect URI is provided
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string"},"error_description":{"type":"string"}}}
enum MHD_Result handle_oidc_end_session_endpoint(struct MHD_Connection *connection,
                                            const char *method,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls);

#endif /* HYDROGEN_OIDC_END_SESSION_H */
