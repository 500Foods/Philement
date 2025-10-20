/*
 * OIDC Revocation endpoint for the Hydrogen Project.
 * Allows clients to invalidate tokens.
 */

#ifndef HYDROGEN_OIDC_REVOCATION_H
#define HYDROGEN_OIDC_REVOCATION_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/revoke endpoint request.
 * Allows clients to invalidate tokens.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/revoke
//@ swagger:method POST
//@ swagger:operationId revokeToken
//@ swagger:tags "OIDC Service"
//@ swagger:summary OAuth 2.0 token revocation endpoint
//@ swagger:description Allows clients to notify the authorization server that a token is no longer needed, allowing the server to invalidate the token. This endpoint implements RFC 7009 and supports revocation of both access tokens and refresh tokens.
//@ swagger:parameter token formData string true "The token to be revoked"
//@ swagger:parameter token_type_hint formData string false "A hint about the type of the token" access_token
//@ swagger:security BasicAuth
//@ swagger:response 200 application/json {} An empty JSON object
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_request"},"error_description":{"type":"string","example":"The request is missing a required parameter"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_client"},"error_description":{"type":"string","example":"Client authentication failed"}}}
enum MHD_Result handle_oidc_revocation_endpoint(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls);

#endif /* HYDROGEN_OIDC_REVOCATION_H */
