/*
 * OIDC Discovery endpoint for the Hydrogen Project.
 * Provides OpenID Connect discovery document (.well-known/openid-configuration).
 */

#ifndef HYDROGEN_OIDC_DISCOVERY_H
#define HYDROGEN_OIDC_DISCOVERY_H

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
 * Handles the /.well-known/openid-configuration endpoint request.
 * Returns the OIDC discovery document with information about the OIDC provider.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_discovery_endpoint(struct MHD_Connection *connection);

#endif /* HYDROGEN_OIDC_DISCOVERY_H */