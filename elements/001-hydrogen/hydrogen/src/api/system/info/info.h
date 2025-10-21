/*
 * System Info API endpoint for the Hydrogen Project.
 * Provides system-level information.
 */

#ifndef HYDROGEN_SYSTEM_INFO_H
#define HYDROGEN_SYSTEM_INFO_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>

// Third-party includes
#include <microhttpd.h>
#include <jansson.h>

// Project includes
#include <src/config/config.h>
#include <src/state/state.h>
#include <src/websocket/websocket_server_internal.h>

/**
 * Handles the /api/system/info endpoint request.
 * Returns system information in JSON format including:
 * - Hardware information
 * - Operating system details
 * - Runtime statistics
 * - Version information
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/info
//@ swagger:method GET
//@ swagger:operationId getSystemInfo
//@ swagger:tags "System Service"
//@ swagger:summary System information endpoint
//@ swagger:description Returns comprehensive system information in JSON format including hardware details, operating system information, runtime statistics, and version information.
//@ swagger:response 200 application/json {"type":"object","properties":{"hardware":{"type":"object"},"os":{"type":"object"},"runtime":{"type":"object"},"version":{"type":"object"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);

/**
 * Extract WebSocket metrics for testing purposes.
 * This function is made non-static to enable unit testing.
 *
 * @param metrics Pointer to WebSocketMetrics structure to fill
 */
void extract_websocket_metrics(WebSocketMetrics *metrics);

#endif /* HYDROGEN_SYSTEM_INFO_H */
