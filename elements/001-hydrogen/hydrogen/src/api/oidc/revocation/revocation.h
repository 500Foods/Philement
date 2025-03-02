/*
 * OIDC Revocation endpoint for the Hydrogen Project.
 * Allows clients to invalidate tokens.
 */

#ifndef HYDROGEN_OIDC_REVOCATION_H
#define HYDROGEN_OIDC_REVOCATION_H

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
enum MHD_Result handle_oidc_revocation_endpoint(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls);

#endif /* HYDROGEN_OIDC_REVOCATION_H */