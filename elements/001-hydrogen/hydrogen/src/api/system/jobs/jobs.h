/*
 * System Jobs API endpoint for the Hydrogen Project.
 * Returns the scripting subsystem's scoreboard job list.
 */

#ifndef HYDROGEN_SYSTEM_JOBS_H
#define HYDROGEN_SYSTEM_JOBS_H

// Network headers
#include <microhttpd.h>

// Project includes
#include <src/api/api_utils.h>

/**
 * Handles the /api/system/jobs endpoint request.
 * Returns a paginated JSON array of scripting subsystem jobs.
 *
 * Requires JWT authentication.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/jobs
//@ swagger:method GET
//@ swagger:operationId getSystemJobs
//@ swagger:tags "System Service"
//@ swagger:summary Scripting job list endpoint
//@ swagger:description Returns a paginated JSON array of scripting subsystem scoreboard job entries. Requires a valid JWT.
//@ swagger:response 200 application/json {"type":"array","description":"Array of job objects"}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"Unauthorized"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to generate job list"}}}
enum MHD_Result handle_system_jobs_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_JOBS_H */
