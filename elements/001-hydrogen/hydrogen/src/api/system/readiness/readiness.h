/*
 * System Readiness API endpoint for the Hydrogen Project.
 * Reports whether the server is ready to accept requests (all databases/migrations complete).
 */

#ifndef HYDROGEN_SYSTEM_READINESS_H
#define HYDROGEN_SYSTEM_READINESS_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /api/system/readiness endpoint request.
 *
 * Intended for use as a Kubernetes readiness probe (and by the test suite). Returns
 * HTTP 200 once the server is Ready For Requests (every enabled database's Lead DQM
 * has completed its conductor sequence), and HTTP 503 while migrations are still in
 * progress. The body reports per-database progress so polling shows databases moving
 * from "starting" to "started".
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/readiness
//@ swagger:method GET
//@ swagger:operationId getSystemReadiness
//@ swagger:tags "System Service"
//@ swagger:summary Readiness check endpoint
//@ swagger:description Returns HTTP 200 when the server is ready to accept requests (all databases and migrations complete), or HTTP 503 while startup/migrations are still in progress. Intended for use as a Kubernetes readiness probe so traffic is routed elsewhere until the pod is fully ready.
//@ swagger:response 200 application/json {"type":"object","properties":{"ready":{"type":"boolean","example":true},"status":{"type":"string","example":"ready"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"ready":{"type":"boolean","example":false},"status":{"type":"string","example":"starting Demo_PG; started Demo_SQL"}}}
enum MHD_Result handle_system_readiness_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_READINESS_H */
