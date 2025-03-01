/*
 * System Health API endpoint for the Hydrogen Project.
 * Provides a simple health check for monitoring.
 */

#ifndef HYDROGEN_SYSTEM_HEALTH_H
#define HYDROGEN_SYSTEM_HEALTH_H

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
 * Handles the /api/system/health endpoint request.
 * Returns a simple health check response indicating the service is alive.
 * Used primarily by load balancers for health monitoring in distributed deployments.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_HEALTH_H */