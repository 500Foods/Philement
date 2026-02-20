/**
 * @file alt_queries.h
 * @brief Alternative Authenticated Conduit Queries API endpoint
 *
 * This module provides authenticated database query execution in parallel
 * with database override capability. It validates JWT tokens before executing
 * multiple queries and allows specifying a different database than the one
 * in the JWT claims.
 *
 * @author Hydrogen Framework
 * @date 2026-01-18
 */

#ifndef HYDROGEN_CONDUIT_ALT_QUERIES_H
#define HYDROGEN_CONDUIT_ALT_QUERIES_H

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

// Include queries.h for DeduplicationResult enum and ApiPostBuffer
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/query/query.h>

// Forward declarations for internal functions (used for testing)
enum MHD_Result alt_queries_deduplicate_and_validate(
    struct MHD_Connection *connection,
    json_t *queries_array,
    const char *database,
    json_t **deduplicated_queries,
    size_t **mapping_array,
    bool **is_duplicate,
    DeduplicationResult *result_code);

enum MHD_Result validate_jwt_for_auth_alt(
    struct MHD_Connection *connection,
    const char *token);

enum MHD_Result parse_alt_queries_request(
    struct MHD_Connection *connection,
    const char *method,
    ApiPostBuffer *buffer,
    void **con_cls,
    char **token,
    char **database,
    json_t **queries_array);

enum MHD_Result execute_single_alt_query(
    struct MHD_Connection *connection,
    json_t *query_obj,
    const char *database,
    int *query_ref,
    PendingQueryResult **pending,
    QueryCacheEntry **cache_entry,
    DatabaseQueue **selected_queue);

/**
 * @brief Cleanup alt queries resources
 *
 * Frees all resources associated with an alt queries request.
 * Safe to call with NULL values.
 *
 * @param database Database name to free
 * @param queries_array Queries array to free
 * @param deduplicated_queries Deduplicated queries array to free
 * @param mapping_array Mapping array to free
 * @param is_duplicate Is duplicate array to free
 * @param pending_results Pending results array to free
 * @param query_refs Query refs array to free
 * @param cache_entries Cache entries array to free
 * @param selected_queues Selected queues array to free
 * @param unique_results Unique results array to free
 * @param query_count Number of queries
 */
void cleanup_alt_queries_resources(
    char *database,
    json_t *queries_array,
    json_t *deduplicated_queries,
    size_t *mapping_array,
    bool *is_duplicate,
    PendingQueryResult **pending_results,
    int *query_refs,
    QueryCacheEntry **cache_entries,
    DatabaseQueue **selected_queues,
    json_t **unique_results,
    size_t query_count
);

/**
 * @brief Handle GET/POST /api/conduit/alt_queries requests
 *
 * Executes multiple authenticated database queries in parallel with database override.
 * Requires valid JWT token in request. The database name can be overridden
 * from the request body, allowing access to different databases than the
 * one specified in the JWT claims.
 *
 * Request body must contain:
 * - token: Valid JWT token (required)
 * - database: Database name to execute against (required, overrides JWT claims)
 * - queries: Array of query objects (required), each containing:
 *   - query_ref: Integer identifier for the query in QTC (required)
 *   - params: Object with typed parameters (optional)
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
//@ swagger:path /api/conduit/alt_queries
//@ swagger:method POST
//@ swagger:operationId executeAuthenticatedQueriesWithDatabaseOverride
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute multiple authenticated database queries in parallel with database override
//@ swagger:description Executes multiple pre-defined queries from the Query Table Cache in parallel with JWT authentication and database override capability. The database name can be specified in the request body, allowing access to different databases than the one in the JWT claims. Supports typed parameters (INTEGER, STRING, BOOLEAN, FLOAT) for each query. Returns an array of query results in JSON format with execution metadata.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token","database","queries"],"properties":{"token":{"type":"string","description":"JWT bearer token (required)","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."},"database":{"type":"string","description":"Target database name (required, overrides JWT claims)","example":"Acuranzo"},"queries":{"type":"array","description":"Array of queries to execute in parallel (required)","items":{"type":"object","required":["query_ref"],"properties":{"query_ref":{"type":"integer","description":"Query identifier from Query Table Cache (required)","example":1234},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"userId":123,"quantity":50}},"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"username":"johndoe","email":"john@example.com"}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"isActive":true,"requireAuth":false}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"discount":0.15,"tax":0.07}}}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","results"],"properties":{"success":{"type":"boolean","description":"Indicates all queries executed successfully","example":true},"results":{"type":"array","description":"Array of query results","items":{"type":"object","required":["success","query_ref"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":1234},"description":{"type":"string","description":"Human-readable description of the query","example":"Fetch user profile by ID"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"user_id":123,"username":"johndoe","email":"john@example.com","is_active":true}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1},"column_count":{"type":"integer","description":"Number of columns in result","example":4},"execution_time_ms":{"type":"integer","description":"Query execution time in milliseconds","example":45},"queue_used":{"type":"string","description":"Database queue that handled the request","example":"fast"},"error":{"type":"string","description":"Error message if query failed","example":"Query not found"}}}},"database":{"type":"string","description":"Database name from request (overridden from JWT)","example":"Acuranzo"},"total_execution_time_ms":{"type":"integer","description":"Total time for all queries in milliseconds","example":145}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Missing required parameter: database"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Authentication error","example":"Invalid or expired JWT token"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to execute queries"}}}
enum MHD_Result handle_conduit_alt_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_ALT_QUERIES_H */