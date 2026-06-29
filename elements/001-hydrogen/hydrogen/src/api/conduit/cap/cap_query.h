/*
 * Cap-Protected Conduit Query API endpoint for the Hydrogen Project.
 * Executes pre-defined protected database queries by reference after
 * verifying a ChaCha (Cap) token.
 */

#ifndef HYDROGEN_CONDUIT_CAP_QUERY_H
#define HYDROGEN_CONDUIT_CAP_QUERY_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <microhttpd.h>
#include <jansson.h>

// API subsystem includes
#include <src/api/api_utils.h>

/**
 * Handles the /api/conduit/cap_query endpoint request.
 * Executes a pre-defined protected query from the Query Table Cache (QTC)
 * after verifying a Cap token with the configured siteverify endpoint.
 *
 * Request body must contain:
 * - query_ref: Integer identifier for the protected query in QTC
 * - database: Database name to execute against
 * - chachaToken: Cap response token from the client
 * - chachaSite: ChaCha site identifier
 * - params: Object with typed parameters (optional)
 *
 * Error contract (all errors return HTTP 400 Bad Request with the same
 * outer shape used by other conduit endpoints):
 *
 *   { "success": false, "error": "<code>", "message": "<detail>" }
 *
 * Error codes:
 *   CAP_TOKEN_MISSING              - chachaToken missing or empty
 *   CAP_TOKEN_INVALID              - chachaToken explicitly rejected by verifier
 *   CAP_VERIFY_FAILED              - Cap verifier unreachable or returned an error
 *   CAP_STATEMENT_TYPE_NOT_ALLOWED - Protected query SQL is not SELECT/INSERT/UPDATE/DELETE
 *
 * Success response:
 *   { "success": true, "query_ref": ..., "rows": [...], ... }
 *
 * When Cap verification fails transiently (HTTP 5xx / timeout) the request
 * is accepted and the success response includes "cap_fallback": true.
 *
 * @param connection The MHD_Connection to send the response through
 * @param url The requested URL path
 * @param method The HTTP method (GET or POST)
 * @param upload_data The request body data
 * @param upload_data_size Pointer to size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/conduit/cap_query
//@ swagger:method POST
//@ swagger:operationId executeCapProtectedQueryByReference
//@ swagger:tags "Conduit Service"
//@ swagger:summary Execute a Cap-protected database query by reference
//@ swagger:description Executes a pre-defined protected query from the Query Table Cache after verifying a Cap (ChaCha) token with the configured siteverify endpoint. Anonymous callers do not need a JWT; instead they supply a Cap response token and site identifier. Protected queries are forced onto the slow queue and restricted to SELECT, INSERT, UPDATE, or DELETE statements. On transient Cap verification failures the request is accepted with cap_fallback set to true.
//@ swagger:request body application/json {"type":"object","required":["query_ref","database","chachaToken","chachaSite"],"properties":{"query_ref":{"type":"integer","description":"Protected query identifier from Query Table Cache (query_type_a28 = 11)","example":85},"database":{"type":"string","description":"Target database name","example":"Demo_PG"},"chachaToken":{"type":"string","description":"Cap response token solved by the client","example":"cap-token-from-widget"},"chachaSite":{"type":"string","description":"Cap public site identifier","example":"a9b8369d3b"},"params":{"type":"object","description":"Typed parameters for query execution (optional)","properties":{"STRING":{"type":"object","description":"String parameters as key-value pairs","example":{"COURSE_NAME":"Calculus I"}},"INTEGER":{"type":"object","description":"Integer parameters as key-value pairs","example":{"count":1}},"BOOLEAN":{"type":"object","description":"Boolean parameters as key-value pairs","example":{"active":true}},"FLOAT":{"type":"object","description":"Float parameters as key-value pairs","example":{"score":9.5}}}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","query_ref","database","rows"],"properties":{"success":{"type":"boolean","description":"Indicates successful query execution","example":true},"cap_fallback":{"type":"boolean","description":"True when Cap verification was transiently unavailable and the request was accepted anyway","example":false},"query_ref":{"type":"integer","description":"The query reference ID that was executed","example":85},"database":{"type":"string","description":"Database name","example":"Demo_PG"},"rows":{"type":"array","description":"Array of result rows as JSON objects","items":{"type":"object"},"example":[{"suggestion_id":1}]},"row_count":{"type":"integer","description":"Number of rows returned","example":1}}}
//@ swagger:response 400 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Error code","example":"CAP_TOKEN_INVALID"},"message":{"type":"string","description":"Human-readable error detail","example":"Cap token rejected by verifier"}}}
enum MHD_Result handle_conduit_cap_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_CAP_QUERY_H */
