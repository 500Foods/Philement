/*
 * OIDC Discovery endpoint for the Hydrogen Project.
 * Provides OpenID Connect discovery document (.well-known/openid-configuration).
 */

#ifndef HYDROGEN_OIDC_DISCOVERY_H
#define HYDROGEN_OIDC_DISCOVERY_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /.well-known/openid-configuration endpoint request.
 * Returns the OIDC discovery document with information about the OIDC provider.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /.well-known/openid-configuration
//@ swagger:method GET
//@ swagger:operationId getOpenIDConfiguration
//@ swagger:tags "OIDC Service"
//@ swagger:summary OpenID Connect discovery document
//@ swagger:description Returns a JSON document containing the OpenID Provider's configuration information including all supported endpoints, scopes, response types, and claims. This document follows the OpenID Connect Discovery 1.0 specification.
//@ swagger:response 200 application/json {"type":"object","properties":{"issuer":{"type":"string"},"authorization_endpoint":{"type":"string"},"token_endpoint":{"type":"string"},"userinfo_endpoint":{"type":"string"},"jwks_uri":{"type":"string"},"registration_endpoint":{"type":"string"},"scopes_supported":{"type":"array"},"response_types_supported":{"type":"array"},"grant_types_supported":{"type":"array"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create discovery document"}}}
enum MHD_Result handle_oidc_discovery_endpoint(struct MHD_Connection *connection);

#endif /* HYDROGEN_OIDC_DISCOVERY_H */
