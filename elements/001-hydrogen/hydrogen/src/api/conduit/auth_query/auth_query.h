/**
 * @file auth_query.h
 * @brief Authenticated Conduit Query API endpoint
 *
 * This module provides authenticated database query execution by reference.
 * It validates JWT tokens before executing queries and extracts the database
 * name from the JWT claims for routing.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#ifndef HYDROGEN_CONDUIT_AUTH_QUERY_H
#define HYDROGEN_CONDUIT_AUTH_QUERY_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <microhttpd.h>
#include <jansson.h>

// Database subsystem includes
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>

/**
 * @brief Handle GET/POST /api/conduit/auth_query requests
 *
 * Executes a single authenticated database query. Requires valid JWT token
 * in request. The database name is extracted from the JWT token claims
 * (not from request parameters), ensuring authenticated database routing.
 *
 * Request body must contain:
 * - token: Valid JWT token (required)
 * - query_ref: Integer identifier for the query in QTC (required)
 * - params: Object with typed parameters (optional)
 *
 * The database field is extracted from the authenticated JWT token, not from
 * the request body, ensuring the user can only access databases they're
 * authorized for.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param version The HTTP version
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
//@ swagger:path /api/conduit/auth_query
//@ swagger:method POST
//@ swagger:operationId executeAuthenticatedQueryByReference
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute authenticated database query by reference
//@ swagger:description Executes a pre-defined query from the Query Table Cache using a query reference ID with JWT authentication. The database name is extracted from the JWT token claims, ensuring secure routing. Supports typed parameters (INTEGER, STRING, BOOLEAN, FLOAT). Returns query results in JSON format with execution metadata.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token","query_ref"],"properties":{"token":{"type":"string","description":"JWT bearer token (required)","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."},"query_ref":{"type":"integer","description":"Query identifier from Query Table Cache (required)","example":1234},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"userId":123,"quantity":50}},"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"username":"johndoe","email":"john@example.com"}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"isActive":true,"requireAuth":false}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"discount":0.15,"tax":0.07}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","query_ref","rows"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":1234},"description":{"type":"string","description":"Human-readable description of the query","example":"Fetch user profile by ID"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"user_id":123,"username":"johndoe","email":"john@example.com","is_active":true}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1},"column_count":{"type":"integer","description":"Number of columns in result","example":4},"execution_time_ms":{"type":"integer","description":"Query execution time in milliseconds","example":45},"queue_used":{"type":"string","description":"Database queue that handled the request","example":"fast"},"database":{"type":"string","description":"Database extracted from JWT token","example":"Acuranzo"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Missing required parameter: token"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Authentication error","example":"Invalid or expired JWT token"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query not found"},"query_ref":{"type":"integer","example":9999}}}
//@ swagger:response 408 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query execution timeout"},"query_ref":{"type":"integer","example":1234},"timeout_seconds":{"type":"integer","example":30}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Database error"},"query_ref":{"type":"integer","example":1234}}}
enum MHD_Result handle_conduit_auth_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_AUTH_QUERY_H */
