/*
 * OIDC JWKS endpoint for the Hydrogen Project.
 * Provides JSON Web Key Set for token verification.
 */

#ifndef HYDROGEN_OIDC_JWKS_H
#define HYDROGEN_OIDC_JWKS_H

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
 * Handles the /oauth/jwks endpoint request.
 * Returns the JSON Web Key Set used for token verification.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_jwks_endpoint(struct MHD_Connection *connection);

#endif /* HYDROGEN_OIDC_JWKS_H */