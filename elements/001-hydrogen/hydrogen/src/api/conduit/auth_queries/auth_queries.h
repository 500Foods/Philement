/**
 * @file auth_queries.h
 * @brief Authenticated Conduit Queries API endpoint
 *
 * This module provides authenticated database query execution in parallel.
 * It validates JWT tokens before executing multiple queries and extracts the
 * database name from the JWT claims for routing.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#ifndef HYDROGEN_CONDUIT_AUTH_QUERIES_H
#define HYDROGEN_CONDUIT_AUTH_QUERIES_H

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

// Forward declarations for internal functions (used for testing)
PendingQueryResult* submit_single_auth_query(const char *database, json_t *query_obj, int *query_ref_out);
json_t* wait_and_build_single_auth_response(const char *database, int query_ref,
                                          const QueryCacheEntry *cache_entry, const DatabaseQueue *selected_queue,
                                          PendingQueryResult *pending);

/**
 * @brief Handle GET/POST /api/conduit/auth_queries requests
 *
 * Executes multiple authenticated database queries in parallel. Requires valid
 * JWT token in request. The database name is extracted from the JWT token claims
 * (not from request parameters), ensuring authenticated database routing.
 *
 * Request body must contain:
 * - token: Valid JWT token (required)
 * - queries: Array of query objects (required), each containing:
 *   - query_ref: Integer identifier for the query in QTC (required)
 *   - params: Object with typed parameters (optional)
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
//@ swagger:path /api/conduit/auth_queries
//@ swagger:method POST
//@ swagger:operationId executeAuthenticatedQueriesByReference
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute multiple authenticated database queries in parallel
//@ swagger:description Executes multiple pre-defined queries from the Query Table Cache in parallel with JWT authentication. The database name is extracted from the JWT token claims, ensuring secure routing. Supports typed parameters (INTEGER, STRING, BOOLEAN, FLOAT) for each query. Returns an array of query results in JSON format with execution metadata. Subject to rate limiting based on MaxQueriesPerRequest configuration (default: 10).
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token","queries"],"properties":{"token":{"type":"string","description":"JWT bearer token (required)","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."},"queries":{"type":"array","description":"Array of queries to execute in parallel (required)","items":{"type":"object","required":["query_ref"],"properties":{"query_ref":{"type":"integer","description":"Query identifier from Query Table Cache (required)","example":1234},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"userId":123,"quantity":50}},"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"username":"johndoe","email":"john@example.com"}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"isActive":true,"requireAuth":false}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"discount":0.15,"tax":0.07}}}}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","results"],"properties":{"success":{"type":"boolean","description":"Indicates all queries executed successfully","example":true},"results":{"type":"array","description":"Array of query results","items":{"type":"object","required":["success","query_ref"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":1234},"description":{"type":"string","description":"Human-readable description of the query","example":"Fetch user profile by ID"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"user_id":123,"username":"johndoe","email":"john@example.com","is_active":true}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1},"column_count":{"type":"integer","description":"Number of columns in result","example":4},"execution_time_ms":{"type":"integer","description":"Query execution time in milliseconds","example":45},"queue_used":{"type":"string","description":"Database queue that handled the request","example":"fast"},"error":{"type":"string","description":"Error message if query failed","example":"Query not found"}}}},"database":{"type":"string","description":"Database extracted from JWT token","example":"Acuranzo"},"total_execution_time_ms":{"type":"integer","description":"Total time for all queries in milliseconds","example":145}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Missing required parameter: token"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Authentication error","example":"Invalid or expired JWT token"}}}
//@ swagger:response 429 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Rate limiting error","example":"Too many queries requested (max: 10)"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to execute queries"}}}
enum MHD_Result handle_conduit_auth_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_AUTH_QUERIES_H */
