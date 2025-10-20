/*
 * Conduit Query API endpoint for the Hydrogen Project.
 * Executes pre-defined database queries by reference.
 */

#ifndef HYDROGEN_CONDUIT_QUERY_H
#define HYDROGEN_CONDUIT_QUERY_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /api/conduit/query endpoint request.
 * Executes a pre-defined query from the Query Table Cache (QTC) with typed parameters.
 *
 * Request body must contain:
 * - query_ref: Integer identifier for the query in QTC
 * - database: Database name to execute against
 * - params: Object with typed parameters (INTEGER, STRING, BOOLEAN, FLOAT)
 *
 * Returns:
 * - 200: Query executed successfully with results
 * - 400: Invalid request (missing fields, malformed parameters)
 * - 404: Query not found in QTC
 * - 408: Query execution timeout
 * - 500: Database error or internal server error
 * - 501: Not yet implemented (stub response)
 *
 * @param connection The MHD_Connection to send the response through
 * @param url The requested URL path
 * @param method The HTTP method (POST expected)
 * @param upload_data The request body data
 * @param upload_data_size Pointer to size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/conduit/query
//@ swagger:method GET
//@ swagger:method POST
//@ swagger:operationId executeQueryByReference
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute database query by reference
//@ swagger:description Executes a pre-defined query from the Query Table Cache using a query reference ID. Supports typed parameters (INTEGER, STRING, BOOLEAN, FLOAT) that are automatically converted to database-specific parameter formats. Returns query results in JSON format with execution metadata. Accepts both GET with query parameters and POST with JSON body.
//@ swagger:parameter query_ref query integer true "Query identifier from Query Table Cache" 1234
//@ swagger:parameter database query string true "Target database name" Acuranzo
//@ swagger:request body application/json {"type":"object","required":["query_ref","database"],"properties":{"query_ref":{"type":"integer","description":"Query identifier from Query Table Cache (required)","example":1234},"database":{"type":"string","description":"Target database name (required)","example":"Acuranzo"},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"userId":123,"quantity":50}},"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"username":"johndoe","email":"john@example.com"}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"isActive":true,"requireAuth":false}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"discount":0.15,"tax":0.07}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","query_ref","rows"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":1234},"description":{"type":"string","description":"Human-readable description of the query","example":"Fetch user profile by ID"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"user_id":123,"username":"johndoe","email":"john@example.com","is_active":true}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1},"column_count":{"type":"integer","description":"Number of columns in result","example":4},"execution_time_ms":{"type":"integer","description":"Query execution time in milliseconds","example":45},"queue_used":{"type":"string","description":"Database queue that handled the request","example":"fast"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error message","example":"Missing required parameter: query_ref"},"details":{"type":"string","description":"Additional error details","example":"The query_ref parameter is required"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query not found"},"query_ref":{"type":"integer","example":9999},"database":{"type":"string","example":"Acuranzo"}}}
//@ swagger:response 408 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query execution timeout"},"query_ref":{"type":"integer","example":1234},"timeout_seconds":{"type":"integer","example":30},"database":{"type":"string","example":"Acuranzo"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Database error"},"database_error":{"type":"string","example":"Table 'users' not found"},"query_ref":{"type":"integer","example":1234},"database":{"type":"string","example":"Acuranzo"}}}
//@ swagger:response 501 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Query execution not yet implemented"},"message":{"type":"string","example":"The Conduit service infrastructure is being built. This endpoint will execute pre-defined queries once the Query Table Cache, parameter processing, and queue selection systems are complete."},"status":{"type":"string","example":"under_construction"}}}
enum MHD_Result handle_conduit_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_QUERY_H */