/*
 * OIDC UserInfo endpoint for the Hydrogen Project.
 * Provides authenticated user information.
 */

#ifndef HYDROGEN_OIDC_USERINFO_H
#define HYDROGEN_OIDC_USERINFO_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/userinfo endpoint request.
 * Returns information about the authenticated user based on the access token.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (GET, POST)
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/userinfo
//@ swagger:method GET
//@ swagger:method POST
//@ swagger:operationId getUserInfo
//@ swagger:tags "OIDC Service"
//@ swagger:summary OpenID Connect UserInfo endpoint
//@ swagger:description Returns claims about the authenticated end-user. Requires a valid access token with appropriate scopes. The claims returned depend on the scopes associated with the access token and the user's profile data.
//@ swagger:security BearerAuth
//@ swagger:response 200 application/json {"type":"object","properties":{"sub":{"type":"string"},"name":{"type":"string"},"given_name":{"type":"string"},"family_name":{"type":"string"},"email":{"type":"string"},"email_verified":{"type":"boolean"},"picture":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_token"},"error_description":{"type":"string","example":"The access token is invalid"}}}
//@ swagger:response 403 application/json {"type":"object","properties":{"error":{"type":"string","example":"insufficient_scope"},"error_description":{"type":"string","example":"The access token does not have the required scopes"}}}
enum MHD_Result handle_oidc_userinfo_endpoint(struct MHD_Connection *connection,
                                         const char *method);

#endif /* HYDROGEN_OIDC_USERINFO_H */
