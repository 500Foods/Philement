/**
 * @file queries.c
 * @brief Public Conduit Queries API endpoint implementation
 *
 * This module implements the public database queries execution endpoint.
 * It executes multiple pre-defined queries in parallel without authentication,
 * requiring an explicit database parameter in the request.
 *
 * @author Hydrogen Framework
 * @date 2026-01-14
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
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/query/query.h>  // Use existing helpers
#include <src/config/config.h>
#include <src/config/config_databases.h>
#include <src/logging/logging.h>

/**
 * @brief Deduplicate queries and validate rate limits
 *
 * Processes the queries array to remove duplicates by query_ref and validates
 * against the MaxQueriesPerRequest limit for the specified database.
 *
 * @param queries_array The input queries array from the request
 * @param database The database name to check rate limits against
 * @param deduplicated_queries Output array of unique query objects (caller must decref)
 * @param mapping_array Output array mapping original indices to deduplicated indices
 * @return MHD_YES on success, MHD_NO on rate limit exceeded
 */
enum MHD_Result deduplicate_and_validate_queries(
    json_t *queries_array,
    const char *database,
    json_t **deduplicated_queries,
    size_t **mapping_array)
{
    if (!queries_array || !database || !deduplicated_queries || !mapping_array) {
        log_this(SR_API, "deduplicate_and_validate_queries: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    size_t original_count = json_array_size(queries_array);
    if (original_count == 0) {
        *deduplicated_queries = json_array();
        *mapping_array = NULL;
        return MHD_YES;
    }

    // Get database connection to check rate limits
    const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
    if (!db_conn) {
        // If connection not found by database name, try to find it by checking if database name matches any connection name
        // This handles cases where the database parameter is not the connection name
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection *conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->connection_name && strcmp(conn->connection_name, database) == 0) {
                db_conn = conn;
                break;
            }
        }
        if (!db_conn) {
            log_this(SR_API, "deduplicate_and_validate_queries: Database connection not found: %s, skipping rate limiting", LOG_LEVEL_ALERT, 1, database);
            // Skip rate limiting if connection not found
            *deduplicated_queries = json_array();
            *mapping_array = calloc(original_count, sizeof(size_t));
            if (!*deduplicated_queries || !*mapping_array) {
                json_decref(*deduplicated_queries);
                free(*mapping_array);
                return MHD_NO;
            }
            // Copy all queries without deduplication
            for (size_t i = 0; i < original_count; i++) {
                json_t *query_obj = json_array_get(queries_array, i);
                json_array_append(*deduplicated_queries, query_obj);
                (*mapping_array)[i] = i;
            }
            log_this(SR_API, "deduplicate_and_validate_queries: Skipped rate limiting for database %s", LOG_LEVEL_DEBUG, 1, database);
            return MHD_YES;
        }
    }

    // Create a hash map to track unique query_refs and their first occurrence index
    // Using a simple array-based approach since we expect small numbers of queries
    int *query_refs = calloc(original_count, sizeof(int));
    size_t *first_occurrence = calloc(original_count, sizeof(size_t));
    size_t unique_count = 0;

    if (!query_refs || !first_occurrence) {
        log_this(SR_API, "deduplicate_and_validate_queries: Failed to allocate memory", LOG_LEVEL_ERROR, 0);
        free(query_refs);
        free(first_occurrence);
        return MHD_NO;
    }

    // First pass: collect unique query_refs and track first occurrences
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

        // Check if we've seen this query_ref before
        bool found = false;
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref) {
                found = true;
                break;
            }
        }

        if (!found) {
            query_refs[unique_count] = query_ref;
            first_occurrence[unique_count] = i;
            unique_count++;
        }
    }

    // Check rate limit
    if ((int)unique_count > db_conn->max_queries_per_request) {
        log_this(SR_API, "deduplicate_and_validate_queries: Rate limit exceeded: %zu unique queries > %d max for database %s",
                 LOG_LEVEL_ERROR, 3, unique_count, db_conn->max_queries_per_request, database);
        free(query_refs);
        free(first_occurrence);
        return MHD_NO;
    }

    // Create deduplicated queries array
    *deduplicated_queries = json_array();
    *mapping_array = calloc(original_count, sizeof(size_t));

    if (!*deduplicated_queries || !*mapping_array) {
        log_this(SR_API, "deduplicate_and_validate_queries: Failed to allocate output arrays", LOG_LEVEL_ERROR, 0);
        json_decref(*deduplicated_queries);
        free(*mapping_array);
        free(query_refs);
        free(first_occurrence);
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

        // Find the deduplicated index for this query_ref
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref) {
                (*mapping_array)[i] = j;

                // Add to deduplicated array if this is the first occurrence
                if (first_occurrence[j] == i) {
                    json_array_append(*deduplicated_queries, query_obj);
                }
                break;
            }
        }
    }

    log_this(SR_API, "deduplicate_and_validate_queries: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_count, unique_count);

    free(query_refs);
    free(first_occurrence);

    return MHD_YES;
}

/**
 * @brief Execute a single query using existing helpers
 *
 * Executes a single query from the queries array using the existing
 * conduit infrastructure helpers from query.h.
 *
 * @param database Database name from request
 * @param query_obj Query object containing query_ref and optional params
 * @return JSON object with query result or error
 */
json_t* execute_single_query(const char *database, json_t *query_obj)
{
    if (!database || !query_obj) {
        log_this(SR_API, "execute_single_query: NULL parameters", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Invalid query object"));
        return error_result;
    }

    // Extract query_ref
    json_t *query_ref_json = json_object_get(query_obj, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_API, "execute_single_query: Missing or invalid query_ref", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Missing required field: query_ref"));
        return error_result;
    }

    int query_ref = (int)json_integer_value(query_ref_json);
    json_t *params = json_object_get(query_obj, "params");

    // Look up database queue and cache entry using helpers from query.h
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;

    if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
        const char *error_msg = !db_queue ? "Database not found" : "Query not found";
        json_t *error_result = create_lookup_error_response(error_msg, database, query_ref, !db_queue);
        return error_result;
    }

    // Process parameters using helper from query.h
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    if (!process_parameters(params, &param_list, cache_entry->sql_template,
                           db_queue->engine_type, &converted_sql, &ordered_params, &param_count)) {
        return create_processing_error_response("Parameter processing failed", database, query_ref);
    }

    // Select queue using helper from query.h
    DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("No suitable queue available", database, query_ref);
    }

    // Generate query ID using helper from query.h
    char *query_id = generate_query_id();
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to generate query ID", database, query_ref);
    }

    // Register pending result using helper from query.h
    PendingResultManager *pending_mgr = get_pending_result_manager();
    PendingQueryResult *pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds, NULL);
    if (!pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to register pending result", database, query_ref);
    }

    // Submit query using helper from query.h
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template, ordered_params,
                                  param_count, cache_entry)) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        return create_processing_error_response("Failed to submit query", database, query_ref);
    }

    // Build response using helper from query.h
    json_t *result = build_response_json(query_ref, database, cache_entry, selected_queue, pending);

    // Clean up
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);

    log_this(SR_API, "execute_single_query: Query completed, query_ref=%d",
             LOG_LEVEL_DEBUG, 1, query_ref);

    return result;
}

/**
 * @brief Submit a single query without waiting for completion
 *
 * Submits a single query from the queries array and returns the pending result
 * for later waiting. This enables parallel execution of multiple queries.
 *
 * @param database Database name from request
 * @param query_obj Query object containing query_ref and optional params
 * @param query_ref_out Output parameter for the query_ref (for error reporting)
 * @return PendingQueryResult on success, NULL on failure
 */
PendingQueryResult* submit_single_query(const char *database, json_t *query_obj, int *query_ref_out)
{
    if (!database || !query_obj || !query_ref_out) {
        log_this(SR_API, "submit_single_query: NULL parameters", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Extract query_ref
    json_t *query_ref_json = json_object_get(query_obj, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_API, "submit_single_query: Missing or invalid query_ref", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    int query_ref = (int)json_integer_value(query_ref_json);
    *query_ref_out = query_ref;
    json_t *params = json_object_get(query_obj, "params");

    // Look up database queue and cache entry using helpers from query.h
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;

    if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
        log_this(SR_API, "submit_single_query: Database or query lookup failed, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Process parameters using helper from query.h
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    if (!process_parameters(params, &param_list, cache_entry->sql_template,
                           db_queue->engine_type, &converted_sql, &ordered_params, &param_count)) {
        log_this(SR_API, "submit_single_query: Parameter processing failed, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Select queue using helper from query.h
    DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_API, "submit_single_query: No suitable queue available, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Generate query ID using helper from query.h
    char *query_id = generate_query_id();
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_API, "submit_single_query: Failed to generate query ID, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Register pending result using helper from query.h
    PendingResultManager *pending_mgr = get_pending_result_manager();
    PendingQueryResult *pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds, NULL);
    if (!pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_API, "submit_single_query: Failed to register pending result, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Submit query using helper from query.h
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template, ordered_params,
                                  param_count, cache_entry)) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_API, "submit_single_query: Failed to submit query, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Clean up resources that are no longer needed
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);

    log_this(SR_API, "submit_single_query: Query submitted, query_ref=%d", LOG_LEVEL_DEBUG, 1, query_ref);

    return pending;
}

/**
 * @brief Wait for a single query to complete and build response
 *
 * Waits for the pending result to complete and builds the JSON response.
 *
 * @param database Database name
 * @param query_ref Query reference ID
 * @param cache_entry Query cache entry
 * @param selected_queue Queue that was used
 * @param pending Pending result to wait for
 * @return JSON response object
 */
json_t* wait_and_build_single_response(const char *database, int query_ref,
                                      const QueryCacheEntry *cache_entry, const DatabaseQueue *selected_queue,
                                      PendingQueryResult *pending)
{
    log_this(SR_API, "wait_and_build_single_response: Waiting for query_ref=%d", LOG_LEVEL_DEBUG, 1, query_ref);

    // Build response using helper from query.h (this waits internally)
    json_t *result = build_response_json(query_ref, database, cache_entry, selected_queue, pending);

    if (!result) {
        log_this(SR_API, "wait_and_build_single_response: ERROR - build_response_json returned NULL for query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
    } else {
        log_this(SR_API, "wait_and_build_single_response: Query completed, query_ref=%d", LOG_LEVEL_DEBUG, 1, query_ref);
    }

    return result;
}

/**
 * @brief Handle public conduit queries request
 *
 * Main handler for the /api/conduit/queries endpoint. Executes multiple
 * pre-defined queries in parallel without authentication.
 *
 * Request format (POST JSON):
 * {
 *   "database": "database_name",
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
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;  // Unused parameter

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

    log_this(SR_API, "handle_conduit_queries_request: Processing public queries request", LOG_LEVEL_DEBUG, 0);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    log_this(SR_API, "handle_conduit_queries_request: Step 1 - Validate HTTP method", LOG_LEVEL_DEBUG, 0);

    // Step 1: Validate HTTP method using helper from query.h
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        log_this(SR_API, "handle_conduit_queries_request: Method validation failed", LOG_LEVEL_ERROR, 0);
        return result;
    }

    log_this(SR_API, "handle_conduit_queries_request: Step 2 - Parse request", LOG_LEVEL_DEBUG, 0);

    // Step 2: Parse request using helper from query.h
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);

    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) {
        log_this(SR_API, "handle_conduit_queries_request: Request parsing failed", LOG_LEVEL_ERROR, 0);
        return result;
    }

    log_this(SR_API, "handle_conduit_queries_request: Step 3 - Extract database field", LOG_LEVEL_DEBUG, 0);

    // Step 3: Extract database field
    json_t *database_json = json_object_get(request_json, "database");
    if (!database_json || !json_is_string(database_json)) {
        log_this(SR_API, "handle_conduit_queries_request: Missing or invalid database field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: database"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    const char *database = json_string_value(database_json);
    log_this(SR_API, "handle_conduit_queries_request: Database = %s", LOG_LEVEL_DEBUG, 1, database);

    log_this(SR_API, "handle_conduit_queries_request: Step 4 - Extract queries array", LOG_LEVEL_DEBUG, 0);

    // Step 4: Extract queries array
    json_t *queries_array = json_object_get(request_json, "queries");
    if (!queries_array || !json_is_array(queries_array)) {
        log_this(SR_API, "handle_conduit_queries_request: Missing or invalid queries field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: queries (must be array)"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    size_t original_query_count = json_array_size(queries_array);
    log_this(SR_API, "handle_conduit_queries_request: Found %zu queries in array", LOG_LEVEL_DEBUG, 1, original_query_count);

    if (original_query_count == 0) {
        log_this(SR_API, "handle_conduit_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        json_object_set_new(error_response, "results", json_array());
        json_object_set_new(error_response, "database", json_string(database));
        json_object_set_new(error_response, "total_execution_time_ms", json_integer(0));
        return api_send_json_response(connection, error_response, MHD_HTTP_OK);
    }

    log_this(SR_API, "handle_conduit_queries_request: Step 5 - Deduplicate queries and validate rate limits", LOG_LEVEL_DEBUG, 0);

    // Step 5: Deduplicate queries and validate rate limits
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    enum MHD_Result dedup_result = deduplicate_and_validate_queries(queries_array, database, &deduplicated_queries, &mapping_array);
    if (dedup_result != MHD_YES) {
        log_this(SR_API, "handle_conduit_queries_request: Rate limit validation failed", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Rate limit exceeded: too many unique queries in request"));
        return api_send_json_response(connection, error_response, MHD_HTTP_TOO_MANY_REQUESTS);
    }

    size_t unique_query_count = json_array_size(deduplicated_queries);
    log_this(SR_API, "handle_conduit_queries_request: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_query_count, unique_query_count);

    // Step 6: Submit all unique queries for parallel execution
    PendingQueryResult **pending_results = calloc(unique_query_count, sizeof(PendingQueryResult*));
    int *unique_query_refs = calloc(unique_query_count, sizeof(int));
    QueryCacheEntry **cache_entries = calloc(unique_query_count, sizeof(QueryCacheEntry*));
    DatabaseQueue **selected_queues = calloc(unique_query_count, sizeof(DatabaseQueue*));

    if (!pending_results || !unique_query_refs || !cache_entries || !selected_queues) {
        log_this(SR_API, "handle_conduit_queries_request: Failed to allocate memory for parallel execution", LOG_LEVEL_ERROR, 0);
        json_decref(deduplicated_queries);
        free(mapping_array);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_API, "handle_conduit_queries_request: Submitting %zu unique queries for parallel execution", LOG_LEVEL_DEBUG, 1, unique_query_count);

    // Submit all unique queries first
    for (size_t i = 0; i < unique_query_count; i++) {
        json_t *query_obj = json_array_get(deduplicated_queries, i);
        int query_ref;

        PendingQueryResult *pending = submit_single_query(database, query_obj, &query_ref);
        if (!pending) {
            log_this(SR_API, "handle_conduit_queries_request: Failed to submit unique query %zu (query_ref=%d), will return error result", LOG_LEVEL_ERROR, 2, i, query_ref);
            // Create a failed pending result for error handling
            pending_results[i] = NULL;  // Mark as failed
            unique_query_refs[i] = query_ref;

            // Try to look up cache entry for error response building
            DatabaseQueue *db_queue = NULL;
            QueryCacheEntry *cache_entry = NULL;
            if (lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
                cache_entries[i] = cache_entry;
                DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
                selected_queues[i] = selected_queue;
            } else {
                // Query not found - create minimal error response
                cache_entries[i] = NULL;
                selected_queues[i] = NULL;
            }
            continue;
        }

        // Store metadata for response building
        pending_results[i] = pending;
        unique_query_refs[i] = query_ref;

        // Look up cache entry and queue for response building
        DatabaseQueue *db_queue = NULL;
        QueryCacheEntry *cache_entry = NULL;
        if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
            log_this(SR_API, "handle_conduit_queries_request: Failed to lookup metadata for unique query %d", LOG_LEVEL_ERROR, 1, query_ref);
            // Mark as failed but continue with other queries
            pending_results[i] = NULL;
            cache_entries[i] = NULL;
            selected_queues[i] = NULL;
            continue;
        }

        DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
        cache_entries[i] = cache_entry;
        selected_queues[i] = selected_queue;
    }

    // Step 6: Suspend webserver connection for long-running queries
    extern pthread_mutex_t webserver_suspend_lock;
    extern volatile bool webserver_thread_suspended;

    pthread_mutex_lock(&webserver_suspend_lock);
    webserver_thread_suspended = true;
    MHD_suspend_connection(connection);

    // Step 7: Wait for all unique queries to complete
    int collective_timeout = 30;  // Use a reasonable collective timeout
    int wait_result = pending_result_wait_multiple(pending_results, unique_query_count, collective_timeout, SR_API);
    if (wait_result != 0) {
        log_this(SR_API, "handle_conduit_queries_request: Some queries failed or timed out", LOG_LEVEL_ERROR, 0);
    }

    // Step 8: Resume connection processing
    MHD_resume_connection(connection);
    webserver_thread_suspended = false;
    pthread_mutex_unlock(&webserver_suspend_lock);

    // Step 9: Build responses for all unique queries (deduplicated results)
    log_this(SR_API, "handle_conduit_queries_request: Building responses for %zu unique queries", LOG_LEVEL_DEBUG, 1, unique_query_count);
    json_t *results_array = json_array();
    bool all_success = true;

    for (size_t i = 0; i < unique_query_count; i++) {
        log_this(SR_API, "handle_conduit_queries_request: Building response for unique query %zu", LOG_LEVEL_DEBUG, 1, i);

        json_t *query_result = NULL;

        if (!pending_results[i]) {
            // Query submission failed - create error response
            log_this(SR_API, "handle_conduit_queries_request: Query submission failed for unique query %zu, creating error response", LOG_LEVEL_ERROR, 1, i);
            query_result = json_object();
            json_object_set_new(query_result, "success", json_false());
            json_object_set_new(query_result, "query_ref", json_integer(unique_query_refs[i]));
            json_object_set_new(query_result, "database", json_string(database));
            json_object_set_new(query_result, "error", json_string("Query submission failed"));
            all_success = false;
        } else if (!cache_entries[i]) {
            // Query lookup failed - create error response
            log_this(SR_API, "handle_conduit_queries_request: Query lookup failed for unique query %zu, creating error response", LOG_LEVEL_ERROR, 1, i);
            query_result = json_object();
            json_object_set_new(query_result, "success", json_false());
            json_object_set_new(query_result, "query_ref", json_integer(unique_query_refs[i]));
            json_object_set_new(query_result, "database", json_string(database));
            json_object_set_new(query_result, "error", json_string("Query not found"));
            all_success = false;
        } else {
            // Normal case - wait for result
            query_result = wait_and_build_single_response(database, unique_query_refs[i],
                                                         cache_entries[i], selected_queues[i],
                                                         pending_results[i]);

            if (!query_result) {
                log_this(SR_API, "handle_conduit_queries_request: ERROR - wait_and_build_single_response returned NULL for unique query %zu", LOG_LEVEL_ERROR, 1, i);
                // Create fallback error response
                query_result = json_object();
                json_object_set_new(query_result, "success", json_false());
                json_object_set_new(query_result, "query_ref", json_integer(unique_query_refs[i]));
                json_object_set_new(query_result, "database", json_string(database));
                json_object_set_new(query_result, "error", json_string("Response building failed"));
                all_success = false;
            } else {
                // Check if query succeeded
                json_t *success_field = json_object_get(query_result, "success");
                if (!success_field || !json_is_true(success_field)) {
                    all_success = false;
                    log_this(SR_API, "handle_conduit_queries_request: Unique query %zu failed", LOG_LEVEL_DEBUG, 1, i);
                }
            }
        }

        // Remove DQM statistics from individual query results to avoid duplication and reduce response size
        json_object_del(query_result, "dqm_statistics");

        json_array_append_new(results_array, query_result);
        log_this(SR_API, "handle_conduit_queries_request: Response built for unique query %zu", LOG_LEVEL_DEBUG, 1, i);
    }

    json_decref(request_json);

    // Step 10: Calculate total execution time
    log_this(SR_API, "handle_conduit_queries_request: Calculating total execution time", LOG_LEVEL_DEBUG, 0);
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 11: Build response
    log_this(SR_API, "handle_conduit_queries_request: Building final response object", LOG_LEVEL_DEBUG, 0);
    json_t *response_obj = json_object();
    if (!response_obj) {
        log_this(SR_API, "handle_conduit_queries_request: ERROR - Failed to create response object", LOG_LEVEL_ERROR, 0);
        json_decref(results_array);
        // Clean up pending results, etc.
        free(pending_results);
        free(unique_query_refs);
        free(cache_entries);
        free(selected_queues);
        json_decref(deduplicated_queries);
        free(mapping_array);
        json_decref(request_json);
        return MHD_NO;
    }

    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    // DQM statistics are only included in status endpoints, not query endpoints

    log_this(SR_API, "handle_conduit_queries_request: Request completed, queries=%zu, time=%ldms",
             LOG_LEVEL_DEBUG, 2, original_query_count, total_time_ms);

    log_this(SR_API, "handle_conduit_queries_request: Calling api_send_json_response", LOG_LEVEL_DEBUG, 0);
    enum MHD_Result send_result = api_send_json_response(connection, response_obj, MHD_HTTP_OK);
    log_this(SR_API, "handle_conduit_queries_request: api_send_json_response returned %d", LOG_LEVEL_DEBUG, 1, send_result);

    // Clean up resources
    free(pending_results);
    free(unique_query_refs);
    free(cache_entries);
    free(selected_queues);
    json_decref(deduplicated_queries);
    free(mapping_array);

    return send_result;
}
