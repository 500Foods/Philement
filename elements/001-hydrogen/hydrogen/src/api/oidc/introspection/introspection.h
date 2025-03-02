/*
 * OIDC Introspection endpoint for the Hydrogen Project.
 * Provides token validation and metadata.
 */

#ifndef HYDROGEN_OIDC_INTROSPECTION_H
#define HYDROGEN_OIDC_INTROSPECTION_H

//@ swagger:service OIDC Introspection Service
//@ swagger:description Provides token validation and metadata for resource servers
//@ swagger:tag oidc OpenID Connect protocol endpoints
//@ swagger:tag introspection Token validation and inspection

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/introspect endpoint request.
 * Validates tokens and provides metadata about them.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/introspect
//@ swagger:method POST
//@ swagger:operationId introspectToken
//@ swagger:tags oidc,introspection
//@ swagger:summary OAuth 2.0 token introspection endpoint
//@ swagger:description Allows authorized clients to determine the active state of a token and its metadata as specified in RFC 7662. Resource servers use this endpoint to validate tokens presented by clients and retrieve associated metadata.
//@ swagger:parameter token string required The string value of the token
//@ swagger:parameter token_type_hint string optional A hint about the type of the token (access_token, refresh_token)
//@ swagger:security BasicAuth
//@ swagger:response 200 application/json {"type":"object","properties":{"active":{"type":"boolean"},"scope":{"type":"string"},"client_id":{"type":"string"},"username":{"type":"string"},"token_type":{"type":"string"},"exp":{"type":"integer"},"iat":{"type":"integer"},"nbf":{"type":"integer"},"sub":{"type":"string"},"aud":{"type":"string"},"iss":{"type":"string"},"jti":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_client"},"error_description":{"type":"string","example":"Client authentication failed"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string","example":"invalid_request"},"error_description":{"type":"string","example":"The request is missing a required parameter"}}}
enum MHD_Result handle_oidc_introspection_endpoint(struct MHD_Connection *connection,
                                               const char *method,
                                               const char *upload_data,
                                               size_t *upload_data_size,
                                               void **con_cls);

#endif /* HYDROGEN_OIDC_INTROSPECTION_H */