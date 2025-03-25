/*
 * OIDC JWKS endpoint for the Hydrogen Project.
 * Provides JSON Web Key Set for token verification.
 */

#ifndef HYDROGEN_OIDC_JWKS_H
#define HYDROGEN_OIDC_JWKS_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /oauth/jwks endpoint request.
 * Returns the JSON Web Key Set used for token verification.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /oauth/jwks
//@ swagger:method GET
//@ swagger:operationId getJWKS
//@ swagger:tags "OIDC Service"
//@ swagger:summary JSON Web Key Set endpoint
//@ swagger:description Returns a set of JSON Web Keys (JWK) that represent the public part of the keys used by the OIDC provider to sign tokens. Clients use these keys to verify the signature of tokens issued by the provider.
//@ swagger:response 200 application/json {"type":"object","properties":{"keys":{"type":"array","items":{"type":"object","properties":{"kty":{"type":"string"},"use":{"type":"string"},"kid":{"type":"string"},"alg":{"type":"string"},"n":{"type":"string"},"e":{"type":"string"}}}}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_oidc_jwks_endpoint(struct MHD_Connection *connection);

#endif /* HYDROGEN_OIDC_JWKS_H */