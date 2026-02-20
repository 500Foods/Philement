/**
 * @file auth_queries.c
 * @brief Authenticated Conduit Queries API endpoint implementation
 *
 * This module implements the authenticated database queries execution endpoint.
 * It validates JWT tokens before executing multiple queries in parallel and extracts
 * the database name from the JWT claims for secure routing.
 *
 * @author Hydrogen Framework
 * @date 2026-01-30
 */

#include <src/hydrogen.h>

// Network headers
#include <microhttpd.h>

// Standard C libraries
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include <src/api/api_service.h>
#include <src/api/api_utils.h>  // For api_send_json_response
#include <src/api/conduit/query/query.h>  // Use existing helpers
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/config/config_databases.h>
#include <src/logging/logging.h>

// Local includes
#include "../conduit_service.h"
#include "../conduit_helpers.h"
#include "../helpers/auth_jwt_helper.h"
#include "../helpers/queries_response_helpers.h"
#include "auth_queries.h"

/**
 * @brief Deduplicate queries and validate rate limits
 *
 * Processes the queries array to remove duplicates by query_ref and validates
 * against the MaxQueriesPerRequest limit for the specified database.
 *
 * @param connection The HTTP connection for error responses
 * @param queries_array The input queries array from the request
 * @param database The database name to check rate limits against
 * @param deduplicated_queries Output array of unique query objects (caller must decref)
 * @param mapping_array Output array mapping original indices to deduplicated indices
 * @param is_duplicate Output array tracking which original queries are duplicates
 * @param result_code Output parameter for deduplication result code
 * @return MHD_YES on success, MHD_NO on rate limit exceeded or error
 */
enum MHD_Result auth_queries_deduplicate_and_validate(
    struct MHD_Connection *connection,
    json_t *queries_array,
    const char *database,
    json_t **deduplicated_queries,
    size_t **mapping_array,
    bool **is_duplicate,
    DeduplicationResult *result_code)
{
    (void)connection;  // Unused parameter
    
    if (!queries_array || !database || !deduplicated_queries || !mapping_array || !is_duplicate) {
        log_this(SR_AUTH, "deduplicate_and_validate_queries: NULL parameters", LOG_LEVEL_ERROR, 0);
        if (result_code) {
            *result_code = DEDUP_ERROR;
        }
        return MHD_NO;
    }

    size_t original_count = json_array_size(queries_array);
    if (original_count == 0) {
        *deduplicated_queries = json_array();
        *mapping_array = NULL;
        *is_duplicate = NULL;
        if (result_code) {
            *result_code = DEDUP_OK;
        }
        return MHD_YES;
    }

    // Validate database connection first
    const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
    if (!db_conn) {
        // If connection not found by database name, try to find it by checking if database name matches any connection name
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection *conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->connection_name && strcmp(conn->connection_name, database) == 0) {
                db_conn = conn;
                break;
            }
        }
        if (!db_conn) {
            log_this(SR_AUTH, "deduplicate_and_validate_queries: Database connection not found: %s", LOG_LEVEL_ALERT, 1, database);
            if (result_code) {
                *result_code = DEDUP_DATABASE_NOT_FOUND;
            }
            return MHD_NO;
        }
    }

    // Allocate memory for duplicate tracking
    *is_duplicate = calloc(original_count, sizeof(bool));
    if (!*is_duplicate) {
        log_this(SR_AUTH, "deduplicate_and_validate_queries: Failed to allocate memory for duplicate tracking", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Create arrays to track unique queries (query_ref + params) and their mapping
    int *query_refs = calloc(original_count, sizeof(int));
    json_t **query_params = calloc(original_count, sizeof(json_t*));
    size_t *first_occurrence = calloc(original_count, sizeof(size_t));
    size_t unique_count = 0;

    if (!query_refs || !query_params || !first_occurrence) {
        log_this(SR_AUTH, "deduplicate_and_validate_queries: Failed to allocate memory", LOG_LEVEL_ERROR, 0);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        return MHD_NO;
    }

    // First pass: collect unique queries (query_ref + params) and track duplicates
    for (size_t i = 0; i < original_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        if (!query_obj || !json_is_object(query_obj)) {
            (*is_duplicate)[i] = true;
            continue;
        }

        json_t *query_ref_json = json_object_get(query_obj, "query_ref");
        if (!query_ref_json || !json_is_integer(query_ref_json)) {
            (*is_duplicate)[i] = true;
            continue;
        }

        int query_ref = (int)json_integer_value(query_ref_json);
        json_t *params = json_object_get(query_obj, "params");
        if (!params) {
            params = json_object();  // Treat missing params as empty object
        }

        // Check if we've seen this query_ref + params combination before
        bool found = false;
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref) {
                // Compare params
                int cmp = json_equal(query_params[j], params);
                if (cmp == 1) {
                    found = true;
                    (*is_duplicate)[i] = true;
                    break;
                }
            }
        }

        if (!found) {
            query_refs[unique_count] = query_ref;
            query_params[unique_count] = params;  // Reference to original, no decref needed
            first_occurrence[unique_count] = i;
            unique_count++;
        }
    }

    // Check rate limit
    if ((int)unique_count > db_conn->max_queries_per_request) {
        log_this(SR_AUTH, "deduplicate_and_validate_queries: Rate limit exceeded: %zu unique queries > %d max for database %s",
                 LOG_LEVEL_ERROR, 3, unique_count, db_conn->max_queries_per_request, database);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        if (result_code) {
            *result_code = DEDUP_RATE_LIMIT;
        }
        // Reset output parameters to NULL
        *deduplicated_queries = NULL;
        *mapping_array = NULL;
        *is_duplicate = NULL;
        return MHD_NO;
    }

    // Create deduplicated queries array
    *deduplicated_queries = json_array();
    *mapping_array = calloc(original_count, sizeof(size_t));

    if (!*deduplicated_queries || !*mapping_array) {
        log_this(SR_AUTH, "deduplicate_and_validate_queries: Failed to allocate output arrays", LOG_LEVEL_ERROR, 0);
        json_decref(*deduplicated_queries);
        free(*mapping_array);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        return MHD_NO;
    }

    // Second pass: build deduplicated array and mapping
    for (size_t i = 0; i < original_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        if (!query_obj || !json_is_object(query_obj)) {
            continue;
        }

        json_t *query_ref_json = json_object_get(query_obj, "query_ref");
        if (!query_ref_json || !json_is_integer(query_ref_json)) {
            continue;
        }

        int query_ref = (int)json_integer_value(query_ref_json);
        json_t *params = json_object_get(query_obj, "params");
        if (!params) {
            params = json_object();
        }

        // Find the deduplicated index for this query_ref + params
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref && json_equal(query_params[j], params)) {
                (*mapping_array)[i] = j;

                // Add to deduplicated array if this is the first occurrence
                if (first_occurrence[j] == i) {
                    json_array_append(*deduplicated_queries, query_obj);
                }
                break;
            }
        }
    }

    log_this(SR_AUTH, "deduplicate_and_validate_queries: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_count, unique_count);

    free(query_refs);
    free(query_params);
    free(first_occurrence);

    if (result_code) {
        *result_code = DEDUP_OK;
    }
    
    return MHD_YES;
}

/**
 * @brief Validate JWT token from Authorization header and extract database name
 *
 * Validates the provided JWT token from the Authorization header and extracts
 * the database name from the token claims for authenticated database routing.
 *
 * @param connection The HTTP connection for error responses
 * @param database Output parameter for extracted database name (caller must free)
 * @return MHD_YES on success, MHD_NO on validation failure
 */
enum MHD_Result validate_jwt_and_extract_database(
    struct MHD_Connection *connection,
    char **database)
{
    if (!connection || !database) {
        log_this(SR_AUTH, "validate_jwt_and_extract_database: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Get the Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    
    if (!auth_header) {
        log_this(SR_AUTH, "validate_jwt_and_extract_database: Missing Authorization header", LOG_LEVEL_ERROR, 0);
        return send_missing_authorization_response(connection);
    }
    
    // Check for "Bearer " prefix
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        log_this(SR_AUTH, "validate_jwt_and_extract_database: Invalid Authorization header format", LOG_LEVEL_ERROR, 0);
        return send_invalid_authorization_format_response(connection);
    }

    // Use helper function to validate JWT
    jwt_validation_result_t result = {0};
    bool valid = extract_and_validate_jwt(auth_header, &result);
    
    if (!valid) {
        const char *error_msg = get_jwt_error_message(result.error);
        log_this(SR_AUTH, "validate_jwt_and_extract_database: JWT validation failed - %s", LOG_LEVEL_ALERT, 1, error_msg);
        free_jwt_validation_result(&result);
        return send_jwt_error_response(connection, error_msg, MHD_HTTP_UNAUTHORIZED);
    }

    // Validate claims using helper
    if (!validate_jwt_claims(&result, connection)) {
        // Error response already sent by helper
        free_jwt_validation_result(&result);
        return MHD_NO;
    }

    // Extract database from JWT claims
    if (!result.claims || !result.claims->database || strlen(result.claims->database) == 0) {
        log_this(SR_AUTH, "validate_jwt_and_extract_database: No database in JWT claims", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);
        return send_conduit_error_response(connection, "JWT token missing database information", MHD_HTTP_UNAUTHORIZED);
    }

    // Allocate and copy database name
    *database = strdup(result.claims->database);
    if (!*database) {
        log_this(SR_AUTH, "validate_jwt_and_extract_database: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);
        return send_internal_server_error_response(connection);
    }

    log_this(SR_AUTH, "validate_jwt_and_extract_database: JWT validated, database=%s", LOG_LEVEL_DEBUG, 1, *database);
    free_jwt_validation_result(&result);
    return MHD_YES;
}

/**
 * @brief Execute a single authenticated query using existing helpers
 *
 * Executes a single authenticated query from the queries array using the existing
 * conduit infrastructure helpers from query.h.
 *
 * @param database Database name from JWT token
 * @param query_obj Query object containing query_ref and optional params
 * @return JSON object with query result or error
 */
/**
 * @brief Cleanup auth queries resources
 *
 * Frees all resources associated with an auth queries request.
 * Safe to call with NULL values.
 *
 * @param request_json Request JSON object to free
 * @param database Database name to free
 * @param queries_array Queries array to free
 * @param deduplicated_queries Deduplicated queries array to free
 * @param mapping_array Mapping array to free
 * @param is_duplicate Is duplicate array to free
 * @param unique_results Unique results array to free
 * @param unique_query_count Number of unique queries
 */
void cleanup_auth_queries_resources(
    json_t *request_json,
    char *database,
    json_t *queries_array,
    json_t *deduplicated_queries,
    size_t *mapping_array,
    bool *is_duplicate,
    json_t **unique_results,
    size_t unique_query_count)
{
    if (request_json) json_decref(request_json);
    if (database) free(database);
    if (queries_array) json_decref(queries_array);
    if (deduplicated_queries) json_decref(deduplicated_queries);
    if (mapping_array) free(mapping_array);
    if (is_duplicate) free(is_duplicate);
    if (unique_results) {
        for (size_t i = 0; i < unique_query_count; i++) {
            if (unique_results[i]) {
                json_decref(unique_results[i]);
            }
        }
        free(unique_results);
    }
}

json_t* execute_single_auth_query(const char *database, json_t *query_obj)
{
    if (!database || !query_obj) {
        log_this(SR_AUTH, "execute_single_auth_query: NULL parameters", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Invalid query object"));
        return error_result;
    }

    // Extract query_ref
    json_t *query_ref_json = json_object_get(query_obj, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_AUTH, "execute_single_auth_query: Missing or invalid query_ref", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Missing required field: query_ref"));
        return error_result;
    }

    int query_ref = (int)json_integer_value(query_ref_json);
    json_t *params = json_object_get(query_obj, "params");

    // Look up database queue and cache entry using helper function
    // For auth_queries, we don't require public queries (require_public = false)
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    
    if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
        const char *error_msg = !db_queue ? "Database not available" : "Query not found";
        const char *message = !db_queue ? "Database is not available" : NULL;
        json_t *error_result = create_lookup_error_response(error_msg, database, query_ref, !db_queue, message);
        return error_result;
    }

    // Process parameters with validation using helper function
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    char* message = process_query_parameters(params, cache_entry, db_queue, &param_list,
                                            &converted_sql, &ordered_params, &param_count);
    if (message) {
        // Parameter processing failed - message contains error details
        json_t *error_response = create_processing_error_response("Parameter processing failed", database, query_ref);
        json_object_set_new(error_response, "message", json_string(message));
        free(message);
        return error_response;
    }

    // Select queue with error handling
    DatabaseQueue *selected_queue = select_query_queue_with_error_handling(database, cache_entry,
                                                                          converted_sql, param_list,
                                                                          ordered_params, message);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("No suitable queue available", database, query_ref);
    }

    // Generate query ID with error handling
    char *query_id = generate_query_id_with_error_handling(converted_sql, param_list,
                                                          ordered_params, message);
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to generate query ID", database, query_ref);
    }

    // Register pending result with error handling
    PendingQueryResult *pending = register_pending_result_with_error_handling(query_id, cache_entry,
                                                                              converted_sql, param_list,
                                                                              ordered_params, message);
    if (!pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to register pending result", database, query_ref);
    }

    // Submit query with error handling
    if (!submit_query_with_error_handling(selected_queue, query_id, cache_entry, ordered_params,
                                         param_count, converted_sql, param_list, message)) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to submit query", database, query_ref);
    }

    // Build response using helper function
    json_t *result = build_response_json(query_ref, database, cache_entry, selected_queue, pending, message);

    // Clean up
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);
    if (message) free(message);

    log_this(SR_AUTH, "execute_single_auth_query: Query completed, query_ref=%d",
             LOG_LEVEL_DEBUG, 1, query_ref);

    return result;
}

/**
 * @brief Handle authenticated conduit queries request
 *
 * Main handler for the /api/conduit/auth_queries endpoint. Validates JWT token
 * from Authorization header, extracts database from token claims, and executes 
 * multiple queries in parallel.
 *
 * Request format (POST JSON):
 * {
 *   "queries": [
 *     {
 *       "query_ref": 1234,
 *       "params": { "INTEGER": {...}, "STRING": {...} }
 *     },
 *     {
 *       "query_ref": 5678,
 *       "params": { "INTEGER": {...}, "STRING": {...} }
 *     }
 *   ]
 * }
 *
 * Response format:
 * {
 *   "success": true,
 *   "results": [ {...}, {...} ],
 *   "database": "database_name",
 *   "total_execution_time_ms": 123
 * }
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (POST only)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_auth_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;

    log_this(SR_AUTH, "handle_conduit_auth_queries_request: Processing authenticated queries request", LOG_LEVEL_DEBUG, 0);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Use common POST body buffering (handles POST requests)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, (size_t*)upload_data_size, con_cls, &buffer);

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
            // All data received, continue with processing
            break;
    }

    log_this(SR_AUTH, "%s: Step 1 - Validate HTTP method", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 1: Validate HTTP method using helper function
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        log_this(SR_AUTH, "%s: Method validation failed", LOG_LEVEL_ERROR, 1, conduit_service_name());
        return result;
    }

    log_this(SR_AUTH, "%s: Step 2 - Parse request", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 2: Parse request using helper function
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);

    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) {
        log_this(SR_AUTH, "%s: Request parsing failed", LOG_LEVEL_ERROR, 1, conduit_service_name());
        return result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_queries_request: Step 3 - Validate JWT and extract database", LOG_LEVEL_DEBUG, 0);

    // Step 3: Validate JWT token from Authorization header and extract database
    char *database = NULL;
    result = validate_jwt_and_extract_database(connection, &database);
    
    if (result != MHD_YES) {
        // Error response already sent by validate_jwt_and_extract_database
        json_decref(request_json);
        return result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_queries_request: Database extracted from JWT: %s", LOG_LEVEL_DEBUG, 1, database);

    log_this(SR_AUTH, "handle_conduit_auth_queries_request: Step 4 - Extract queries array", LOG_LEVEL_DEBUG, 0);

    // Step 4: Extract queries array
    json_t *queries_array = json_object_get(request_json, "queries");
    if (!queries_array || !json_is_array(queries_array)) {
        log_this(SR_AUTH, "handle_conduit_auth_queries_request: Missing or invalid queries field", LOG_LEVEL_ERROR, 0);
        free(database);
        json_decref(request_json);
        return send_conduit_error_response(connection, "Missing required parameter: queries (must be array)", MHD_HTTP_BAD_REQUEST);
    }

    size_t original_query_count = json_array_size(queries_array);
    if (original_query_count == 0) {
        log_this(SR_AUTH, "handle_conduit_auth_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        json_object_set_new(error_response, "results", json_array());
        json_object_set_new(error_response, "database", json_string(database));
        json_object_set_new(error_response, "total_execution_time_ms", json_integer(0));
        
        free(database);
        json_decref(request_json);
        
        return api_send_json_response(connection, error_response, MHD_HTTP_OK);
    }

    log_this(SR_AUTH, "%s: Step 5 - Deduplicate queries and validate rate limits", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 5: Deduplicate queries and validate rate limits
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;
    enum MHD_Result dedup_result = auth_queries_deduplicate_and_validate(connection, queries_array, database, 
                                                                    &deduplicated_queries, &mapping_array, 
                                                                    &is_duplicate, &dedup_code);
    
    bool rate_limit_exceeded = false;
    int max_queries_per_request = 0;
    size_t unique_query_count = 0;
    
    if (dedup_result != MHD_YES) {
        log_this(SR_AUTH, "%s: Validation failed with code %d", LOG_LEVEL_ERROR, 2, conduit_service_name(), dedup_code);
        
        if (dedup_code == DEDUP_RATE_LIMIT) {
            // Get the max queries limit for the database
            const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
            if (db_conn) {
                max_queries_per_request = db_conn->max_queries_per_request;
            }
            rate_limit_exceeded = true;
            
            // When rate limit is exceeded, we still want to execute queries up to the limit
            unique_query_count = (size_t)max_queries_per_request;
            
            // Create deduplicated_queries array with only the queries up to the limit
            deduplicated_queries = json_array();
            for (size_t i = 0; i < unique_query_count && i < original_query_count; i++) {
                json_t *query = json_array_get(queries_array, i);
                json_array_append_new(deduplicated_queries, json_deep_copy(query));
            }
            
            // Create mapping array and is_duplicate array
            mapping_array = malloc(original_query_count * sizeof(size_t));
            is_duplicate = malloc(original_query_count * sizeof(bool));
            
            for (size_t i = 0; i < original_query_count; i++) {
                if (i < unique_query_count) {
                    mapping_array[i] = i;
                    is_duplicate[i] = false;
                } else {
                    mapping_array[i] = 0;
                    is_duplicate[i] = true;
                }
            }
        } else {
            // For other validation errors, use helper to build error response
            json_t *error_response = build_dedup_error_json(dedup_code, database, 0);
            unsigned int http_status = get_dedup_http_status(dedup_code);
            free(database);
            json_decref(request_json);
            return api_send_json_response(connection, error_response, http_status);
        }
    } else {
        // Normal case - deduplication succeeded
        unique_query_count = json_array_size(deduplicated_queries);
    }

    log_this(SR_AUTH, "%s: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 3, conduit_service_name(), original_query_count, unique_query_count);

    log_this(SR_AUTH, "%s: Step 6 - Execute queries", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 6: Execute all unique queries
    json_t *results_array = json_array();
    bool all_success = true;

    // Execute unique queries and store results
    json_t **unique_results = calloc(unique_query_count, sizeof(json_t*));
    
    if (!unique_results) {
        log_this(SR_AUTH, "%s: Failed to allocate memory for query results", LOG_LEVEL_ERROR, 1, conduit_service_name());
        json_decref(deduplicated_queries);
        free(mapping_array);
        free(is_duplicate);
        free(database);
        json_decref(request_json);
        return send_conduit_error_response(connection, "Internal server error", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    for (size_t i = 0; i < unique_query_count; i++) {
        log_this(SR_AUTH, "%s: Executing unique query %zu", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
        
        json_t *query_obj = json_array_get(deduplicated_queries, i);
        json_t *query_result = execute_single_auth_query(database, query_obj);
        
        if (!query_result) {
            log_this(SR_AUTH, "%s: Failed to execute unique query %zu", LOG_LEVEL_ERROR, 2, conduit_service_name(), i);
            query_result = json_object();
            json_object_set_new(query_result, "success", json_false());
            json_object_set_new(query_result, "error", json_string("Query execution failed"));
            all_success = false;
        } else {
            // Check if query succeeded
            json_t *success_field = json_object_get(query_result, "success");
            if (!success_field || !json_is_true(success_field)) {
                all_success = false;
                log_this(SR_AUTH, "%s: Unique query %zu failed", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
            }
        }
        
        unique_results[i] = query_result;
        log_this(SR_AUTH, "%s: Unique query %zu completed", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
    }
    
    // Map results back to original query order using helper functions
    for (size_t i = 0; i < original_query_count; i++) {
        if (rate_limit_exceeded && (int)i >= max_queries_per_request) {
            json_array_append_new(results_array, build_rate_limit_result_entry(max_queries_per_request));
            all_success = false;
        } else if (is_duplicate[i]) {
            json_array_append_new(results_array, build_duplicate_result_entry());
            all_success = false;
        } else {
            if (mapping_array[i] < unique_query_count) {
                json_array_append_new(results_array, unique_results[mapping_array[i]]);
            } else {
                json_array_append_new(results_array, build_invalid_mapping_result_entry());
                all_success = false;
            }
        }
    }
    
    // Clean up
    free(unique_results);
    json_decref(deduplicated_queries);
    free(mapping_array);
    free(is_duplicate);
    json_decref(request_json);

    // Step 7: Calculate total execution time
    log_this(SR_AUTH, "%s: Calculating total execution time", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 8: Determine appropriate HTTP status code using helper
    log_this(SR_AUTH, "%s: Determining HTTP status code", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    unsigned int http_status = MHD_HTTP_OK;
    
    if (!all_success) {
        http_status = determine_queries_http_status(results_array, original_query_count);
    }

    // Step 9: Build response
    log_this(SR_AUTH, "%s: Building final response object", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    json_t *response_obj = json_object();
    if (!response_obj) {
        log_this(SR_AUTH, "%s: ERROR - Failed to create response object", LOG_LEVEL_ERROR, 1, conduit_service_name());
        json_decref(results_array);
        free(database);
        return MHD_NO;
    }

    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    log_this(SR_AUTH, "%s: Request completed, queries=%zu, time=%ldms, status=%d",
             LOG_LEVEL_DEBUG, 4, conduit_service_name(), original_query_count, total_time_ms, http_status);

    log_this(SR_AUTH, "%s: Calling api_send_json_response", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    enum MHD_Result send_result = api_send_json_response(connection, response_obj, http_status);
    log_this(SR_AUTH, "%s: api_send_json_response returned %d", LOG_LEVEL_DEBUG, 1, conduit_service_name(), send_result);

    free(database);

    return send_result;
}
