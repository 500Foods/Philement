/*
 * System Test API endpoint for the Hydrogen Project.
 * Provides a test endpoint that returns useful diagnostic information.
 */

#ifndef HYDROGEN_SYSTEM_TEST_H
#define HYDROGEN_SYSTEM_TEST_H

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
 * Handles the /api/system/test endpoint request.
 * Returns useful diagnostic information in JSON format including:
 * - IP of the caller
 * - JWT claims (if present in the Authorization header)
 * - X-Forwarded-For header information (for proxy detection)
 * - Query parameters as a JSON array
 * - POST data with URL decoding
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (GET, POST)
 * @param upload_data Upload data for POST requests
 * @param upload_data_size Size of the upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_system_test_request(struct MHD_Connection *connection,
                                         const char *method,
                                         const char *upload_data,
                                         size_t *upload_data_size,
                                         void **con_cls);

#endif /* HYDROGEN_SYSTEM_TEST_H */