/*
 * System Health API endpoint for the Hydrogen Project.
 * Provides a simple health check for monitoring.
 */

#ifndef HYDROGEN_SYSTEM_HEALTH_H
#define HYDROGEN_SYSTEM_HEALTH_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /api/system/health endpoint request.
 * Returns a simple health check response indicating the service is alive.
 * Used primarily by load balancers for health monitoring in distributed deployments.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/health
//@ swagger:method GET
//@ swagger:operationId getSystemHealth
//@ swagger:tags "System Service"
//@ swagger:summary Health check endpoint
//@ swagger:description Returns a simple health check response indicating the service is alive. Used primarily by load balancers for health monitoring in distributed deployments.
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string","example":"Yes, I'm alive, thanks!"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_HEALTH_H */
