/*
 * OIDC Token endpoint for the Hydrogen Project.
 * Handles OAuth 2.0 token requests.
 */

#ifndef HYDROGEN_OIDC_TOKEN_H
#define HYDROGEN_OIDC_TOKEN_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/token endpoint request.
 * Processes OAuth 2.0 token requests and issues tokens.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/token
//@ swagger:method POST
//@ swagger:operationId issueTokens
//@ swagger:tags "OIDC Service"
//@ swagger:summary OAuth 2.0 token endpoint
//@ swagger:description Issues access tokens, refresh tokens, and ID tokens based on the provided grant type. Supports authorization_code, refresh_token, client_credentials, and password grant types. Client authentication is required either via HTTP Basic Authentication or using client_id and client_secret parameters.
//@ swagger:parameter grant_type string required The OAuth 2.0 grant type (authorization_code, refresh_token, client_credentials, password)
//@ swagger:parameter code string conditional The authorization code (required for grant_type=authorization_code)
//@ swagger:parameter redirect_uri string conditional The redirect URI used in the authorization request (required for grant_type=authorization_code)
//@ swagger:parameter client_id string conditional The OAuth 2.0 client identifier (if not using HTTP Basic Authentication)
//@ swagger:parameter client_secret string conditional The OAuth 2.0 client secret (if not using HTTP Basic Authentication)
//@ swagger:parameter refresh_token string conditional The refresh token (required for grant_type=refresh_token)
//@ swagger:parameter username string conditional The resource owner username (required for grant_type=password)
//@ swagger:parameter password string conditional The resource owner password (required for grant_type=password)
//@ swagger:parameter scope string optional Space-delimited list of requested scopes
//@ swagger:parameter code_verifier string conditional PKCE code verifier (required if code_challenge was used in the authorization request)
//@ swagger:response 200 application/json {"type":"object","properties":{"access_token":{"type":"string"},"token_type":{"type":"string","example":"Bearer"},"expires_in":{"type":"integer"},"refresh_token":{"type":"string"},"id_token":{"type":"string"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string"},"error_description":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_client"},"error_description":{"type":"string"}}}
enum MHD_Result handle_oidc_token_endpoint(struct MHD_Connection *connection,
                                       const char *method,
                                       const char *upload_data,
                                       const size_t *upload_data_size,
                                       void **con_cls);

#endif /* HYDROGEN_OIDC_TOKEN_H */
