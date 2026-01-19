/*
 * Conduit Status API endpoint for the Hydrogen Project.
 * Provides database readiness status for Conduit service.
 */

#ifndef HYDROGEN_CONDUIT_STATUS_H
#define HYDROGEN_CONDUIT_STATUS_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <microhttpd.h>
#include <jansson.h>

// Database subsystem includes for type definitions
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

/**
 * Handles the /api/conduit/status endpoint request.
 * Returns readiness status for all configured databases.
 *
 * Request: GET /api/conduit/status
 *
 * Returns:
 * - 200: Status information for all databases
 * - 500: Internal server error
 *
 * @param connection The MHD_Connection to send the response through
 * @param url The requested URL path
 * @param method The HTTP method (GET expected)
 * @param upload_data The request body data
 * @param upload_data_size Pointer to size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/conduit/status
//@ swagger:method GET
//@ swagger:operationId getConduitStatus
//@ swagger:tags "Conduit Service"
//@ swagger:summary Get database readiness status
//@ swagger:description Returns the readiness status of all configured databases for Conduit operations. When authenticated with a valid JWT token, includes additional details like migration status, query cache entries, and DQM statistics.
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","required":["success","databases"],"properties":{"success":{"type":"boolean","description":"Indicates successful status retrieval","example":true},"databases":{"type":"object","description":"Database readiness status by name","properties":{"postgresql_demo":{"type":"object","required":["ready","last_checked"],"properties":{"ready":{"type":"boolean","description":"Whether database is ready for queries","example":true},"last_checked":{"type":"string","description":"ISO 8601 timestamp of last status check","example":"2026-01-18T12:15:00Z"},"migration_status":{"type":"string","description":"Migration completion status (included when authenticated)","enum":["completed","failed","in_progress","not_started"],"example":"completed"},"query_cache_entries":{"type":"integer","description":"Number of queries in QTC (included when authenticated)","example":150}}}}},"dqm_statistics":{"type":"object","description":"DQM statistics (included when authenticated)","properties":{"queue_selection_counters":{"type":"array","items":{"type":"integer"},"description":"Number of queries sent to each queue type"},"total_queries_submitted":{"type":"integer","description":"Total queries submitted"},"total_queries_completed":{"type":"integer","description":"Total queries completed"},"total_queries_failed":{"type":"integer","description":"Total queries failed"},"total_timeouts":{"type":"integer","description":"Total query timeouts"},"per_queue_stats":{"type":"array","items":{"type":"object","properties":{"submitted":{"type":"integer"},"completed":{"type":"integer"},"failed":{"type":"integer"},"avg_execution_time_ms":{"type":"integer"},"last_used":{"type":"integer"}}}}}}}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Internal server error"},"details":{"type":"string","description":"Additional error details","example":"Failed to access database manager"}}}

enum MHD_Result handle_conduit_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_STATUS_H */