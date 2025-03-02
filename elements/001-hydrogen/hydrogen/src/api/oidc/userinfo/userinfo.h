/*
 * OIDC UserInfo endpoint for the Hydrogen Project.
 * Provides authenticated user information.
 */

#ifndef HYDROGEN_OIDC_USERINFO_H
#define HYDROGEN_OIDC_USERINFO_H

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
 * Handles the /oauth/userinfo endpoint request.
 * Returns information about the authenticated user based on the access token.
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (GET, POST)
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_userinfo_endpoint(struct MHD_Connection *connection,
                                         const char *method);

#endif /* HYDROGEN_OIDC_USERINFO_H */