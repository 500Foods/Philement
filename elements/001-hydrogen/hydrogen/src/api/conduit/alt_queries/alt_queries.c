/**
 * @file alt_queries.c
 * @brief Alternative Authenticated Conduit Queries API endpoint implementation
 *
 * This module implements the authenticated database queries execution endpoint
 * with database override capability. It validates JWT tokens before executing
 * multiple queries in parallel and allows specifying a different database than
 * the one in the JWT claims.
 *
 * @author Hydrogen Framework
 * @date 2026-01-18
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
#include <src/api/conduit/alt_queries/alt_queries.h>
#include <src/api/conduit/query/query.h>  // Use existing helpers
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/logging/logging.h>

/**
 * @brief Validate JWT token for authentication (without extracting database)
 *
 * Validates the provided JWT token for authentication purposes only.
 * Unlike auth_queries, this does not extract the database from the token.
 *
 * @param connection The HTTP connection for error responses
 * @param token The JWT token string to validate
 * @return MHD_YES on success, MHD_NO on validation failure
 */
static enum MHD_Result validate_jwt_for_auth(
    struct MHD_Connection *connection,
    const char *token)
{
    if (!token) {
        log_this(SR_AUTH, "validate_jwt_for_auth: NULL token", LOG_LEVEL_ERROR, 0);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing authentication token"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    // Validate JWT token - pass NULL since database comes from request
    jwt_validation_result_t result = validate_jwt(token, NULL);
    if (!result.valid || !result.claims) {
        log_this(SR_AUTH, "validate_jwt_for_auth: JWT validation failed", LOG_LEVEL_ALERT, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid or expired JWT token"));
        return api_send_json_response(connection, error_response, MHD_HTTP_UNAUTHORIZED);
    }

    log_this(SR_AUTH, "validate_jwt_for_auth: JWT validated successfully", LOG_LEVEL_DEBUG, 0);
    free_jwt_validation_result(&result);
    return MHD_YES;
}

/**
 * @brief Parse alternative authenticated queries request
 *
 * Parses the request JSON and extracts the JWT token, database, and queries array.
 *
 * @param connection The HTTP connection for error responses
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param token Output parameter for JWT token
 * @param database Output parameter for database name
 * @param queries_array Output parameter for queries array
 * @return MHD_YES on success, MHD_NO on parsing failure
 */
static enum MHD_Result parse_alt_queries_request(
    struct MHD_Connection *connection,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    char **token,
    char **database,
    json_t **queries_array)
{
    if (!method || !token || !database || !queries_array) {
        log_this(SR_AUTH, "parse_alt_queries_request: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Parse request data (handles both GET and POST)
    json_t *request_json = parse_request_data(connection, method, upload_data, upload_data_size);
    if (!request_json) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to parse request data", LOG_LEVEL_ERROR, 0);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid request format"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    // Extract token field
    json_t *token_json = json_object_get(request_json, "token");
    if (!token_json || !json_is_string(token_json)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid token field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: token"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    *token = strdup(json_string_value(token_json));
    if (!*token) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to allocate token string", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract database field
    json_t *database_json = json_object_get(request_json, "database");
    if (!database_json || !json_is_string(database_json)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid database field", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: database"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    *database = strdup(json_string_value(database_json));
    if (!*database) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract queries array
    json_t *queries = json_object_get(request_json, "queries");
    if (!queries || !json_is_array(queries)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid queries field", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: queries (must be array)"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    size_t query_count = json_array_size(queries);
    if (query_count == 0) {
        log_this(SR_AUTH, "parse_alt_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    // Deep copy the queries array since we need it after freeing request_json
    *queries_array = json_deep_copy(queries);
    if (!*queries_array) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to copy queries array", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);
        return MHD_NO;
    }

    json_decref(request_json);
    log_this(SR_AUTH, "parse_alt_queries_request: Successfully parsed, database=%s, queries=%zu", LOG_LEVEL_DEBUG, 2, *database, query_count);
    return MHD_YES;
}

/**
 * @brief Submit a single query without waiting for completion
 *
 * Submits a single query from the queries array and returns the pending result
 * for later waiting. This enables parallel execution of multiple queries.
 *
 * @param database Database name from request (overridden)
 * @param query_obj Query object containing query_ref and optional params
 * @param query_ref_out Output parameter for the query_ref (for error reporting)
 * @return PendingQueryResult on success, NULL on failure
 */
static PendingQueryResult* submit_single_query(const char *database, json_t *query_obj, int *query_ref_out)
{
    if (!database || !query_obj || !query_ref_out) {
        log_this(SR_AUTH, "submit_single_query: NULL parameters", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Extract query_ref
    json_t *query_ref_json = json_object_get(query_obj, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_AUTH, "submit_single_query: Missing or invalid query_ref", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    int query_ref = (int)json_integer_value(query_ref_json);
    *query_ref_out = query_ref;
    json_t *params = json_object_get(query_obj, "params");

    // Look up database queue and cache entry using helpers from query.h
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;

    if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
        log_this(SR_AUTH, "submit_single_query: Database or query lookup failed, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Process parameters using helper from query.h
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    if (!process_parameters(params, &param_list, cache_entry->sql_template,
                           db_queue->engine_type, &converted_sql, &ordered_params, &param_count)) {
        log_this(SR_AUTH, "submit_single_query: Parameter processing failed, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Select queue using helper from query.h
    DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_AUTH, "submit_single_query: No suitable queue available, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Generate query ID using helper from query.h
    char *query_id = generate_query_id();
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_AUTH, "submit_single_query: Failed to generate query ID, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
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
        log_this(SR_AUTH, "submit_single_query: Failed to register pending result, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Submit query using helper from query.h
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template, ordered_params,
                                  param_count, cache_entry)) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        log_this(SR_AUTH, "submit_single_query: Failed to submit query, query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
        return NULL;
    }

    // Clean up resources that are no longer needed
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);

    log_this(SR_AUTH, "submit_single_query: Query submitted, query_ref=%d", LOG_LEVEL_DEBUG, 1, query_ref);

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
static json_t* wait_and_build_single_response(const char *database, int query_ref,
                                             const QueryCacheEntry *cache_entry, const DatabaseQueue *selected_queue,
                                             PendingQueryResult *pending)
{
    // Build response using helper from query.h (this waits internally)
    json_t *result = build_response_json(query_ref, database, cache_entry, selected_queue, pending);

    log_this(SR_AUTH, "wait_and_build_single_response: Query completed, query_ref=%d",
             LOG_LEVEL_DEBUG, 1, query_ref);

    return result;
}

/**
 * @brief Handle alternative authenticated conduit queries request
 *
 * Main handler for the /api/conduit/alt_queries endpoint. Validates JWT token,
 * extracts database from request body (overriding JWT claims), and executes
 * multiple queries in parallel.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_alt_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;
    (void)con_cls;  // Unused parameter

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Processing alternative authenticated queries request", LOG_LEVEL_DEBUG, 0);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Step 1: Validate HTTP method using helper from query.h
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        return result;
    }

    // Step 2: Parse request and extract token, database, queries
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;

    result = parse_alt_queries_request(connection, method, upload_data, upload_data_size,
                                      &token, &database, &queries_array);
    if (result != MHD_YES) {
        return result;
    }

    // Step 3: Validate JWT token for authentication
    result = validate_jwt_for_auth(connection, token);
    free(token);  // Token no longer needed after validation
    token = NULL;

    if (result != MHD_YES) {
        free(database);
        json_decref(queries_array);
        return result;
    }

    // Step 4: Submit all queries for parallel execution
    size_t query_count = json_array_size(queries_array);
    PendingQueryResult **pending_results = calloc(query_count, sizeof(PendingQueryResult*));
    int *query_refs = calloc(query_count, sizeof(int));
    QueryCacheEntry **cache_entries = calloc(query_count, sizeof(QueryCacheEntry*));
    DatabaseQueue **selected_queues = calloc(query_count, sizeof(DatabaseQueue*));

    if (!pending_results || !query_refs || !cache_entries || !selected_queues) {
        log_this(SR_AUTH, "handle_conduit_alt_queries_request: Failed to allocate memory for parallel execution", LOG_LEVEL_ERROR, 0);
        free(database);
        json_decref(queries_array);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Submitting %zu queries for parallel execution", LOG_LEVEL_DEBUG, 1, query_count);

    // Submit all queries first
    bool submission_failed = false;
    for (size_t i = 0; i < query_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        int query_ref;

        PendingQueryResult *pending = submit_single_query(database, query_obj, &query_ref);
        if (!pending) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Failed to submit query %zu", LOG_LEVEL_ERROR, 1, i);
            submission_failed = true;
            break;
        }

        // Store metadata for response building
        pending_results[i] = pending;
        query_refs[i] = query_ref;

        // Look up cache entry and queue for response building
        DatabaseQueue *db_queue = NULL;
        QueryCacheEntry *cache_entry = NULL;
        if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Failed to lookup metadata for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            submission_failed = true;
            break;
        }

        DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
        cache_entries[i] = cache_entry;
        selected_queues[i] = selected_queue;
    }

    if (submission_failed) {
        // Clean up allocated resources
        for (size_t i = 0; i < query_count; i++) {
            if (pending_results[i]) {
                // Note: pending results will be cleaned up by the manager
            }
        }
        free(pending_results);
        free(query_refs);
        free(cache_entries);
        free(selected_queues);
        free(database);
        json_decref(queries_array);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Failed to submit queries"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Step 5: Suspend webserver connection for long-running queries
    extern pthread_mutex_t webserver_suspend_lock;
    extern volatile bool webserver_thread_suspended;

    pthread_mutex_lock(&webserver_suspend_lock);
    webserver_thread_suspended = true;
    MHD_suspend_connection(connection);

    // Step 6: Wait for all queries to complete
    int collective_timeout = 30;  // Use a reasonable collective timeout
    int wait_result = pending_result_wait_multiple(pending_results, query_count, collective_timeout, SR_AUTH);
    if (wait_result != 0) {
        log_this(SR_AUTH, "handle_conduit_alt_queries_request: Some queries failed or timed out", LOG_LEVEL_ERROR, 0);
    }

    // Step 7: Resume connection processing
    MHD_resume_connection(connection);
    webserver_thread_suspended = false;
    pthread_mutex_unlock(&webserver_suspend_lock);

    // Step 8: Build responses for all queries
    json_t *results_array = json_array();
    bool all_success = true;

    for (size_t i = 0; i < query_count; i++) {
        json_t *query_result = wait_and_build_single_response(database, query_refs[i],
                                                            cache_entries[i], selected_queues[i],
                                                            pending_results[i]);

        // Check if query succeeded
        json_t *success_field = json_object_get(query_result, "success");
        if (!success_field || !json_is_true(success_field)) {
            all_success = false;
        }

        json_array_append_new(results_array, query_result);
    }

    // Clean up
    free(pending_results);
    free(query_refs);
    free(cache_entries);
    free(selected_queues);

    json_decref(queries_array);

    // Step 5: Calculate total execution time
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 6: Build response
    json_t *response_obj = json_object();
    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    // Add DQM statistics
    if (global_queue_manager) {
        json_t* dqm_stats = database_queue_manager_get_stats_json(global_queue_manager);
        if (dqm_stats) {
            json_object_set_new(response_obj, "dqm_statistics", dqm_stats);
        }
    }

    free(database);

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Request completed, queries=%zu, time=%ldms",
             LOG_LEVEL_DEBUG, 2, query_count, total_time_ms);

    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}