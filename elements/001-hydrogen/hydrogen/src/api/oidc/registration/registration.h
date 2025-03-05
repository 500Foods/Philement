/*
 * OIDC Registration endpoint for the Hydrogen Project.
 * Allows clients to register with the OIDC provider.
 */

#ifndef HYDROGEN_OIDC_REGISTRATION_H
#define HYDROGEN_OIDC_REGISTRATION_H

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
 * Handles the /oauth/register endpoint request.
 * Allows clients to register with the OIDC provider.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (POST)
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/register
//@ swagger:method POST
//@ swagger:operationId registerClient
//@ swagger:tags "OIDC Service"
//@ swagger:summary Dynamic Client Registration endpoint
//@ swagger:description Allows clients to register with the OIDC provider, following the OpenID Connect Dynamic Client Registration protocol. The registration metadata is submitted as a JSON document and the server returns client credentials and configuration.
//@ swagger:requestBody application/json {"type":"object","properties":{"redirect_uris":{"type":"array","items":{"type":"string"}},"client_name":{"type":"string"},"client_uri":{"type":"string"},"logo_uri":{"type":"string"},"scope":{"type":"string"},"grant_types":{"type":"array","items":{"type":"string"}},"response_types":{"type":"array","items":{"type":"string"}},"token_endpoint_auth_method":{"type":"string"},"jwks_uri":{"type":"string"},"jwks":{"type":"object"}}}
//@ swagger:response 201 application/json {"type":"object","properties":{"client_id":{"type":"string"},"client_secret":{"type":"string"},"client_id_issued_at":{"type":"integer"},"client_secret_expires_at":{"type":"integer"},"registration_access_token":{"type":"string"},"registration_client_uri":{"type":"string"},"redirect_uris":{"type":"array","items":{"type":"string"}},"client_name":{"type":"string"},"client_uri":{"type":"string"},"logo_uri":{"type":"string"},"scope":{"type":"string"},"grant_types":{"type":"array","items":{"type":"string"}},"response_types":{"type":"array","items":{"type":"string"}},"token_endpoint_auth_method":{"type":"string"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string"},"error_description":{"type":"string"}}}
enum MHD_Result handle_oidc_registration_endpoint(struct MHD_Connection *connection,
                                             const char *method,
                                             const char *upload_data,
                                             size_t *upload_data_size,
                                             void **con_cls);

#endif /* HYDROGEN_OIDC_REGISTRATION_H */