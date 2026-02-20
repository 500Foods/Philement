/**
 * @file auth_query.c
 * @brief Authenticated Conduit Query API endpoint implementation
 *
 * This module implements the authenticated database query execution endpoint.
 * It validates JWT tokens before executing queries and extracts the database
 * name from JWT claims for secure routing.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#include <src/hydrogen.h>

// Network headers
#include <microhttpd.h>

// Standard C libraries
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include <src/api/api_service.h>
#include <src/api/api_utils.h>  // For api_send_json_response
#include <src/api/conduit/auth_query/auth_query.h>
#include <src/api/conduit/query/query.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/logging/logging.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Local includes
#include "../conduit_service.h"
#include "../conduit_helpers.h"
#include "../helpers/auth_jwt_helper.h"

/**
 * Handle the result of api_buffer_post_data.
 * Returns MHD_YES to continue processing, or an error result to return immediately.
 */
enum MHD_Result handle_auth_query_buffer_result(struct MHD_Connection *connection, ApiBufferResult buf_result, void **con_cls) {
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            // More data expected for POST, continue receiving
            return MHD_YES;

        case API_BUFFER_ERROR:
            // Error occurred during buffering
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);

        case API_BUFFER_METHOD_ERROR:
            // Unsupported HTTP method
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);

        case API_BUFFER_COMPLETE:
            // All data received (or GET request), continue with processing
            return MHD_YES;

        default:
            // Should not happen, but handle gracefully
            return api_send_error_and_cleanup(connection, con_cls,
                "Unknown buffer result", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
}

// Forward declaration - prototype is in auth_query.h
enum MHD_Result validate_jwt_from_header(
    struct MHD_Connection *connection,
    jwt_validation_result_t **jwt_result
);

/**
 * @brief Validate JWT token from Authorization header and extract database name and claims
 *
 * Extracts the JWT token from the Authorization header, validates it,
 * and extracts the database name from the token claims. This ensures
 * secure database routing based on authenticated user sessions.
 *
 * @param connection The HTTP connection for error responses
 * @param jwt_result Output parameter for JWT validation result (contains claims)
 * @return MHD_YES on success, MHD_NO on validation failure
 */
enum MHD_Result validate_jwt_from_header(
    struct MHD_Connection *connection,
    jwt_validation_result_t **jwt_result)
{
    log_this(SR_AUTH, "validate_jwt_from_header: Starting JWT validation", LOG_LEVEL_TRACE, 0);
    
    if (!connection || !jwt_result) {
        log_this(SR_AUTH, "validate_jwt_from_header: NULL parameters (connection=%p, jwt_result=%p)",
                 LOG_LEVEL_ERROR, 2, (void*)connection, (void*)jwt_result);
        return MHD_NO;
    }

    // Get the Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    
    if (!auth_header) {
        log_this(SR_AUTH, "validate_jwt_from_header: Missing Authorization header", LOG_LEVEL_ERROR, 0);
        log_this(SR_AUTH, "validate_jwt_from_header: Sending 401 - Missing Authorization header", LOG_LEVEL_TRACE, 0);
        return send_missing_authorization_response(connection);
    }
    
    // Check for "Bearer " prefix
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        log_this(SR_AUTH, "validate_jwt_from_header: Invalid Authorization header format (does not start with 'Bearer ')", LOG_LEVEL_ERROR, 0);
        log_this(SR_AUTH, "validate_jwt_from_header: Sending 401 - Invalid Authorization format", LOG_LEVEL_TRACE, 0);
        return send_invalid_authorization_format_response(connection);
    }
    
    // Extract token (skip "Bearer " prefix)
    const char *token = auth_header + 7;
    log_this(SR_AUTH, "validate_jwt_from_header: Extracted token (length=%zu)", LOG_LEVEL_TRACE, 1, strlen(token));

    // Use helper function to validate JWT
    log_this(SR_AUTH, "validate_jwt_from_header: Calling extract_and_validate_jwt()", LOG_LEVEL_TRACE, 0);
    jwt_validation_result_t result = {0};
    bool valid = extract_and_validate_jwt(auth_header, &result);
    
    if (!valid) {
        const char *error_msg = get_jwt_error_message(result.error);
        unsigned int http_status = MHD_HTTP_UNAUTHORIZED;
        
        log_this(SR_AUTH, "validate_jwt_from_header: JWT validation failed - error_code=%d, msg=%s",
                 LOG_LEVEL_ALERT, 2, result.error, error_msg);

        log_this(SR_AUTH, "validate_jwt_from_header: Sending %d - %s", LOG_LEVEL_TRACE, 2, http_status, error_msg);
        return send_jwt_error_response(connection, error_msg, http_status);
    }

    log_this(SR_AUTH, "validate_jwt_from_header: JWT is valid, checking claims", LOG_LEVEL_TRACE, 0);

    // Validate claims using helper
    if (!validate_jwt_claims(&result, connection)) {
        // Error response already sent by helper
        free_jwt_validation_result(&result);
        return MHD_NO;
    }

    log_this(SR_AUTH, "validate_jwt_from_header: JWT database claim: '%s'", LOG_LEVEL_TRACE, 1, result.claims->database);

    // Return the JWT result for use by caller
    *jwt_result = malloc(sizeof(jwt_validation_result_t));
    if (!*jwt_result) {
        log_this(SR_AUTH, "validate_jwt_from_header: Failed to allocate JWT result", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);
        log_this(SR_AUTH, "validate_jwt_from_header: Sending 500 - Allocation failed", LOG_LEVEL_TRACE, 0);
        return send_internal_server_error_response(connection);
    }
    **jwt_result = result; // Copy the struct

    log_this(SR_AUTH, "validate_jwt_from_header: JWT validated successfully, database='%s'", LOG_LEVEL_DEBUG, 1, result.claims->database);
    return MHD_YES;
}

void cleanup_auth_query_resources(
    json_t *request_json,
    jwt_validation_result_t *jwt_result,
    char *query_id,
    ParameterList *param_list,
    char *converted_sql,
    TypedParameter **ordered_params,
    size_t param_count,
    char *message)
{
    if (request_json) json_decref(request_json);
    if (jwt_result) {
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
    }
    if (query_id) free(query_id);
    if (param_list) free_parameter_list(param_list);
    if (converted_sql) free(converted_sql);
    if (ordered_params) {
        for (size_t i = 0; i < param_count; i++) {
            if (ordered_params[i]) {
                free_typed_parameter(ordered_params[i]);
            }
        }
        free(ordered_params);
    }
    if (message) free(message);
}

/**
 * @brief Handle authenticated conduit query request
 *
 * Main handler for the /api/conduit/auth_query endpoint. Validates JWT token,
 * extracts database from token claims, and executes the requested query.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (POST only)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_auth_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    // cppcheck-suppress[constParameterPointer] - upload_data_size parameter must match libmicrohttpd callback signature
    size_t *upload_data_size,
    void **con_cls)
{
    (void)url;

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Starting request processing", LOG_LEVEL_TRACE, 0);
    
    // Validate required parameters
    if (!connection || !method || !upload_data_size) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: NULL parameters (connection=%p, method=%p, upload_data_size=%p)", 
                 LOG_LEVEL_ERROR, 3, (void*)connection, (void*)method, (void*)upload_data_size);
        return MHD_NO;
    }

    // Use common POST body buffering (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    enum MHD_Result buffer_result = handle_auth_query_buffer_result(connection, buf_result, con_cls);
    if (buf_result != API_BUFFER_COMPLETE) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Buffer processing complete (non-OK result), returning", LOG_LEVEL_TRACE, 0);
        return buffer_result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Processing authenticated query request", LOG_LEVEL_DEBUG, 0);

    // Step 1: Validate HTTP method (POST only)
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 1 - Validating HTTP method", LOG_LEVEL_TRACE, 0);
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Method validation failed", LOG_LEVEL_TRACE, 0);
        api_free_post_buffer(con_cls);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: HTTP method validation passed", LOG_LEVEL_TRACE, 0);

    // Step 2: Parse request data from buffer
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 2 - Parsing request data", LOG_LEVEL_TRACE, 0);
    json_t *request_json = NULL;
    
    // Check if buffer is NULL (can happen in some test scenarios)
    if (!buffer) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Buffer is NULL, cannot parse request", LOG_LEVEL_ERROR, 0);
        api_free_post_buffer(con_cls);
        return handle_buffer_null_case(connection);
    }
    
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);

    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Request parsing failed", LOG_LEVEL_TRACE, 0);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Request parsing succeeded", LOG_LEVEL_TRACE, 0);

    // Step 3: Extract and validate required fields
    // For auth_query, we only need query_ref and params from the request body
    // The database comes from JWT claims, not the request
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 3 - Extracting fields", LOG_LEVEL_TRACE, 0);
    int query_ref = 0;
    json_t* params_json = NULL;
    
    enum MHD_Result extraction_result = handle_auth_query_field_extraction(connection, request_json, &query_ref, &params_json);
    if (extraction_result != MHD_YES) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Missing or invalid query_ref", LOG_LEVEL_TRACE, 0);
        json_decref(request_json);
        return MHD_YES; // Response sent by helper
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Fields extracted: query_ref=%d", LOG_LEVEL_TRACE, 1, query_ref);

    // Step 4: Validate JWT token from Authorization header
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 4 - Validating JWT", LOG_LEVEL_TRACE, 0);
    jwt_validation_result_t *jwt_result = NULL;
    result = validate_jwt_from_header(connection, &jwt_result);
    if (result != MHD_YES) {
        // Error response already sent by validate_jwt_from_header
        log_this(SR_AUTH, "handle_conduit_auth_query_request: JWT validation failed, response already sent", LOG_LEVEL_TRACE, 0);
        json_decref(request_json);
        return MHD_YES;  // Response was sent
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: JWT validation succeeded", LOG_LEVEL_TRACE, 0);

    // Get database from JWT claims
    const char* jwt_database = jwt_result->claims->database;
    
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Using database from JWT: '%s'", LOG_LEVEL_TRACE, 1, jwt_database);
    log_this(SR_AUTH, "handle_conduit_auth_query_request: global_queue_manager=%p", LOG_LEVEL_TRACE, 1, (void*)global_queue_manager);

    // Step 5: Look up database queue and query cache entry
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 5 - Looking up database '%s'", LOG_LEVEL_TRACE, 1, jwt_database);
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    bool query_not_found = false;
    
    // Use database from JWT claims for auth_query (not require_public - any query is allowed with auth)
    result = handle_database_lookup(connection, jwt_database, query_ref, &db_queue, &cache_entry, &query_not_found, false);
    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Database lookup returned error", LOG_LEVEL_TRACE, 0);
        cleanup_auth_query_resources(request_json, jwt_result, NULL, NULL, NULL, NULL, 0, NULL);
        return result;
    }

    // Handle invalid queryref case
    if (query_not_found) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Query not found (query_ref=%d), building invalid queryref response", LOG_LEVEL_TRACE, 1, query_ref);
        json_t* response = build_invalid_queryref_response(query_ref, jwt_database, NULL);
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Sending invalid queryref response", LOG_LEVEL_TRACE, 0);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(response);
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Invalid queryref response sent, returning", LOG_LEVEL_TRACE, 0);
        return http_result;
    }

    // Check if database lookup failed
    if (!db_queue) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Database lookup succeeded but db_queue is NULL", LOG_LEVEL_ERROR, 0);
        enum MHD_Result response_result = send_database_not_found_response(connection, jwt_database);
        cleanup_auth_query_resources(request_json, jwt_result, NULL, NULL, NULL, NULL, 0, NULL);
        return response_result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Database and query lookup successful (db_queue=%p, cache_entry=%p)", 
             LOG_LEVEL_TRACE, 2, (void*)db_queue, (void*)cache_entry);

    // Step 6: Process parameters
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 6 - Processing parameters", LOG_LEVEL_TRACE, 0);
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;
    
    result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                        jwt_database, query_ref, &param_list, &converted_sql,
                                        &ordered_params, &param_count, &message);
    
    if (result != MHD_YES) {
        // Error response already sent by helper, or parameter error - cleanup and return
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Parameter processing failed (result=%d, converted_sql=%p)", 
                 LOG_LEVEL_TRACE, 2, result, (void*)converted_sql);
        cleanup_auth_query_resources(request_json, jwt_result, NULL, param_list, converted_sql, ordered_params, param_count, message);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Parameter processing succeeded", LOG_LEVEL_TRACE, 0);

    // Step 7: Select appropriate queue
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 7 - Selecting queue", LOG_LEVEL_TRACE, 0);
    DatabaseQueue *selected_queue = NULL;
    result = handle_queue_selection(connection, jwt_database, query_ref, cache_entry,
                                   param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Queue selection failed", LOG_LEVEL_TRACE, 0);
        cleanup_auth_query_resources(request_json, jwt_result, NULL, param_list, converted_sql, ordered_params, param_count, message);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Queue selection succeeded (selected_queue=%p)", LOG_LEVEL_TRACE, 1, (void*)selected_queue);

    // Step 8: Generate query ID
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 8 - Generating query ID", LOG_LEVEL_TRACE, 0);
    char *query_id = NULL;
    result = handle_query_id_generation(connection, jwt_database, query_ref, param_list,
                                       converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Query ID generation failed", LOG_LEVEL_TRACE, 0);
        cleanup_auth_query_resources(request_json, jwt_result, NULL, param_list, converted_sql, ordered_params, param_count, message);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Query ID generated: %s", LOG_LEVEL_TRACE, 1, query_id ? query_id : "(null)");

    // Step 9: Register pending result
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 9 - Registering pending result", LOG_LEVEL_TRACE, 0);
    PendingQueryResult *pending = NULL;
    result = handle_pending_registration(connection, jwt_database, query_ref, query_id,
                                        param_list, converted_sql, ordered_params,
                                        cache_entry, &pending);
    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Pending registration failed", LOG_LEVEL_TRACE, 0);
        cleanup_auth_query_resources(request_json, jwt_result, query_id, param_list, converted_sql, ordered_params, param_count, message);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Pending registration succeeded", LOG_LEVEL_TRACE, 0);

    // Step 10: Submit query to database queue
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 10 - Submitting query", LOG_LEVEL_TRACE, 0);
    result = handle_query_submission(connection, jwt_database, query_ref, selected_queue, query_id,
                                    converted_sql, param_list, ordered_params, param_count, cache_entry);
    if (result != MHD_YES) {
        // Error response already sent by helper
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Query submission failed", LOG_LEVEL_TRACE, 0);
        cleanup_auth_query_resources(request_json, jwt_result, query_id, param_list, converted_sql, ordered_params, param_count, message);
        return result;
    }
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Query submission succeeded", LOG_LEVEL_TRACE, 0);

    // Step 11: Wait for result and build response
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Step 11 - Building response", LOG_LEVEL_TRACE, 0);
    result = handle_response_building(connection, query_ref, jwt_database, cache_entry,
                                     selected_queue, pending, query_id, converted_sql,
                                     param_list, ordered_params, message);
    
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Response building returned %d", LOG_LEVEL_TRACE, 1, result);

    // Clean up
    log_this(SR_AUTH, "handle_conduit_auth_query_request: Cleaning up resources", LOG_LEVEL_TRACE, 0);
    cleanup_auth_query_resources(request_json, jwt_result, query_id, param_list, converted_sql, ordered_params, param_count, message);

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Request completed with result=%d", LOG_LEVEL_DEBUG, 1, result);
    return result;
}
