/*
 * OIDC Authorization endpoint for the Hydrogen Project.
 * Handles the OAuth 2.0 authorization code flow.
 */

#ifndef HYDROGEN_OIDC_AUTHORIZATION_H
#define HYDROGEN_OIDC_AUTHORIZATION_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/authorize endpoint request.
 * Processes OAuth 2.0 authorization requests and redirects users accordingly.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (GET, POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/authorize
//@ swagger:method GET
//@ swagger:method POST
//@ swagger:operationId authorizeUser
//@ swagger:tags "OIDC Service"
//@ swagger:summary OAuth 2.0 authorization endpoint
//@ swagger:description Initiates the OAuth 2.0 authorization flow. For GET requests, presents a login UI to the user. For POST requests, processes login data and redirects with an authorization code. Supports multiple response types including 'code' for Authorization Code flow and 'token' for Implicit flow.
//@ swagger:parameter client_id query string true "The OAuth 2.0 client identifier"
//@ swagger:parameter redirect_uri query string true "The URI to redirect to after successful authorization"
//@ swagger:parameter response_type query string true "The OAuth 2.0 response type" code
//@ swagger:parameter scope query string false "Space-delimited list of requested scopes" openid
//@ swagger:parameter state query string false "Opaque value used for state verification"
//@ swagger:parameter nonce query string false "String value used for replay prevention"
//@ swagger:parameter code_challenge query string false "PKCE code challenge"
//@ swagger:parameter code_challenge_method query string false "PKCE code challenge method" S256
//@ swagger:response 302 Redirect to the client's redirect_uri with authorization code or error
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string"},"error_description":{"type":"string"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_oidc_authorization_endpoint(struct MHD_Connection *connection,
                                               const char *method,
                                               const char *upload_data,
                                               size_t *upload_data_size,
                                               void **con_cls);

#endif /* HYDROGEN_OIDC_AUTHORIZATION_H */
