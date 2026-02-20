/**
 * @file alt_query.h
 * @brief Alternative Authenticated Conduit Query API endpoint
 *
 * This module provides authenticated database query execution by reference
 * with database override capability. It validates JWT tokens before executing
 * queries and allows specifying a different database than the one in the JWT claims.
 *
 * @author Hydrogen Framework
 * @date 2026-01-18
 */

#ifndef HYDROGEN_CONDUIT_ALT_QUERY_H
#define HYDROGEN_CONDUIT_ALT_QUERY_H

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

// Include query.h for ApiPostBuffer
#include <src/api/conduit/query/query.h>

// Forward declarations for internal functions (used for testing)
enum MHD_Result validate_jwt_for_auth(
    struct MHD_Connection *connection,
    const char *token);

enum MHD_Result parse_alt_request(
    struct MHD_Connection *connection,
    const char *method,
    ApiPostBuffer *buffer,
    void **con_cls,
    char **token,
    char **database,
    int *query_ref,
    json_t **params_json);

/**
 * @brief Handle the result of api_buffer_post_data for alt_query
 *
 * Returns MHD_YES to continue processing, or an error result to return immediately.
 *
 * @param connection The HTTP connection
 * @param buf_result The buffer result to handle
 * @param con_cls Connection-specific data
 * @return MHD_YES to continue processing, or MHD_NO to stop
 */
enum MHD_Result handle_alt_query_buffer_result(
    struct MHD_Connection *connection,
    ApiBufferResult buf_result,
    void **con_cls
);

/**
 * @brief Handle GET/POST /api/conduit/alt_query requests
 *
 * Executes a single authenticated database query with database override.
 * Requires valid JWT token in request. The database name can be overridden
 * from the request body, allowing access to different databases than the
 * one specified in the JWT claims.
 *
 * Request body must contain:
 * - token: Valid JWT token (required)
 * - database: Database name to execute against (required, overrides JWT claims)
 * - query_ref: Integer identifier for the query in QTC (required)
 * - params: Object with typed parameters (optional)
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
//@ swagger:path /api/conduit/alt_query
//@ swagger:method POST
//@ swagger:operationId executeAuthenticatedQueryWithDatabaseOverride
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute authenticated database query with database override
//@ swagger:description Executes a pre-defined query from the Query Table Cache using a query reference ID with JWT authentication and database override capability. The database name can be specified in the request body, allowing access to different databases than the one in the JWT claims. Supports typed parameters (INTEGER, STRING, BOOLEAN, FLOAT). Returns query results in JSON format with execution metadata.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token","database","query_ref"],"properties":{"token":{"type":"string","description":"JWT bearer token (required)","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."},"database":{"type":"string","description":"Target database name (required, overrides JWT claims)","example":"Acuranzo"},"query_ref":{"type":"integer","description":"Query identifier from Query Table Cache (required)","example":1234},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"userId":123,"quantity":50}},"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"username":"johndoe","email":"john@example.com"}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"isActive":true,"requireAuth":false}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"discount":0.15,"tax":0.07}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","query_ref","rows"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":1234},"description":{"type":"string","description":"Human-readable description of the query","example":"Fetch user profile by ID"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"user_id":123,"username":"johndoe","email":"john@example.com","is_active":true}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1},"column_count":{"type":"integer","description":"Number of columns in result","example":4},"execution_time_ms":{"type":"integer","description":"Query execution time in milliseconds","example":45},"queue_used":{"type":"string","description":"Database queue that handled the request","example":"fast"},"database":{"type":"string","description":"Database name from request (overridden from JWT)","example":"Acuranzo"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Missing required parameter: database"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Authentication error","example":"Invalid or expired JWT token"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query not found"},"query_ref":{"type":"integer","example":9999}}}
//@ swagger:response 408 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query execution timeout"},"query_ref":{"type":"integer","example":1234},"timeout_seconds":{"type":"integer","example":30}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Database error"},"query_ref":{"type":"integer","example":1234}}}
enum MHD_Result handle_conduit_alt_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls
);

/**
 * @brief Cleanup alt query resources
 *
 * Frees all resources associated with an alt query request.
 * Safe to call with NULL values.
 *
 * @param database Database name to free
 * @param query_id Query ID string to free
 * @param param_list Parameter list to free
 * @param converted_sql Converted SQL string to free
 * @param ordered_params Ordered parameters array to free
 * @param param_count Number of ordered parameters
 * @param message Message string to free
 */
void cleanup_alt_query_resources(
    char *database,
    char *query_id,
    ParameterList *param_list,
    char *converted_sql,
    TypedParameter **ordered_params,
    size_t param_count,
    char *message
);

#endif /* HYDROGEN_CONDUIT_ALT_QUERY_H */