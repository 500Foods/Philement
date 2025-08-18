/*
 * System Test API endpoint for the Hydrogen Project.
 * Provides a test endpoint that returns useful diagnostic information.
 */

#ifndef HYDROGEN_SYSTEM_TEST_H
#define HYDROGEN_SYSTEM_TEST_H

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
//@ swagger:path /api/system/test
//@ swagger:method GET
//@ swagger:operationId testSystemEndpointGet
//@ swagger:tags "System Service"
//@ swagger:summary API diagnostic test endpoint
//@ swagger:description Returns diagnostic information useful for testing and debugging API calls. Supports both GET and POST methods to test different request types. The response includes client IP address, authentication details, headers, query parameters, and POST data.
//@ swagger:response 200 application/json {"type":"object","properties":{"ip":{"type":"string","example":"192.168.1.100"},"jwt_claims":{"type":"object"},"headers":{"type":"object"},"query_params":{"type":"array"},"post_data":{"type":"object"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}

//@ swagger:path /api/system/test
//@ swagger:method POST
//@ swagger:operationId testSystemEndpointPost
//@ swagger:tags "System Service"
//@ swagger:summary API diagnostic test endpoint
//@ swagger:description Returns diagnostic information useful for testing and debugging API calls. Supports both GET and POST methods to test different request types. The response includes client IP address, authentication details, headers, query parameters, and POST data.
//@ swagger:response 200 application/json {"type":"object","properties":{"ip":{"type":"string","example":"192.168.1.100"},"jwt_claims":{"type":"object"},"headers":{"type":"object"},"query_params":{"type":"array"},"post_data":{"type":"object"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_test_request(struct MHD_Connection *connection,
                                         const char *method,
                                         const char *upload_data,
                                         size_t *upload_data_size,
                                         void **con_cls);

#endif /* HYDROGEN_SYSTEM_TEST_H */
