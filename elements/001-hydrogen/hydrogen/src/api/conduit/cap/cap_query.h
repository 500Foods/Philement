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
enum MHD_Result handle_conduit_cap_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif /* HYDROGEN_CONDUIT_CAP_QUERY_H */
