/*
 * System AppConfig API Endpoint Header
 * 
 * Declares the /api/system/appconfig endpoint that provides the current
 * application configuration in plain text format.
 */

#ifndef HYDROGEN_APPCONFIG_H
#define HYDROGEN_APPCONFIG_H

// Network headers
#include <microhttpd.h>

/**
 * Handles the /api/system/appconfig endpoint request.
 * Returns the current application configuration.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/appconfig
//@ swagger:method GET
//@ swagger:operationId getSystemAppConfig
//@ swagger:tags "System Service"
//@ swagger:summary Application configuration endpoint
//@ swagger:description Returns the current application configuration settings in plain text format
//@ swagger:response 200 text/plain {"type":"string","description":"Current application configuration"}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string"}}}

// Function declarations
enum MHD_Result handle_system_appconfig_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_APPCONFIG_H */