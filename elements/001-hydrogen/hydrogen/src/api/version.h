/*
 * Version API endpoint for the Hydrogen Project.
 * Provides version information for the API and server.
 */

#ifndef HYDROGEN_VERSION_H
#define HYDROGEN_VERSION_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /api/version and /api/system/version endpoint requests.
 * Returns version information in JSON format.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/version
//@ swagger:method GET
//@ swagger:operationId versionEndpointGet
//@ swagger:tags "API Service"
//@ swagger:summary Get API and server version information
//@ swagger:description Returns version information for the API and server in JSON format.
//@ swagger:response 200 application/json {"type":"object","properties":{"api":{"type":"string","example":"0.1"},"server":{"type":"string","example":"1.9.3"},"text":{"type":"string","example":"OctoPrint 1.9.3"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}

//@ swagger:path /api/system/version
//@ swagger:method GET
//@ swagger:operationId systemVersionEndpointGet
//@ swagger:tags "System Service"
//@ swagger:summary Get system version information
//@ swagger:description Returns version information for the system in JSON format.
//@ swagger:response 200 application/json {"type":"object","properties":{"api":{"type":"string","example":"0.1"},"server":{"type":"string","example":"1.9.3"},"text":{"type":"string","example":"OctoPrint 1.9.3"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_version_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_VERSION_H */
