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
#include <src/logging/logging.h>

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
static json_t* execute_single_query(const char *database, json_t *query_obj)
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
    if (!prepare_and_submit_query(selected_queue, query_id, converted_sql, ordered_params,
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
    (void)url;
    (void)con_cls;  // Unused parameter

    log_this(SR_API, "handle_conduit_queries_request: Processing public queries request", LOG_LEVEL_DEBUG, 0);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Step 1: Validate HTTP method using helper from query.h
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        return result;
    }

    // Step 2: Parse request using helper from query.h
    json_t *request_json = NULL;
    result = handle_request_parsing(connection, method, upload_data, upload_data_size, &request_json);
    if (result != MHD_YES) {
        return result;
    }

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

    size_t query_count = json_array_size(queries_array);
    if (query_count == 0) {
        log_this(SR_API, "handle_conduit_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    // Step 5: Execute all queries
    json_t *results_array = json_array();
    bool all_success = true;

    log_this(SR_API, "handle_conduit_queries_request: Executing %zu queries", LOG_LEVEL_DEBUG, 1, query_count);

    for (size_t i = 0; i < query_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        json_t *query_result = execute_single_query(database, query_obj);
        
        // Check if query succeeded
        json_t *success_field = json_object_get(query_result, "success");
        if (!success_field || !json_is_true(success_field)) {
            all_success = false;
        }
        
        json_array_append_new(results_array, query_result);
    }

    json_decref(request_json);

    // Step 6: Calculate total execution time
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) + 
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 7: Build response
    json_t *response_obj = json_object();
    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    log_this(SR_API, "handle_conduit_queries_request: Request completed, queries=%zu, time=%ldms", 
             LOG_LEVEL_DEBUG, 2, query_count, total_time_ms);
    
    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}
